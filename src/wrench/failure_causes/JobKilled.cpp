/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/failure_causes/JobKilled.h>

#include <wrench/logging/TerminalOutput.h>
#include <wrench/data_file/DataFile.h>
#include <wrench/job/Job.h>
#include <wrench/services/storage/StorageService.h>
#include <wrench/services/Service.h>

WRENCH_LOG_CATEGORY(wrench_core_job_killed, "Log category for JobKilled");

namespace wrench {

    /**
    * @brief Constructor
    * @param job: the job that was killed
    */
    JobKilled::JobKilled(std::shared_ptr<Job> job) {
        this->job = job;
    }

    /**
     * @brief Getter
     * @return the job
     */
    std::shared_ptr<Job> JobKilled::getJob() {
        return this->job;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string JobKilled::toString() {
        std::string message = "Job " + this->job->getName() + " ";
        if (this->job->getParentComputeService()) {
            message += "on service " + this->job->getParentComputeService()->getName() + " ";
        }
        message += "was killed";
        return message;
    }


}// namespace wrench
