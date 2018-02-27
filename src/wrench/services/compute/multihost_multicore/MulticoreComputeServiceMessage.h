/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_MULTICORECOMPUTESERVICEMESSAGE_H
#define WRENCH_MULTICORECOMPUTESERVICEMESSAGE_H


#include <vector>

#include "wrench/services/compute/ComputeServiceMessage.h"

namespace wrench {

    class WorkflowTask;

    class WorkUnitMultiCoreExecutor;

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Top-level MulticoreComputeServiceMessage class
     */
    class MulticoreComputeServiceMessage : public ComputeServiceMessage {
    protected:
        MulticoreComputeServiceMessage(std::string name, double payload);
    };

    /**
     * @brief MulticoreComputeServiceNotEnoughCoresMessage class
     */
    class MulticoreComputeServiceNotEnoughCoresMessage : public MulticoreComputeServiceMessage {
    public:
        MulticoreComputeServiceNotEnoughCoresMessage(WorkflowJob *, ComputeService *, double payload);

        /** @brief The job that couldn't run due to not enough cores */
        WorkflowJob *job;
        /** @brief The compute service on which there weren't enough cores */
        ComputeService *compute_service;
    };


    /***********************/
    /** \endcond           */
    /***********************/
};

#endif //WRENCH_MULTICORECOMPUTESERVICEMESSAGE_H
