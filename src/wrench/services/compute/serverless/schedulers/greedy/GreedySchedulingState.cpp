#include <wrench/services/compute/serverless/schedulers/greedy/GreedySchedulingState.h>
#include <wrench/services/compute/serverless/ServerlessStateOfTheSystem.h>
#include <wrench/function/RegisteredFunction.h>
#include <wrench/logging/TerminalOutput.h>

#include "wrench/function/Image.h"

WRENCH_LOG_CATEGORY(wrench_greedy_scheduling_state, "Log category for GreedySchedulingState");


namespace wrench {

    /**
     * @brief Constructor
     * @param state current system state
     * @param schedulable_invocations the schedulable invocations
     */
    GreedySchedulingState::GreedySchedulingState(const ServerlessStateOfTheSystem* state,
        const std::vector<std::shared_ptr<Invocation>>&schedulable_invocations) {

        compute_nodes   = state->getComputeNodes();
        cores_available = state->getAvailableCores();
        // disk_available  = state->getAvailableDiskSpace();
        // ram_available   = state->getAvailableRAMSpace();

        // Incorporate the "one the way to ram/disk" into availability (which cannot account for LRU behavior though)
        // for (auto const &node: compute_nodes) {
        //     for (auto const &image: node->getImagesBeingLoaded()) {
        //         ram_available.at(node) -= image->getRAMFootprint();
        //     }
        //     for (auto const &image: node->getImagesBeingCopied()) {
        //         ram_available.at(node) -= image->getDiskFootprint();
        //     }
        // }

        // Determine the set of relevant images
        std::set<std::shared_ptr<Image>> relevant_images;
        for (auto const &inv: schedulable_invocations) {
            relevant_images.insert(inv->getRegisteredFunction()->getImage());
        }

        // Build useful maps
        for (const auto &node : state->getComputeNodes()) {
            for (const auto &image: relevant_images) {
                if (node->isImageOnDisk(image)) {
                    images_on_disk[node].insert(image);
                }
                if (node->isImageInRAM(image)) {
                    images_in_ram[node].insert(image);
                }
                if (node->isImageBeingCopied(image)) {
                    images_on_their_way_to_disk[node].insert(image);
                }
                if (node->isImageBeingLoaded(image)) {
                    images_on_their_way_to_ram[node].insert(image);
                }
            }
        }

    }

}
