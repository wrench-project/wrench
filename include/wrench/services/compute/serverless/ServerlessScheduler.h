/**
 * Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SERVERLESSSCHEDULER_H
#define WRENCH_SERVERLESSSCHEDULER_H

#include <wrench/function/Invocation.h>
#include <wrench/function/Image.h>
#include <wrench/services/compute/serverless/ServerlessStateOfTheSystem.h>
#include <wrench/services/compute/serverless/Container.h>
#include <vector>
#include <string>

namespace wrench {

    /***********************/
    /** \cond INTERNAL    **/
    /***********************/

    struct TerminateContainer {
        std::shared_ptr<Container> container;
    };

    struct CopyImage {
        std::shared_ptr<Image> image;
        std::shared_ptr<ServerlessComputeNode> compute_node;
    };

    struct LoadImage {
        std::shared_ptr<Image> image;
        std::shared_ptr<ServerlessComputeNode> compute_node;
    };

    struct DispatchInvocation {
        std::shared_ptr<Invocation> invocation;
        ServerlessComputeNode* compute_node;
        std::shared_ptr<Container> container; // nullptr if none
    };

    /**
     * @brief A data structure that stores all scheduling decisions made by a serverless scheduler:
     *        - Which idle containers should be terminated right now
     *        - Which images should be copied from the head node to compute nodes' disks, initiated right now
     *        - Which images should be loaded into compute node's RAMs, initiated right now
     *        - Which invocations should be dispatched right now
     */
    struct ServerlessSchedulingDecisions {
        /** @brief The list of idle containers to terminate */
        std::vector<TerminateContainer> container_terminations;
	    /** @brief The list of image copies to storage at compute nodes */
        std::vector<CopyImage> image_copies_to_disk;
	    /** @brief The list of image loads in RAM at compute nodes */
        std::vector<LoadImage> image_loads_to_RAM;
	    /** @brief The list of function invocations at compute nodes */
        std::vector<DispatchInvocation> invocation_dispatches;

        void print() {
            if (container_terminations.empty() and image_copies_to_disk.empty() and image_loads_to_RAM.empty() and invocation_dispatches.empty()) {
                return;
            }

            std::cerr << "** SCHEDULING DECISIONS **" << std::endl;
            if (not container_terminations.empty()) {
                for (const auto& [container] : container_terminations) {
                    std::cerr << "  Container termination: " << container->getRegisteredFunction()->getImage()->getName() <<
                        "(" <<container->getComputeNode()->hostname << ")" << std::endl;
                }
            }
            if (not image_copies_to_disk.empty()) {
                for (const auto& [image, node] : image_copies_to_disk) {
                    std::cerr << "  Image copy: " << image->getName() << " at " << node->hostname << std::endl;
                }
            }
            if (not image_loads_to_RAM.empty()) {
                for (const auto& [image, node] : image_loads_to_RAM) {
                    std::cerr << "  Image load: " << image->getName() << " at " << node->hostname << std::endl;
                }
            }
            if (not invocation_dispatches.empty()) {
                for (const auto& [invocation, node, container] : invocation_dispatches) {
                    std::cerr << "  Invocation dispatch: for " << invocation->getRegisteredFunction()->getImage()->getName() <<
                        " at " << node->hostname << " (" << (container ? "on an idle container" : "on a new container") << ")" << std::endl;
                }
            }

        }
    };

    /**
     * @brief Abstract base class for scheduling in a serverless compute service.
     */
    class ServerlessScheduler {
    public:
        ServerlessScheduler() = default;
        virtual ~ServerlessScheduler() = default;

        /**
         * @brief Given the list of schedulable invocations and the current system state, decide:
         *   - which idle containers to terminate
         *   - which images to copy to compute nodes
         *   - which images to load into memory at compute nodes
         *   - which invocations to start at compute nodes
         *
         *   Note that the disk and RAM at each compute host is managed using LRU, and so the scheduler
         *   doesn't have full control (which would likely be intractable anyway). But that means that
         *   when asking from the system state questions like "how much RAM is available at that node?"
         *   is not clear-cut: the answer may be 0 but one can perhaps use the node because some memory
         *   content will be evicted due to the LRU behavior...
         *
         *   As a result of the above, the scheduling decisions returned by this method may not all be
         *   feasible, and the same invocations could be passed to the method over and over....
         *
         * @param schedulable_invocations A list of invocations whose images reside on the head node
         * @param state The current system state
         * @return A SchedulingDecisions object
         */
        virtual std::shared_ptr<ServerlessSchedulingDecisions> schedule(
            const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
            const ServerlessStateOfTheSystem* state
        ) = 0;


    };
    /***********************/
    /** \endcond          **/
    /***********************/
} // namespace wrench

#endif // WRENCH_SERVERLESSSCHEDULER_H
