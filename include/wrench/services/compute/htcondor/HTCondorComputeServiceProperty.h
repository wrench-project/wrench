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
        /** @brief Overhead, in seconds, of the HTCondor Negotiator, which is invoked each time a new job is submitted or
         *  a running job completes and there are still pending jobs **/
        DECLARE_PROPERTY_NAME(NEGOTIATOR_OVERHEAD);

        /** @brief Overhead (in seconds) between condor and a batch_standard_and_pilot_jobs compute service for the start of execution of grid-universe jobs
         */
        DECLARE_PROPERTY_NAME(GRID_PRE_EXECUTION_DELAY);

        /** @brief Overhead (in seconds) between condor and a batch_standard_and_pilot_jobs compute service for the completion of execution of grid-universe jobs
         */
        DECLARE_PROPERTY_NAME(GRID_POST_EXECUTION_DELAY);

        /** @brief Overhead (in seconds) between condor and a bare-metal compute service for the start of execution of non-grid-universe jobs
         */
        DECLARE_PROPERTY_NAME(NON_GRID_PRE_EXECUTION_DELAY);

        /** @brief Overhead (in seconds) between condor and a bare-metal compute for the completion of execution of non-grid-universe jobs
         */
        DECLARE_PROPERTY_NAME(NON_GRID_POST_EXECUTION_DELAY);
    };
}// namespace wrench

#endif//WRENCH_HTCONDORCOMPUTESERVICEPROPERTY_H
