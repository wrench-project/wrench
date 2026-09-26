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

        _compute_nodes = state->getComputeNodes();
        _cores_available = state->getAvailableCores();
        _ram_available = state->getAvailableRAMSpace();
        _disk_available = state->getAvailableDiskSpace();

        // Determine the set of relevant images
        std::set<std::shared_ptr<Image>> relevant_images;
        for (auto const &inv: schedulable_invocations) {
            relevant_images.insert(inv->getFunction()->getImage());
        }

        // Initialize and build useful maps
        for (const auto &node : state->getComputeNodes()) {

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

            _idle_containers[node] = {};
            for (auto const &container : node->getIdleContainers()) {
                _idle_containers.at(node).insert(container);
            }

            _busy_containers[node] = {};
            for (auto const &container : node->getBusyContainers()) {
                _busy_containers.at(node).insert(container);
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

}
