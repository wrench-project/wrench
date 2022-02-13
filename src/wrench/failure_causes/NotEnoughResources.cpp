/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/failure_causes/NotEnoughResources.h>

#include <wrench/logging/TerminalOutput.h>
#include <wrench/job/Job.h>
#include <wrench/services/Service.h>

WRENCH_LOG_CATEGORY(wrench_core_not_enough_resources, "Log category for NotEnoughResources");

namespace wrench {


    /**
     * @brief Constructor
     * @param job: the job that could not be executed (or nullptr if no job was involved)
     * @param service: the compute service that didn't have enough cores or ram
     */
    NotEnoughResources::NotEnoughResources(std::shared_ptr<Job> job, std::shared_ptr<Service> service) {
        this->job = job;
        this->service = service;
    }

    /**
     * @brief Getter
     * @return the job
     */
    std::shared_ptr<Job> NotEnoughResources::getJob() {
        return this->job;
    }

    /**
     * @brief Getter
     * @return the compute service
     */
    std::shared_ptr<Service> NotEnoughResources::getService() {
        return this->service;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string NotEnoughResources::toString() {
        std::string text_msg = "Compute service " + this->service->getName() + " on host " +
                               this->service->getHostname() + " does not have enough compute resources";
        if (job) {
            text_msg += " to support job " + job->getName();
        }
        return text_msg;
    }

}
