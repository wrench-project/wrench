/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_NOT_ENOUGH_RESOURCES_FOR_JOBS_H
#define WRENCH_NOT_ENOUGH_RESOURCES_FOR_JOBS_H

#include <set>
#include <string>

#include "wrench/failure_causes/FailureCause.h"
#include "wrench/services/compute/ComputeService.h"

namespace wrench {

    class Job;

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/


    /**
     * @brief A "compute service doesn't have enough resources" failure cause
     */
    class NotEnoughResourcesForJob : public FailureCause {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        NotEnoughResourcesForJob(std::shared_ptr<Job> job, std::shared_ptr<Service> service);
        /***********************/
        /** \endcond           */
        /***********************/

        std::shared_ptr<Job> getJob();
        std::shared_ptr<Service> getService();
        std::string toString() override;

    private:
        std::shared_ptr<Job> job;
        std::shared_ptr<Service> service;
    };


    /***********************/
    /** \endcond           */
    /***********************/
}// namespace wrench


#endif//WRENCH_NOT_ENOUGH_RESOURCES_FOR_JOB_H
