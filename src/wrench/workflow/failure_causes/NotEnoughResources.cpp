/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/workflow/failure_causes/NotEnoughResources.h>

#include "wrench/logging/TerminalOutput.h"
#include "wrench/workflow/job/WorkflowJob.h"
#include "wrench/services/compute/ComputeService.h"

WRENCH_LOG_CATEGORY(wrench_core_not_enough_resources, "Log category for NotEnoughResources");

namespace wrench {


    /**
     * @brief Constructor
     * @param job: the job that could not be executed (or nullptr if no job was involved)
     * @param compute_service: the compute service that didn't have enough cores or ram
     */
    NotEnoughResources::NotEnoughResources(WorkflowJob *job, std::shared_ptr<ComputeService> compute_service) {
        this->job = job;
        this->compute_service = compute_service;
    }

    /**
     * @brief Getter
     * @return the job
     */
    WorkflowJob *NotEnoughResources::getJob() {
        return this->job;
    }

    /**
     * @brief Getter
     * @return the compute service
     */
    std::shared_ptr<ComputeService> NotEnoughResources::getComputeService() {
        return this->compute_service;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string NotEnoughResources::toString() {
        std::string text_msg = "Compute service " + this->compute_service->getName() + " on host " +
                               this->compute_service->getHostname() + " does not have enough compute resources";
        if (job) {
            text_msg += " to support job " + job->getName();
        }
        return text_msg;
    }

};
