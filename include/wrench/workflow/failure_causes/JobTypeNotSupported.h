/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_JOB_TYPE_NOT_SUPPORTED_H
#define WRENCH_JOB_TYPE_NOT_SUPPORTED_H

#include <set>
#include <string>

#include "wrench/workflow/failure_causes/FailureCause.h"
#include "wrench/services/compute/ComputeService.h"

namespace wrench {

    class WorkflowJob;

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
     * @brief A "compute service does not support requested job type" failure cause
     */
    class JobTypeNotSupported : public FailureCause {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        JobTypeNotSupported(std::shared_ptr<WorkflowJob> job, std::shared_ptr<ComputeService>  compute_service);
        /***********************/
        /** \endcond           */
        /***********************/

        std::shared_ptr<WorkflowJob> getJob();
        std::shared_ptr<ComputeService>  getComputeService();
        std::string toString();

    private:
        std::shared_ptr<WorkflowJob> job;
        std::shared_ptr<ComputeService>  compute_service;
    };


    /***********************/
    /** \endcond           */
    /***********************/
};


#endif //WRENCH_JOB_TYPE_NOT_SUPPORTED_H
