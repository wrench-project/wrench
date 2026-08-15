#include <wrench/services/compute/serverless/schedulers/greedy/GreedySchedulingState.h>
#include <wrench/services/compute/serverless/ServerlessStateOfTheSystem.h>
#include <wrench/function/RegisteredFunction.h>
#include <wrench/logging/TerminalOutput.h>

#include "wrench/function/Image.h"
#include "wrench/services/compute/serverless/Container.h"

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

        // Determine the set of relevant images
        std::set<std::shared_ptr<Image>> relevant_images;
        for (auto const &inv: schedulable_invocations) {
            std::cerr << "** SCHEDULING STATE: RELEVANT IMAGE: " << inv->getRegisteredFunction()->getImage()->getName() << "\n";
            relevant_images.insert(inv->getRegisteredFunction()->getImage());
        }


        // Initialize and build useful maps
        for (const auto &node : state->getComputeNodes()) {
            images_on_disk[node] = {};
            images_in_ram[node] = {};
            images_on_their_way_to_disk[node] = {};
            images_on_their_way_to_ram[node] = {};
            idle_containers[node] = {};

            for (const auto &image: relevant_images) {
                std::cerr << "IS IMAGE ON DISK : " << image->getName() << "  " << node->isImageOnDisk(image) << "\n";
                if (node->isImageOnDisk(image)) {
                    images_on_disk.at(node).insert(image);
                }
                std::cerr << "IS IMAGE IN RAM : " << image->getName() << "  " << node->isImageInRAM(image) << "\n";

                if (node->isImageInRAM(image)) {
                    images_in_ram.at(node).insert(image);
                }
                if (node->isImageBeingCopied(image)) {
                    images_on_their_way_to_disk.at(node).insert(image);
                }
                if (node->isImageBeingLoaded(image)) {
                    images_on_their_way_to_ram.at(node).insert(image);
                }
                for (auto const &container : node->getIdleContainers()) {
                    auto container_image = container->getRegisteredFunction()->getImage();
                    if (relevant_images.count(container_image)) {
                        idle_containers.at(node).insert(container);
                    }
                }
            }
        }

    }

}
