/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/failure_causes/NotEnoughResourcesForJob.h>

#include <wrench/logging/TerminalOutput.h>
#include <wrench/job/Job.h>
#include <wrench/services/Service.h>

#include <utility>

WRENCH_LOG_CATEGORY(wrench_core_not_enough_resources_for_job, "Log category for NotEnoughResourcesForJob");

namespace wrench {


    /**
     * @brief Constructor
     * @param job: the job that could not be executed (or nullptr if no job was involved)
     * @param service: the compute service that didn't have enough cores or ram
     */
    NotEnoughResourcesForJob::NotEnoughResourcesForJob(std::shared_ptr<Job> job, std::shared_ptr<Service> service) {
        this->job = std::move(job);
        this->service = std::move(service);
    }

    /**
     * @brief Getter
     * @return the job
     */
    std::shared_ptr<Job> NotEnoughResourcesForJob::getJob() {
        return this->job;
    }

    /**
     * @brief Getter
     * @return the compute service
     */
    std::shared_ptr<Service> NotEnoughResourcesForJob::getService() {
        return this->service;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string NotEnoughResourcesForJob::toString() {
        std::string text_msg = "Compute service " + this->service->getName() + " on host " +
                               this->service->getHostname() + " does not have enough compute resources";
        if (job) {
            text_msg += " to support job " + job->getName();
        }
        return text_msg;
    }

}// namespace wrench
