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

#include <wrench/services/compute/serverless/Invocation.h>
#include <wrench/services/compute/serverless/ServerlessStateOfTheSystem.h>
#include <vector>
#include <string>

namespace wrench {

    /***********************/
    /** \cond DEVELOPER   **/
    /***********************/

    /**
     * @brief A data structure that stores all scheduling decisions made by a serverless scheduler:
     *        - Which images should be copied from the head node to compute nodes' disks
     *        - Which images should be loaded into compute node's RAMs
     *        - Which invocations should be started at compute nodes
     */
    struct ServerlessSchedulingDecisions {
	/** @brief The hostname-keyed map of images to copy to compute nodes */
        std::map<std::string, std::vector<std::shared_ptr<DataFile>>> images_to_copy_to_compute_node;
	/** @brief The hostname-keyed map of images to load in RAM at compute nodes */
        std::map<std::string, std::vector<std::shared_ptr<DataFile>>> images_to_load_into_RAM_at_compute_node;
	/** @brief The hostname-keyed map of functions to invoke at compute nodes */
        std::map<std::string, std::vector<std::shared_ptr<Invocation>>> invocations_to_start_at_compute_node;
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
            const std::shared_ptr<ServerlessStateOfTheSystem>& state
        ) = 0;


    };
    /***********************/
    /** \endcond          **/
    /***********************/
} // namespace wrench

#endif // WRENCH_SERVERLESSSCHEDULER_H
