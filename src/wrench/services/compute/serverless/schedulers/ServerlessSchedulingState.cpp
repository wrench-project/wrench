#include <wrench/services/compute/serverless/schedulers/ServerlessSchedulingState.h>
#include <wrench/services/compute/serverless/ServerlessStateOfTheSystem.h>
#include <wrench/function/Function.h>
#include <wrench/logging/TerminalOutput.h>

#include "wrench/function/Image.h"
#include "wrench/services/compute/serverless/Container.h"

WRENCH_LOG_CATEGORY(wrench_serverless_scheduling_state, "Log category for ServerlessSchedulingState");


namespace wrench {

    /**
     * @brief Constructor
     * @param state current system state
     * @param schedulable_invocations the schedulable invocations
     */
    ServerlessSchedulingState::ServerlessSchedulingState(const ServerlessStateOfTheSystem* state,
        const std::vector<std::shared_ptr<Invocation>>&schedulable_invocations) {

        _schedulable_invocations = schedulable_invocations;

        _activated_compute_nodes = state->getActivatedComputeNodes();
        _num_deactivated_compute_nodes = state->getNumDeactivatedComputeNodes();
        _num_being_activated_compute_nodes = state->getNumBeingActivatedComputeNodes();
        _ram_available = state->getAvailableRAMSpace();
        _disk_available = state->getAvailableDiskSpace();

        // std::map<std::shared_ptr<ServerlessComputeNode>, unsigned long> _num_possible_new_invocations_on_compute_node;
        // std::map<std::shared_ptr<ServerlessComputeNode>, unsigned long> _num_possible_new_containers_on_compute_node;
        // std::map<std::shared_ptr<Container>, unsigned long> _num_possible_new_invocations_on_container;

        // Initialize and build useful maps
        for (const auto &node : _activated_compute_nodes) {

            _num_possible_new_containers_on_compute_node[node] = node->getMaxNumContainers() - node->getTotalNumContainers();
            auto num_total_ongoing_invocations = 0UL;
            for (auto const &container : node->getInitializedContainers()) {
                auto num_in_flight_invocations = container->getNumInflightInvocations();
                num_total_ongoing_invocations += num_in_flight_invocations;
                _num_possible_new_invocations_on_container[container] =
                    container->getMaxNumInflightInvocations() - num_in_flight_invocations;
            }
            _num_possible_new_invocations_on_compute_node[node] = node->getMaxNumInvocations() - num_total_ongoing_invocations;

            _image_layers_on_disk[node] = state->getImageLayersOnDiskAtNode(node);
            _image_layers_on_their_way_to_disk[node] = state->getImageLayersBeingCopiedToNode(node);

            _image_layers_in_ram[node] = state->getImageLayersInRAMAtNode(node);
            _image_layers_on_their_way_to_ram[node] = state->getImageLayersBeingLoadedAtNode(node);

            // Make all being-loaded/copied layers protected
            _protected_layers[node] = {};
            auto& protected_layers = _protected_layers.at(node);
            const auto& layers_being_copied =_image_layers_on_their_way_to_disk.at(node);
            protected_layers.insert(layers_being_copied.begin(),layers_being_copied.end());

            const auto& layers_being_loaded =_image_layers_on_their_way_to_ram.at(node);
            protected_layers.insert(layers_being_loaded.begin(),layers_being_loaded.end());

            _initializing_containers[node] = {};
            for (auto const &container : node->getInitializingContainers()) {
                _initializing_containers.at(node).insert(container);
            }

            _initialized_containers[node] = {};
            for (auto const &container : node->getInitializedContainers()) {
                _initialized_containers.at(node).insert(container);
            }
        }

    }

    /**
     * @brief Are all layers of an image in RAM at a compute node?
     * @param image an image
     * @param node the compute node
     * @return True if all necessary layers are in RAM
     */
    bool ServerlessSchedulingState::areAllImageLayersInRAM(const std::shared_ptr<Image>& image, const std::shared_ptr<ServerlessComputeNode> &node) const {
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
    bool ServerlessSchedulingState::areAllImageLayersInRAMOrOnTheirWayToRAM(const std::shared_ptr<Image>& image, const std::shared_ptr<ServerlessComputeNode> &node) const {
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
    bool ServerlessSchedulingState::areAllImageLayersOnDiskOrOnTheirWayToRAM(const std::shared_ptr<Image>& image, const std::shared_ptr<ServerlessComputeNode> &node) const {
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
    bool ServerlessSchedulingState::areAllImageLayersOnDisk(const std::shared_ptr<Image>& image, const std::shared_ptr<ServerlessComputeNode> &node) const {
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
    bool ServerlessSchedulingState::areAllImageLayersOnDiskOrOnTheirWayToDisk(const std::shared_ptr<Image>& image, const std::shared_ptr<ServerlessComputeNode> &node) const {
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
