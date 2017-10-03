/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include "CloudScheduler.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(cloud_scheduler, "Log category for Cloud Scheduler");

namespace wrench {

    static unsigned long VM_ID = 1;

    /**
     * @brief Constructor
     *
     * @param cloud_service: a pointer to a cloud service
     * @param execution_hosts: list of execution hosts to run VMs
     * @param simulation: a pointer to the simulation object
     *
     * @throw std::runtime_error
     */
    CloudScheduler::CloudScheduler(ComputeService *cloud_service, std::vector<std::string> &execution_hosts,
                                   Simulation *simulation)
            : execution_hosts(execution_hosts), simulation(simulation) {

      if (typeid(cloud_service) == typeid(CloudService)) {
        throw std::runtime_error("The provided cloud service is not a CloudService object.");
      }
      this->cloud_service = cloud_service;
    }

    /**
     * @brief Schedule and run a set of ready tasks on available cloud resources
     *
     * @param job_manager: a job manager
     * @param ready_tasks: a map of ready workflow tasks
     * @param compute_services: a set of compute services available to run jobs
     *
     * @throw std::runtime_error
     */
    void CloudScheduler::scheduleTasks(JobManager *job_manager,
                                       std::map<std::string, std::vector<WorkflowTask *>> ready_tasks,
                                       const std::set<ComputeService *> &compute_services) {

      if (compute_services.find(cloud_service) == compute_services.end()) {
        throw std::runtime_error("The provided cloud service is not listed as a compute service.");
      }
      auto *cs = (CloudService *) this->cloud_service;

      WRENCH_INFO("There are %ld ready tasks to schedule", ready_tasks.size());
      int scheduled = 0;

      for (auto itc : ready_tasks) {

        //TODO add support to pilot jobs

        unsigned long num_idle_cores = 0;

        // Check that it can run it right now in terms of idle cores
        try {
          num_idle_cores = cs->getNumIdleCores();
        } catch (WorkflowExecutionException &e) {
          // The service has some problem, forget it
          // TODO launch error
        }

        // Decision making
        WorkflowJob *job = (WorkflowJob *) job_manager->createStandardJob(itc.second, {});
        if ((num_idle_cores - scheduled) <= 0) {
          try {
            std::string vm_host = "vm" + std::to_string(VM_ID++) + "_" + execution_hosts[0];

            cs->createVM(execution_hosts[0], vm_host, job->getNumCores());

          } catch (WorkflowExecutionException &e) {
            //TODO launch error
          }
        }
        job_manager->submitJob(job, cs);
        scheduled++;
      }
      WRENCH_INFO("Done with scheduling tasks as standard jobs");
    }
}
