/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/failure_causes/JobKilled.h>

#include "wrench/logging/TerminalOutput.h"
#include "wrench/workflow/WorkflowFile.h"
#include "wrench/job/WorkflowJob.h"
#include "wrench/services/storage/StorageService.h"
#include "wrench/services/compute/ComputeService.h"

WRENCH_LOG_CATEGORY(wrench_core_job_killed, "Log category for JobKilled");

namespace wrench {

    /**
    * @brief Constructor
    * @param job: the job that could not be executed
    * @param compute_service: the compute service that didn't have enough cores
    */
    JobKilled::JobKilled(std::shared_ptr<WorkflowJob> job, std::shared_ptr<ComputeService> compute_service) {
        this->job = job;
        this->compute_service = compute_service;
    }

    /**
     * @brief Getter
     * @return the job
     */
    std::shared_ptr<WorkflowJob> JobKilled::getJob() {
        return this->job;
    }

    /**
     * @brief Getter
     * @return the compute service
     */
    std::shared_ptr<ComputeService> JobKilled::getComputeService() {
        return this->compute_service;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string JobKilled::toString() {
        return "Job " + this->job->getName() + " on service " +
               this->compute_service->getName() + " was killed (likely the service was stopped/terminated)";
    }


}
