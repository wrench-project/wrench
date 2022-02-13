/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/job/PilotJob.h>

#include <utility>

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param workflow: a workflow
     */
    PilotJob::PilotJob(std::shared_ptr<JobManager> job_manager) :
            Job("", std::move(job_manager)), state(PilotJob::State::NOT_SUBMITTED) {

      this->name = "pilot_job_" + std::to_string(Job::getNewUniqueNumber());
    }

    /**
     * @brief Get the state of the pilot job
     * @return the state
     */
    PilotJob::State PilotJob::getState() {
      return this->state;
    }

    /**
     * @brief Get the compute service provided by the (running) pilot job
     * @return a compute service
     */
    std::shared_ptr<BareMetalComputeService> PilotJob::getComputeService() {
      return this->compute_service;
    }

    /**
     * @brief Set the compute service that runs on the pilot job
     * @param cs: a compute service
     */
    void PilotJob::setComputeService(std::shared_ptr<BareMetalComputeService> cs) {
      this->compute_service = std::move(cs);
    }


}