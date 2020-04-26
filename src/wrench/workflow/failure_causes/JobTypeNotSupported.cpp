/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/workflow/failure_causes/FailureCause.h>
#include <wrench/workflow/failure_causes/FileAlreadyBeingCopied.h>
#include <wrench/workflow/failure_causes/JobTypeNotSupported.h>

#include "wrench/logging/TerminalOutput.h"
#include "wrench/workflow/failure_causes/FailureCause.h"
#include "wrench/workflow/WorkflowFile.h"
#include "wrench/workflow/job/WorkflowJob.h"
#include "wrench/services/storage/StorageService.h"
#include "wrench/services/compute/ComputeService.h"

WRENCH_LOG_CATEGORY(wrench_core_job_type_not_supported, "Log category for JobTypeNotSupported");

namespace wrench {

    /**
     * @brief Constructor
     * @param job: the job that wasn't supported
     * @param compute_service: the compute service that did not support it
     */
    JobTypeNotSupported::JobTypeNotSupported(WorkflowJob *job, std::shared_ptr<ComputeService> compute_service) {
        this->job = job;
        this->compute_service = compute_service;
    }

    /**
     * @brief Getter
     * @return the job
     */
    WorkflowJob *JobTypeNotSupported::getJob() {
        return this->job;
    }

    /**
     * @brief Getter
     * @return the compute service
     */
    std::shared_ptr<ComputeService> JobTypeNotSupported::getComputeService() {
        return this->compute_service;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string JobTypeNotSupported::toString() {
        return "Compute service " + this->compute_service->getName() + " on host " +
               this->compute_service->getHostname() + " does not support jobs of type " + this->job->getTypeAsString();
    }

}
