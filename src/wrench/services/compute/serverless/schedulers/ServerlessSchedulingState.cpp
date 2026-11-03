#include "wrench/services/compute/serverless/schedulers/ServerlessSchedulingState.h"
#include "wrench/services/compute/serverless/ServerlessStateOfTheSystem.h"
#include "wrench/function/Function.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/function/Image.h"
#include "wrench/services/compute/serverless/Container.h"

WRENCH_LOG_CATEGORY(wrench_serverless_scheduling_state, "Log category for ServerlessSchedulingState");


namespace wrench {
    /**
     * @brief Constructor
     * @param state current system state
     */
    ServerlessSchedulingState::ServerlessSchedulingState(const ServerlessStateOfTheSystem* state) {
        _admitted_invocations = state->getAdmittedInvocations();
        _schedulable_invocations = state->getSchedulableInvocations();

        _activated_compute_nodes = state->getActivatedComputeNodes();
        _num_deactivated_compute_nodes = state->getNumDeactivatedComputeNodes();
        _num_being_activated_compute_nodes = state->getNumBeingActivatedComputeNodes();
        _ram_available = state->getAvailableRAMSpace();
        _disk_available = state->getAvailableDiskSpace();

        // Initialize the pending counts to the outstanding counts
        for (auto const& [function, count] : state->getOutstandingInvocationCounts()) {
            _pending_invocation_counts[function] = count;
        }

        // Initialize and build useful maps
        for (const auto& node : _activated_compute_nodes) {
            _num_possible_new_containers_on_compute_node[node] = node->getMaxNumContainers() - node->
                getTotalNumContainers();
            auto num_total_ongoing_invocations = 0UL;

            for (auto const& container : node->getInitializedContainers()) {
                auto function = container->getFunction();
                auto num_in_flight_invocations = container->getNumInflightInvocations();
                _num_in_flight_invocations_on_container[container] = num_in_flight_invocations;
                _pending_invocation_counts[function] -= num_in_flight_invocations;
                num_total_ongoing_invocations += num_in_flight_invocations;
                _num_possible_new_invocations_on_container[container] =
                    container->getMaxNumInflightInvocations() - num_in_flight_invocations;
            }
            for (const auto& container : node->getInitializingContainers()) {
                auto function = container->getFunction();
                auto num_in_flight_invocations = container->getNumInflightInvocations();
                num_total_ongoing_invocations += num_in_flight_invocations;
                _pending_invocation_counts[function] -= num_in_flight_invocations;
            }
            _num_possible_new_invocations_on_compute_node[node] = node->getMaxNumInvocations() -
                num_total_ongoing_invocations;

            _image_layers_on_disk[node] = state->getImageLayersOnDiskAtNode(node);
            _image_layers_on_their_way_to_disk[node] = state->getImageLayersBeingCopiedToNode(node);

            _image_layers_in_ram[node] = state->getImageLayersInRAMAtNode(node);
            _image_layers_on_their_way_to_ram[node] = state->getImageLayersBeingLoadedAtNode(node);

            // Make all being-loaded/copied layers protected
            _protected_layers[node] = {};
            auto& protected_layers = _protected_layers.at(node);
            const auto& layers_being_copied = _image_layers_on_their_way_to_disk.at(node);
            protected_layers.insert(layers_being_copied.begin(), layers_being_copied.end());

            const auto& layers_being_loaded = _image_layers_on_their_way_to_ram.at(node);
            protected_layers.insert(layers_being_loaded.begin(), layers_being_loaded.end());

            _initializing_containers[node] = {};
            for (auto const& container : node->getInitializingContainers()) {
                _initializing_containers.at(node).insert(container);
            }

            _initialized_containers[node] = {};
            for (auto const& container : node->getInitializedContainers()) {
                _initialized_containers.at(node).insert(container);
            }
        }
    }

    /**
     * @brief Exclude a compute node from all future planning
     * @param node 
     */
    void ServerlessSchedulingState::excludeNodeFromPlanning(const std::shared_ptr<ServerlessComputeNode>& node) {
        auto& active_nodes = _activated_compute_nodes;
        const auto node_it =
            std::find(active_nodes.begin(), active_nodes.end(), node);
        if (node_it == active_nodes.end()) {
            return;
        }
        active_nodes.erase(node_it);
        _num_deactivated_compute_nodes += 1;

        _disk_available.erase(node);
        _ram_available.erase(node);
        _image_layers_on_disk.erase(node);
        _image_layers_on_their_way_to_disk.erase(node);
        _image_layers_in_ram.erase(node);
        _image_layers_on_their_way_to_ram.erase(node);
        _protected_layers.erase(node);

        _initializing_containers.erase(node);
        for (const auto& container :
             _initialized_containers.at(node)) {
            _num_possible_new_invocations_on_container.erase(container);
            _num_in_flight_invocations_on_container.erase(container);
        }
        _initialized_containers.erase(node);

        _num_possible_new_invocations_on_compute_node.erase(node);
        _num_possible_new_containers_on_compute_node.erase(node);
    }

    /**
     * @brief Plan to fail some invocations
     * @param invocations A set of invocations to fail
     */
    void ServerlessSchedulingState::planInvocationFailures(const std::set<std::shared_ptr<Invocation>>& invocations) {
        if (invocations.empty()) {
            return;
        }

        // Filter out the "to fail" invocations from the list of schedulable invocations
        auto& pending = _schedulable_invocations;
        pending.erase(
            std::remove_if(
                pending.begin(),
                pending.end(),
                [&invocations](const auto& invocation) {
                    return invocations.count(invocation) != 0;
                }),
            pending.end());

        // Adjust the pending counts
        for (auto const& inv : invocations) {
            _pending_invocation_counts.at(inv->getFunction())--;
        }
    }

    /**
     * @brief Make all of an image's layer protected at a compute node
     * @param node The compute node
     * @param image The image
     */
    void ServerlessSchedulingState::protectImageLayers(const std::shared_ptr<ServerlessComputeNode>& node,
                                                       const std::shared_ptr<Image>& image) {
        const auto& layers = image->getLayers();
        _protected_layers.at(node).insert(layers.begin(), layers.end());
    }

    /**
     * @brief Plan for the termination of an idle container
     * @param container the container
     */
    void ServerlessSchedulingState::planIdleContainerTermination(const std::shared_ptr<ServerlessComputeNode>& node,
                                                                 const std::shared_ptr<Container>& container) {
        _ram_available.at(node) += container->getFunction()->getRAMSpaceLimit();
        _disk_available.at(node) += container->getFunction()->getDiskSpaceLimit();
        _num_possible_new_containers_on_compute_node.at(node) += 1;
        _initialized_containers.at(node).erase(container);
        _num_possible_new_invocations_on_container.erase(container);
        _num_in_flight_invocations_on_container.erase(container);
    }

    /**
     * @brief Plan a layer eviction from RAM
     *
     * @param node The compute node where the eviction will take place
     * @param layer The layer that will be evicted
     */
    void ServerlessSchedulingState::planLayerEvictionFromRAM(const std::shared_ptr<ServerlessComputeNode>& node,
                                                             const std::shared_ptr<ImageLayer>& layer) {
        if (_protected_layers.at(node).count(layer)) {
            throw std::runtime_error(
                "Eviction plan attempts to evict a protected RAM layer");
        }
        auto& resident_layers = _image_layers_in_ram.at(node);

        if (resident_layers.erase(layer) != 1) {
            throw std::runtime_error(
                "Eviction plan attempts to evict a nonresident "
                "or already-evicted RAM layer");
        }
        _ram_available.at(node) += layer->getRAMFootprint();
    }

    /**
     * @brief Plan a layer eviction from Disk
     *
     * @param node The compute node where the eviction will take place
     * @param layer The layer that will be evicted
     */
    void ServerlessSchedulingState::planLayerEvictionFromDisk(const std::shared_ptr<ServerlessComputeNode>& node,
                                                              const std::shared_ptr<ImageLayer>& layer) {
        if (_protected_layers.at(node).count(layer)) {
            throw std::runtime_error(
                "Eviction plan attempts to evict a protected disk layer");
        }
        auto& resident_layers = _image_layers_on_disk.at(node);

        if (resident_layers.erase(layer) != 1) {
            throw std::runtime_error(
                "Eviction plan attempts to evict a nonresident "
                "or already-evicted disk layer");
        }
        _disk_available.at(node) += layer->getDiskFootprint();
    }


    /**
     * @brief Plan for a new invocation
     * @param inv The invocation to place
     * @param node The node on which to place it
     * @param container The container to use (if non-nullptr)
     */
    void ServerlessSchedulingState::planInvocationDispatch(const std::shared_ptr<Invocation>& inv,
                                                           const std::shared_ptr<ServerlessComputeNode>& node,
                                                           const std::shared_ptr<Container>& container) {
        if (container) {
            _num_possible_new_invocations_on_container.at(container) -= 1;
            _num_in_flight_invocations_on_container.at(container) += 1;
        } else {
            // The selected plan's container terminations have already been credited.
            _num_possible_new_containers_on_compute_node.at(node) -= 1;
            _ram_available.at(node) -= inv->getFunction()->getRAMSpaceLimit();
            _disk_available.at(node) -= inv->getFunction()->getDiskSpaceLimit();
        }
        _num_possible_new_invocations_on_compute_node.at(node) -= 1;

        // Update the pending counts
        auto& num_pending = _pending_invocation_counts.at(inv->getFunction());
        if (num_pending == 0) {
            throw std::runtime_error("Cannot select a dispatch for a function with zero pending invocations");
        }
        --num_pending;
    }

    /**
     * @brief Plan for a layer load to RAM
     * @param node The node at which to load
     * @param layer The layer to load
     */
    void ServerlessSchedulingState::planLayerLoadToRAM(const std::shared_ptr<ServerlessComputeNode>& node,
                                                       const std::shared_ptr<ImageLayer>& layer) {
        _image_layers_on_their_way_to_ram.at(node).insert(layer);
        _ram_available.at(node) -= layer->getRAMFootprint();
    }

    /**
     * @brief Plan for a layer copy to disk
     * @param node The node at which to copy
     * @param layer The layer to copy
     */
    void ServerlessSchedulingState::planLayerCopyToDisk(const std::shared_ptr<ServerlessComputeNode>& node,
                                                        const std::shared_ptr<ImageLayer>& layer) {
        _image_layers_on_their_way_to_disk.at(node).insert(layer);
        _disk_available.at(node) -= layer->getDiskFootprint();
    }

    /**
     * @brief Plan for a container prewarm
     * @param node The compute node at which to prewarm a container
     */
    void ServerlessSchedulingState::planContainerPrewarm(const std::shared_ptr<ServerlessComputeNode>& node,
                                                         const std::shared_ptr<Function>& function) {
        _num_possible_new_containers_on_compute_node.at(node) -= 1;
    }


    /**
     * @brief Are all layers of an image in RAM at a compute node?
     * @param image an image
     * @param node the compute node
     * @return True if all necessary layers are in RAM
     */
    bool ServerlessSchedulingState::areAllImageLayersInRAM(const std::shared_ptr<Image>& image,
                                                           const std::shared_ptr<ServerlessComputeNode>& node) const {
        for (auto const& layer : image->getLayers()) {
            if (_image_layers_in_ram.at(node).count(layer) == 0) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Are all layers either in ram or on their way to ram at a compute node?
     * @param image an image
     * @param node the compute node
     * @return True or false
     */
    bool ServerlessSchedulingState::areAllImageLayersInRAMOrOnTheirWayToRAM(
        const std::shared_ptr<Image>& image, const std::shared_ptr<ServerlessComputeNode>& node) const {
        for (auto const& layer : image->getLayers()) {
            if (_image_layers_in_ram.at(node).count(layer))
                continue;
            if (_image_layers_on_their_way_to_ram.at(node).count(layer))
                continue;

            return false;
        }
        return true;
    }


    /**
     * @brief Are all layers either on disk or on their way to ram at a compute node?
     * @param image an image
     * @param node the compute node
     * @return True or false
     */
    bool ServerlessSchedulingState::areAllImageLayersOnDiskOrOnTheirWayToRAM(
        const std::shared_ptr<Image>& image, const std::shared_ptr<ServerlessComputeNode>& node) const {
        for (auto const& layer : image->getLayers()) {
            if ((_image_layers_on_disk.at(node).count(layer) == 0) and
                (_image_layers_on_their_way_to_ram.at(node).count(layer) == 0)) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Are all layers either on disk or on their way to ram at a compute node?
     * @param image an image
     * @param node the compute node
     * @return True or false
     */
    bool ServerlessSchedulingState::areAllImageLayersOnDisk(const std::shared_ptr<Image>& image,
                                                            const std::shared_ptr<ServerlessComputeNode>& node) const {
        for (auto const& layer : image->getLayers()) {
            if (_image_layers_on_disk.at(node).count(layer) == 0) {
                return false;
            }
        }
        return true;
    }


    /**
     * @brief Are all layers either on disk or on their way to ram at a compute node?
     * @param image an image
     * @param node the compute node
     * @return True or false
     */
    bool ServerlessSchedulingState::areAllImageLayersOnDiskOrOnTheirWayToDisk(
        const std::shared_ptr<Image>& image, const std::shared_ptr<ServerlessComputeNode>& node) const {
        for (auto const& layer : image->getLayers()) {
            if ((_image_layers_on_disk.at(node).count(layer) == 0) and
                (_image_layers_on_their_way_to_disk.at(node).count(layer) == 0)) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Get the set of idle containers at a compute node
     * @param node the compute node
     * @return a set of idle containers
     */
    std::set<std::shared_ptr<Container>> ServerlessSchedulingState::getIdleContainersAtComputeNode(
        const std::shared_ptr<ServerlessComputeNode>& node) const {
        std::set<std::shared_ptr<Container>> idle_containers;
        for (auto const& container : _initialized_containers.at(node)) {
            if (_num_possible_new_invocations_on_container.at(container) == container->getMaxNumInflightInvocations()) {
                idle_containers.insert(container);
            }
        }
        return idle_containers;
    }
}
