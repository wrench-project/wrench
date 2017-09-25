/**
 * Copyright (c) 2017. The WRENCH Team.
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
     * @param num_cores: the number of cores required by the pilot job
     * @param duration: duration of the pilot job, in seconds
     */
    PilotJob::PilotJob(Workflow *workflow, unsigned long num_cores, double duration) :
            WorkflowJob(WorkflowJob::PILOT, num_cores), state(PilotJob::State::NOT_SUBMITTED) {

      this->workflow = workflow;
      this->duration = duration;
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
     * @brief Get the compute service on which the pilot job is running
     * @return a compute service
     */
    ComputeService *PilotJob::getComputeService() {
      return this->compute_service.get();
    }

    /**
     * @brief Set the compute service on which the pilot job is running
     *        (this class will take case of memory deallocation)
     * @param cs: a compute service
     */
    void PilotJob::setComputeService(ComputeService *cs) {
      this->compute_service = std::unique_ptr<ComputeService>(cs);
    }

    /** @brief Get the job's duration
     *
     * @return the duration in seconds
     */
    double PilotJob::getDuration() {
      return this->duration;
    }

};