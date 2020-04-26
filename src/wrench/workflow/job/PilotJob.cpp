/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/workflow/job/PilotJob.h"

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param workflow: a workflow
     */
    PilotJob::PilotJob(Workflow *workflow) :
            WorkflowJob(WorkflowJob::PILOT), state(PilotJob::State::NOT_SUBMITTED) {

      this->workflow = workflow;
      this->name = "pilot_job_" + std::to_string(WorkflowJob::getNewUniqueNumber());
    }

    /**
     * @brief Get the state of the pilot job
     * @return the state
     */
    PilotJob::State PilotJob::getState() {
      return this->state;
    }

    /**
     * @brief Get the pilot job priority value
     * @return the pilot job priority value
     */
    unsigned long PilotJob::getPriority() {
      // TODO: implement the function
      return 0;
    }

    /**
     * @brief Get the compute service provided by the (running) pilot job
     * @return a compute service
     */
    std::shared_ptr<ComputeService> PilotJob::getComputeService() {
      return this->compute_service;
    }

    /**
     * @brief Set the compute service on which the pilot job is running
     * @param cs: a compute service
     */
    void PilotJob::setComputeService(std::shared_ptr<ComputeService> cs) {
      this->compute_service = cs;
    }


}