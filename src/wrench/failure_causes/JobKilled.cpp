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
#include <wrench/workflow/WorkflowFile.h>
#include <wrench/job/Job.h>
#include <wrench/services/storage/StorageService.h>
#include <wrench/services/Service.h>

WRENCH_LOG_CATEGORY(wrench_core_job_killed, "Log category for JobKilled");

namespace wrench {

    /**
    * @brief Constructor
    * @param job: the job that could not be executed
    * @param service: the compute service that didn't have enough cores
    */
    JobKilled::JobKilled(std::shared_ptr<Job> job, std::shared_ptr<Service> service) {
        this->job = job;
        this->service = service;
    }

    /**
     * @brief Getter
     * @return the job
     */
    std::shared_ptr<Job> JobKilled::getJob() {
        return this->job;
    }

    /**
     * @brief Getter
     * @return the compute service
     */
    std::shared_ptr<Service> JobKilled::getService() {
        return this->service;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string JobKilled::toString() {
        return "Job " + this->job->getName() + " on service " +
               this->service->getName() + " was killed (likely the service was stopped/terminated)";
    }


}
