/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_MULTICORECOMPUTESERVICEPROPERTY_H
#define WRENCH_MULTICORECOMPUTESERVICEPROPERTY_H

#include <map>

#include "wrench/services/compute/ComputeServiceProperty.h"

namespace wrench {

   /**
    * @brief Properties for a MulticoreComputeService
    */
    class MultihostMulticoreComputeServiceProperty : public ComputeServiceProperty {

    public:

        /** @brief The number of bytes in the control message sent by the daemon to state that it does not have sufficient cores to (ever) run a submitted job **/
        DECLARE_PROPERTY_NAME(NOT_ENOUGH_CORES_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent to the daemon to ask it for its per-core flop rate **/
        DECLARE_PROPERTY_NAME(FLOP_RATE_REQUEST_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent by the daemon to state its per-core flop rate **/
        DECLARE_PROPERTY_NAME(FLOP_RATE_ANSWER_MESSAGE_PAYLOAD);

        /** @brief The overhead to start a thread execution, in seconds **/
        DECLARE_PROPERTY_NAME(THREAD_STARTUP_OVERHEAD);

        /** @brief The job selection policy:
         *      - FCFS: serve jobs in First-Come-First-Serve manner
         */
        DECLARE_PROPERTY_NAME(JOB_SELECTION_POLICY);

        /** @brief The resource allocation policy:
         *      - aggressive: Give each job as much as it might need (hosts and cores)
         *                    assuming that all computational tasks can run in parallel and
         *                    can use as many cores as possible.
         */
        DECLARE_PROPERTY_NAME(RESOURCE_ALLOCATION_POLICY);

        /** @brief The algorithm that, once a resource allocation has been determined
         *         for a job, decides how many cores are given to
         *         a computational task. Possible values are:
         *                  - maximum (default)
         *                  - minimum
         **/
        DECLARE_PROPERTY_NAME(TASK_SCHEDULING_CORE_ALLOCATION_ALGORITHM);

        /** @brief The algorithm that, once a resource allocation has been determined
         *         for a job, decides which ready computational task,
         *         in case multiple can be ran, should run first. Possible values are:
         *                  - maximum_flops (default)
         *                  - maximum_minimum_cores
         **/

        DECLARE_PROPERTY_NAME(TASK_SCHEDULING_TASK_SELECTION_ALGORITHM);

        /** @brief The algorithm that decides, once a resource allocation has been determined
         *         for a job, on which host a task should be placed. Possible values are:
         *                  - best fit (default)
         */
        DECLARE_PROPERTY_NAME(TASK_SCHEDULING_HOST_SELECTION_ALGORITHM);

    };

};


#endif //WRENCH_MULTICORECOMPUTESERVICEPROPERTY_H
