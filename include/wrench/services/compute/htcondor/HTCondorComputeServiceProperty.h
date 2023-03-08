/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_HTCONDORCOMPUTESERVICEPROPERTY_H
#define WRENCH_HTCONDORCOMPUTESERVICEPROPERTY_H

#include "wrench/services/compute/ComputeServiceProperty.h"

namespace wrench {

    /**
     * @brief Properties for an HTCondor service
     */
    class HTCondorComputeServiceProperty : public ComputeServiceProperty {

    public:
        /**
         * @brief Overhead of the HTCondor Negotiator, which is invoked each time a new job is submitted or
         *  a running job completes and there are still pending jobs. Default: "0". Default unit: second.
         *  Examples: "1s", "200ms", "1.5s", etc.
         **/
        DECLARE_PROPERTY_NAME(NEGOTIATOR_OVERHEAD);

        /**
         * @brief Overhead between condor and a batch compute service for the start of execution of grid-universe jobs.
         *  Default unit: second. Examples: "1s", "200ms", "1.5s", etc.
         */
        DECLARE_PROPERTY_NAME(GRID_PRE_EXECUTION_DELAY);

        /**
         * @brief Overhead between condor and a batch compute service for the completion of execution of grid-universe jobs.
         *  Default unit: second. Examples: "1s", "200ms", "1.5s", etc.
         */
        DECLARE_PROPERTY_NAME(GRID_POST_EXECUTION_DELAY);

        /**
         * @brief Overhead between condor and a bare-metal compute service for the start of execution of non-grid-universe jobs.
         *  Default unit: second. Examples: "1s", "200ms", "1.5s", etc.
         */
        DECLARE_PROPERTY_NAME(NON_GRID_PRE_EXECUTION_DELAY);

        /**
         * @brief Overhead between condor and a bare-metal compute for the completion of execution of non-grid-universe jobs.
         *  Default unit: second. Examples: "1s", "200ms", "1.5s", etc.
         */
        DECLARE_PROPERTY_NAME(NON_GRID_POST_EXECUTION_DELAY);

        /**
         * @brief Whether the HTCondorComputeService should contact its subordinate BareMetalComputeServices (i.e.,
         * when running non-grid jobs) to find out about their currently available resources, or instead keep its own record
         * of their available resources. The two options are:
         *   - "true": Messages are exchanged between the HTCondorComputeService (i.e., the negotiator) and subordinate
         *     BareMetalComputeServices to find out their current available resources (with the caveat that by the time they respond
         *     they could be used by some other job directly submitted to then via other means, in which case their response
         *     is stale and their resources may become oversubscribed).
         *   - "false" (default): This makes simulation faster since fewer messages are exchanged, but will lead to
         *     possibly different executions than the "Yes" option if the subordinate BareMetalComputeServices are use to run
         *     jobs that are not issued by the HTCondorComputeService, as resources will become oversubscribed).
         */
        DECLARE_PROPERTY_NAME(CONTACT_COMPUTE_SERVICES_FOR_AVAILABILITY);

    };
}// namespace wrench

#endif//WRENCH_HTCONDORCOMPUTESERVICEPROPERTY_H
