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

        /** @brief Whether the condor service supports grid jobs that are offloaded to batch service **/
        DECLARE_PROPERTY_NAME(SUPPORTS_GRID_UNIVERSE);

        /** @brief Overhead delay in seconds between condor and slurm for the start of execution **/
        DECLARE_PROPERTY_NAME(GRID_PRE_EXECUTION_DELAY);

        /** @brief Overhead delay in seconds between condor and slurm for the completion of execution **/
        DECLARE_PROPERTY_NAME(GRID_POST_EXECUTION_DELAY);
    };
}

#endif //WRENCH_HTCONDORCOMPUTESERVICEPROPERTY_H
