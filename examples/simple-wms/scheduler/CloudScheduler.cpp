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
#include <climits>

XBT_LOG_NEW_DEFAULT_CATEGORY(cloud_scheduler, "Log category for Cloud Scheduler");

namespace wrench {

    static unsigned long VM_ID = 1;

    /**
     * @brief Constructor
     *
     * @param cloud_service: a pointer to a cloud service
     * @param execution_hosts: list of physical execution hosts to run VMs
     * @param simulation: a pointer to the simulation object
     *
     * @throw std::runtime_error
     */
    CloudScheduler::CloudScheduler(ComputeService *cloud_service, std::vector<std::string> &execution_hosts,
                                   Simulation *simulation) : simulation(simulation) {

      if (typeid(cloud_service) == typeid(CloudService)) {
        throw std::runtime_error("The provided cloud service is not a CloudService object.");
      }
      if (execution_hosts.empty()) {
        throw std::runtime_error("At least one execution host should be provided");
      }
      this->execution_hosts = execution_hosts;
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
        throw std::runtime_error("The default cloud service is not listed as a compute service.");
      }
      auto *cs = (CloudService *) this->cloud_service;

      WRENCH_INFO("There are %ld ready tasks to schedule", ready_tasks.size());
      long scheduled = 0;

      for (auto itc : ready_tasks) {
        //TODO add support to pilot jobs

        long num_idle_cores = 0;

        // Check that it can run it right now in terms of idle cores
        try {
          num_idle_cores = cs->getNumIdleCores();
        } catch (WorkflowExecutionException &e) {
          // The service has some problem, forget it
          throw std::runtime_error("Unable to get the number of idle cores.");
        }

        // Decision making
        WorkflowJob *job = (WorkflowJob *) job_manager->createStandardJob(itc.second, {});
        if (num_idle_cores - scheduled <= 0) {
          try {
            std::string pm_host = choosePMHostname();
            std::string vm_host = "vm" + std::to_string(VM_ID++) + "_" + pm_host;

            if (cs->createVM(pm_host, vm_host, ((StandardJob *) (job))->getMinimumRequiredNumCores())) {
              this->vm_list[pm_host].push_back(vm_host);
            }

          } catch (WorkflowExecutionException &e) {
            // unable to create a new VM, tasks won't be scheduled in this iteration.
            return;
          }
        }
        job_manager->submitJob(job, cs);
        scheduled++;
      }
      WRENCH_INFO("Done with scheduling tasks as standard jobs");
    }

    /**
     * @brief Select a physical host (PM) with the least number of VMs.
     *
     * @return a physical hostname
     */
    std::string CloudScheduler::choosePMHostname() {

      std::pair<std::string, unsigned long> min_pm("", ULONG_MAX);

      for (auto &host : this->execution_hosts) {
        auto entry = this->vm_list.find(host);

        if (entry == this->vm_list.end()) {
          return host;
        }
        if (entry->second.size() < min_pm.second) {
          min_pm.first = entry->first;
          min_pm.second = entry->second.size();
        }
      }

      return min_pm.first;
    }
}
