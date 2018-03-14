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
     * @param num_hosts: the number of hosts
     * @param num_cores_per_host: the number of cores per host
     * @param ram_per_host: the number of bytes of RAM per host
     * @param duration: the pilot job's duration in seconds
     */
    PilotJob::PilotJob(Workflow *workflow, unsigned long num_hosts,
                       unsigned long num_cores_per_host, double ram_per_host, double duration) :
            WorkflowJob(WorkflowJob::PILOT), state(PilotJob::State::NOT_SUBMITTED) {

      this->workflow = workflow;
      this->name = "pilot_job_" + std::to_string(WorkflowJob::getNewUniqueNumber());

      this->ram_per_host = ram_per_host;
      this->num_hosts = num_hosts;
      this->num_cores_per_host = num_cores_per_host;
      this->duration = duration;
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
    void PilotJob::setComputeService(std::shared_ptr<ComputeService> cs) {
      this->compute_service = cs;
    }

    /**
     * @brief Get the pilot job's number of hosts
     * @return a number of hosts
     */
    unsigned long PilotJob::getNumHosts() const {
      return num_hosts;
    }

    /**
     * @brief Get the pilot job's number of cores per host
     * @return a number of cores
     */
    unsigned long PilotJob::getNumCoresPerHost() const {
      return num_cores_per_host;
    }

    /**
     * @brief Get the pilot job's memory requirement per host
     * @return a number of bytes
     */
    double PilotJob::getMemoryPerHost() const {
      return ram_per_host;
    }

    /**
     * @brief Get the pilot job's duration
     * @return a number of seconds
     */
    double PilotJob::getDuration() const {
      return duration;
    }

};