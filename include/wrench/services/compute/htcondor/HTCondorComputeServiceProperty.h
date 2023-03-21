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
         * when running non-grid jobs) to find out about their currently available resources (value "false"), or instead or instead
         * uses the "out of simulation time" method to query these services about their currently available resources (value "true");
         * "true" is the default value since it reduces simulation time. The caveat that it no longer simulates the network load and overhead
         * of all "tell me how many free resources you have right now?" and "this is what I got!" control messages.
         */
        DECLARE_PROPERTY_NAME(INSTANT_RESOURCE_AVAILABILITIES);
    };
}// namespace wrench

#endif//WRENCH_HTCONDORCOMPUTESERVICEPROPERTY_H
