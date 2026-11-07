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
        for (auto const& node : _activated_compute_nodes) {
            _node_states[node]._ram_available = node->getFreeRAMSpace();
            _node_states[node]._disk_available = node->getFreeDiskSpace();
        }

        // Initialize the pending counts to the outstanding counts
        for (auto const& [function, count] : state->getOutstandingInvocationCounts()) {
            _pending_invocation_counts[function] = count;
        }

        // Initialize and build useful maps
        for (const auto& node : _activated_compute_nodes) {
            _node_states[node]._num_possible_new_containers = node->getMaxNumContainers() - node->
                getTotalNumContainers();
            auto num_total_ongoing_invocations = 0UL;

            for (auto const& container : node->getInitializedContainers()) {
                auto function = container->getFunction();
                auto num_in_flight_invocations = container->getNumInflightInvocations();
                _num_in_flight_invocations_on_container[container] = num_in_flight_invocations;
                _pending_invocation_counts[function] -= num_in_flight_invocations;
                num_total_ongoing_invocations += num_in_flight_invocations;
            }
            for (const auto& container : node->getInitializingContainers()) {
                auto function = container->getFunction();
                auto num_in_flight_invocations = container->getNumInflightInvocations();
                num_total_ongoing_invocations += num_in_flight_invocations;
                _pending_invocation_counts[function] -= num_in_flight_invocations;
            }
            _node_states[node]._num_possible_new_invocations = node->getMaxNumInvocations() -
                num_total_ongoing_invocations;

            _node_states[node]._image_layers_on_disk = state->getImageLayersOnDiskAtNode(node);
            _node_states[node]._image_layers_on_their_way_to_disk = state->getImageLayersBeingStagedToNode(node);

            _node_states[node]._image_layers_in_ram = state->getImageLayersInRAMAtNode(node);
            _node_states[node]._image_layers_on_their_way_to_ram = state->getImageLayersBeingLoadedAtNode(node);

            // Make all being-loaded/staged layers protected
            _node_states[node]._protected_layers = {};
            auto& protected_layers = _node_states[node]._protected_layers;
            const auto& layers_being_staged = _node_states[node]._image_layers_on_their_way_to_disk;
            protected_layers.insert(layers_being_staged.begin(), layers_being_staged.end());

            const auto& layers_being_loaded = _node_states[node]._image_layers_on_their_way_to_ram;
            protected_layers.insert(layers_being_loaded.begin(), layers_being_loaded.end());

            _node_states[node]._initializing_containers = {};
            for (auto const& container : node->getInitializingContainers()) {
                _node_states[node]._initializing_containers.insert(container);
            }

            _node_states[node]._initialized_containers = {};
            for (auto const& container : node->getInitializedContainers()) {
                _node_states[node]._initialized_containers.insert(container);
            }
        }
    }

    /**
     * @brief Get the admitted invocations (images not yet downloaded to the head node)
     * @return A (reference to) the map of admitted invocations (keyed by Image)
     */
    const std::map<std::shared_ptr<Image>, std::vector<std::shared_ptr<Invocation>>>&
    ServerlessSchedulingState::getAdmittedInvocations() const {
        return _admitted_invocations;
    }

    /**
     * @brief Get the schedulable invocations
     * @return A (reference to) the list of schedulable invocations
     */
    std::vector<std::shared_ptr<Invocation>>& ServerlessSchedulingState::getSchedulableInvocations() {
        return _schedulable_invocations;
    }

    /**
     * @brief Get the pending invocation counts
     * @return The map of pending invocations
     */
    const std::map<std::shared_ptr<Function>, unsigned long>&
    ServerlessSchedulingState::getPendingInvocationCounts() const {
        return _pending_invocation_counts;
    }

    /**
     * @brief Get the number of (planned) in-flight invocations at a container
     * @param container A container
     * @return A number of invocations
     */
    unsigned long ServerlessSchedulingState::getNumInFlightInvocationsOnContainer(
        const std::shared_ptr<Container>& container) const {
        return _num_in_flight_invocations_on_container.at(container);
    }

    /**
     * @brief Get the activated compute nodes
     * @return A list of compute nodes
     */
    const std::vector<std::shared_ptr<ServerlessComputeNode>>& ServerlessSchedulingState::getActivatedComputeNodes() {
        return _activated_compute_nodes;
    }

    /**
     * @brief Get the number of compute nodes that are being activated
     * @return A number of nodes
     */
    unsigned long ServerlessSchedulingState::getNumBeingActivatedComputeNodes() const {
        return _num_being_activated_compute_nodes;
    }

    /**
     * @brief Get the number of deactivated compute nodes
     * @return A number of nodes
     */
    unsigned long ServerlessSchedulingState::getNumDeactivatedComputeNodes() const {
        return _num_deactivated_compute_nodes;
    }

    /**
     * @brief Get the initialized containers at a compute node
     * @param node A compute node
     * @return A set of containers
     */
    const std::set<std::shared_ptr<Container>>& ServerlessSchedulingState::getInitializedContainers(
        const std::shared_ptr<ServerlessComputeNode>& node) {
        return _node_states.at(node)._initialized_containers;
    }

    /**
    * @brief Get the initializing containers at a compute node
    * @param node A compute node
    * @return A set of containers
    */
    const std::set<std::shared_ptr<Container>>& ServerlessSchedulingState::getInitializingContainers(
        const std::shared_ptr<ServerlessComputeNode>& node) {
        return _node_states.at(node)._initializing_containers;
    }


    /**
     * @brief Get the node capacity remaining for invocation selections in this round.
     *
     * Starts with the node's invocation limit minus its observed in-flight
     * invocations. Selected dispatches, image preparations, and waits for image
     * preparation each consume one unit during planning.
     * @param node A compute node
     * @return A remaining invocation capacity in this planning snapshot
     */
    unsigned long ServerlessSchedulingState::getNumPossibleNewInvocationsAtComputeNode(
        const std::shared_ptr<ServerlessComputeNode>& node) const {
        return _node_states.at(node)._num_possible_new_invocations;
    }

    /**
     * @brief Get the number of possible new invocations on a container
     * @param container A container
     * @returnA remaining invocation capacity in this planning snapshot
     */
    unsigned long ServerlessSchedulingState::getNumPossibleNewInvocationsOnContainer(
        const std::shared_ptr<Container>& container) const {
        return container->getMaxNumInflightInvocations() - _num_in_flight_invocations_on_container.at(container);
    }

    /**
     * @brief Get the number of possible new containers at a compute node
     * @param node A compute node
     * @return A remaining container capacity in this planning snapshot
     */
    unsigned long ServerlessSchedulingState::getNumPossibleNewContainersAtComputeNode(
        const std::shared_ptr<ServerlessComputeNode>& node) const {
        return _node_states.at(node)._num_possible_new_containers;
    }

    /**
     * @brief Get the protected image layers at a compute node
     * @param node A compute node
     * @return A set of image layers
     */
    const std::set<std::shared_ptr<ImageLayer>>& ServerlessSchedulingState::getProtectedImageLayers(
        const std::shared_ptr<ServerlessComputeNode>& node) {
        return _node_states.at(node)._protected_layers;
    }

    /**
     * @brief Get the image layers on disk at a compute node
     * @param node A compute node
     * @return A set of image layers
     */
    const std::set<std::shared_ptr<ImageLayer>>& ServerlessSchedulingState::getImageLayersOnDisk(
        const std::shared_ptr<ServerlessComputeNode>& node) {
        return _node_states.at(node)._image_layers_on_disk;
    }

    /**
     * @brief Get the image layers on their way to disk at a compute node
     * @param node A compute node
     * @return A set of image layers
     */
    const std::set<std::shared_ptr<ImageLayer>>& ServerlessSchedulingState::getImageLayersOnTheirWayToDisk(
        const std::shared_ptr<ServerlessComputeNode>& node) {
        return _node_states.at(node)._image_layers_on_their_way_to_disk;
    }

    /**
     * @brief Get the image layers in RAM at a compute node
     * @param node A compute node
     * @return A set of image layers
     */
    const std::set<std::shared_ptr<ImageLayer>>& ServerlessSchedulingState::getImageLayersInRAM(
        const std::shared_ptr<ServerlessComputeNode>& node) {
        return _node_states.at(node)._image_layers_in_ram;
    }

    /**
     * @brief Get the image layers on their way to RAM at a compute node
     * @param node A compute node
     * @return A set of image layers
     */
    const std::set<std::shared_ptr<ImageLayer>>& ServerlessSchedulingState::getImageLayersOnTheirWayToRAM(
        const std::shared_ptr<ServerlessComputeNode>& node) const {
        return _node_states.at(node)._image_layers_on_their_way_to_ram;
    }

    /**
     * @brief Get the RAM space (in bytes) available at a compute node
     * @param node A compute node
     * @return A number of bytes
     */
    sg_size_t ServerlessSchedulingState::getRAMSpaceAvailable(
        const std::shared_ptr<ServerlessComputeNode>& node) const {
        return _node_states.at(node)._ram_available;
    }

    /**
     * @brief Get the disk space (in bytes) available at a compute node
     * @param node A compute node
     * @return A number of bytes
     */
    sg_size_t ServerlessSchedulingState::getDiskSpaceAvailable(
        const std::shared_ptr<ServerlessComputeNode>& node) const {
        return _node_states.at(node)._disk_available;
    }


    /**
    * @brief Claim one unit of a node's invocation capacity in this planning round.
    *
    * Updates only the planning snapshot. Does not assign an invocation to the
    * node, reserve a container or its private resources, change pending
    * invocation counts, or append a scheduling decision.
    *
    * Each invocation selecting a node must consume exactly one unit of node
    * capacity. Do not make a separate claim if dispatch accounting already
    * includes it.
    * @param node A compute node
    */
    void ServerlessSchedulingState::reserveInvocationSlot(const std::shared_ptr<ServerlessComputeNode>& node) {

        auto& available =
            _node_states.at(node)._num_possible_new_invocations;

        if (available == 0) {
            throw std::runtime_error(
                "Cannot reserve an invocation slot on a full node");
        }
        --available;
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

        // Remove node's container
        for (const auto& container :
             _node_states.at(node)._initialized_containers) {
            _num_in_flight_invocations_on_container.erase(container);
        }

        // Remove all node-related bookkeeping
        _node_states.erase(node);
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
        _node_states.at(node)._protected_layers.insert(layers.begin(), layers.end());
    }

    /**
     * @brief Plan for the termination of an idle container
     * @param container the container
     */
    void ServerlessSchedulingState::planIdleContainerTermination(const std::shared_ptr<ServerlessComputeNode>& node,
                                                                 const std::shared_ptr<Container>& container) {
        _node_states.at(node)._ram_available += container->getFunction()->getRAMSpaceLimit();
        _node_states.at(node)._disk_available += container->getFunction()->getDiskSpaceLimit();
        _node_states.at(node)._num_possible_new_containers += 1;
        _node_states.at(node)._initialized_containers.erase(container);
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
        if (_node_states.at(node)._protected_layers.count(layer)) {
            throw std::runtime_error(
                "Eviction plan attempts to evict a protected RAM layer");
        }
        auto& resident_layers = _node_states.at(node)._image_layers_in_ram;

        if (resident_layers.erase(layer) != 1) {
            throw std::runtime_error(
                "Eviction plan attempts to evict a nonresident "
                "or already-evicted RAM layer");
        }
        _node_states.at(node)._ram_available += layer->getRAMFootprint();
    }

    /**
     * @brief Plan a layer eviction from Disk
     *
     * @param node The compute node where the eviction will take place
     * @param layer The layer that will be evicted
     */
    void ServerlessSchedulingState::planLayerEvictionFromDisk(const std::shared_ptr<ServerlessComputeNode>& node,
                                                              const std::shared_ptr<ImageLayer>& layer) {
        if (_node_states.at(node)._protected_layers.count(layer)) {
            throw std::runtime_error(
                "Eviction plan attempts to evict a protected disk layer");
        }
        auto& resident_layers = _node_states.at(node)._image_layers_on_disk;

        if (resident_layers.erase(layer) != 1) {
            throw std::runtime_error(
                "Eviction plan attempts to evict a nonresident "
                "or already-evicted disk layer");
        }
        _node_states.at(node)._disk_available += layer->getDiskFootprint();
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
            _num_in_flight_invocations_on_container.at(container) += 1;
        } else {
            // The selected plan's container terminations have already been credited.
            _node_states.at(node)._num_possible_new_containers -= 1;
            _node_states.at(node)._ram_available -= inv->getFunction()->getRAMSpaceLimit();
            _node_states.at(node)._disk_available -= inv->getFunction()->getDiskSpaceLimit();
        }
        _node_states.at(node)._num_possible_new_invocations -= 1;

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
        _node_states.at(node)._image_layers_on_their_way_to_ram.insert(layer);
        _node_states.at(node)._ram_available -= layer->getRAMFootprint();
    }

    /**
     * @brief Plan for a layer staging to disk
     * @param node The node at which to stage
     * @param layer The layer to stage
     */
    void ServerlessSchedulingState::planLayerStagingToDisk(const std::shared_ptr<ServerlessComputeNode>& node,
                                                           const std::shared_ptr<ImageLayer>& layer) {
        _node_states.at(node)._image_layers_on_their_way_to_disk.insert(layer);
        _node_states.at(node)._disk_available -= layer->getDiskFootprint();
    }

    /**
     * @brief Plan for a container prewarm
     * @param node The compute node at which to prewarm a container
     */
    void ServerlessSchedulingState::planContainerPrewarm(const std::shared_ptr<ServerlessComputeNode>& node,
                                                         const std::shared_ptr<Function>& function) {
        _node_states.at(node)._num_possible_new_containers -= 1;
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
            if (_node_states.at(node)._image_layers_in_ram.count(layer) == 0) {
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
            if (_node_states.at(node)._image_layers_in_ram.count(layer))
                continue;
            if (_node_states.at(node)._image_layers_on_their_way_to_ram.count(layer))
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
            if ((_node_states.at(node)._image_layers_on_disk.count(layer) == 0) and
                (_node_states.at(node)._image_layers_on_their_way_to_ram.count(layer) == 0)) {
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
            if (_node_states.at(node)._image_layers_on_disk.count(layer) == 0) {
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
            if ((_node_states.at(node)._image_layers_on_disk.count(layer) == 0) and
                (_node_states.at(node)._image_layers_on_their_way_to_disk.count(layer) == 0)) {
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
        for (auto const& container : _node_states.at(node)._initialized_containers) {
            if (_num_in_flight_invocations_on_container.at(container) == 0) {
                idle_containers.insert(container);
            }
        }
        return idle_containers;
    }
}
