/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/workflow/failure_causes/JobTimeout.h>

#include "wrench/logging/TerminalOutput.h"
#include "wrench/workflow/job/WorkflowJob.h"

WRENCH_LOG_CATEGORY(wrench_core_job_timeout, "Log category for JobTimeout");

namespace wrench {

    /**
    * @brief Constructor
    *
    * @param job: the job that has timed out
    */
    JobTimeout::JobTimeout(WorkflowJob *job) {
        this->job = job;
    }


    /**
     * @brief Getter
     * @return the job
     */
    WorkflowJob *JobTimeout::getJob() {
        return this->job;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string JobTimeout::toString() {
        return std::string("Job has timed out - likely not enough time was requested from a (batch-scheduled?) compute service");
    }

}
