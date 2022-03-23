/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_JOB_FILLED_H
#define WRENCH_JOB_FILLED_H

#include <set>
#include <string>

#include "FailureCause.h"
#include "wrench/services/compute/ComputeService.h"

namespace wrench {

    class Job;

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
    * @brief A "job has been killed" failure cause
    */
    class JobKilled : public FailureCause {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        JobKilled(std::shared_ptr<Job> job);
        /***********************/
        /** \endcond           */
        /***********************/

        std::shared_ptr<Job> getJob();
        std::string toString() override;

    private:
        std::shared_ptr<Job> job;
    };


    /***********************/
    /** \endcond           */
    /***********************/
};// namespace wrench


#endif//WRENCH_JOB_FILLED_H
