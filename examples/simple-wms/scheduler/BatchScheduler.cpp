/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include "BatchScheduler.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(batch_scheduler, "Log category for Batch Scheduler");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param batch_service: a pointer to a batch service
     * @param execution_hosts: list of physical execution hosts to run the batch job
     * @param simulation: a pointer to the simulation object
     *
     * @throw std::runtime_error
     */
    BatchScheduler::BatchScheduler(ComputeService *batch_service, Simulation *simulation) : simulation(simulation) {

      if (typeid(batch_service) == typeid(BatchService)) {
        throw std::runtime_error("The provided batch service is not a BatchService object.");
      }
      this->batch_service = batch_service;
    }

    /**
     * @brief Schedule and run a set of ready tasks on the batch service
     *
     * @param job_manager: a job manager
     * @param ready_tasks: a map of ready workflow tasks
     * @param compute_services: a set of compute services available to run jobs
     *
     * @throw std::runtime_error
     */
    void BatchScheduler::scheduleTasks(JobManager *job_manager,
                                       std::map<std::string, std::vector<WorkflowTask *>> ready_tasks,
                                       const std::set<ComputeService *> &compute_services) {

      if (compute_services.find(batch_service) == compute_services.end()) {
        throw std::runtime_error("The default batch service is not listed as a compute service.");
      }
      auto *cs = (BatchService *) this->batch_service;

      WRENCH_INFO("There are %ld ready tasks to schedule", ready_tasks.size());

      for (auto itc : ready_tasks) {
        //TODO add support to pilot jobs

        WorkflowJob *job = (WorkflowJob *) job_manager->createStandardJob(itc.second, {});
        std::map<std::string, std::string> batch_job_args;
        batch_job_args["-N"] = "1";
        batch_job_args["-t"] = "2000000"; //time in minutes
        batch_job_args["-c"] = "1"; //number of cores per node
        job_manager->submitJob(job, cs, batch_job_args);
      }
      WRENCH_INFO("Done with scheduling tasks as standard jobs");
    }
}
