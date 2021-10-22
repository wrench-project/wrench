/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_JOB_TIMEOUT_H
#define WRENCH_JOB_TIMEOUT_H

#include <set>
#include <string>

#include "FailureCause.h"

namespace wrench {

    class WorkflowJob;

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
    * @brief A "job has timed out" failure cause
    */
    class JobTimeout : public FailureCause {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        JobTimeout(std::shared_ptr<WorkflowJob> job);
        /***********************/
        /** \endcond           */
        /***********************/

        std::shared_ptr<WorkflowJob> getJob();
        std::string toString();

    private:
        std::shared_ptr<WorkflowJob> job;
    };


    /***********************/
    /** \endcond           */
    /***********************/
};


#endif //WRENCH_JOB_TIMEOUT_H
