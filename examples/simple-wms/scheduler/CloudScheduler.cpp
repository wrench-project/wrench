/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include "CloudScheduler.h"
#include <climits>
#include <numeric>

XBT_LOG_NEW_DEFAULT_CATEGORY(cloud_scheduler, "Log category for Cloud Scheduler");

namespace wrench {

    static unsigned long VM_ID = 1;

    /**
     * @brief Constructor
     *
     */
    CloudScheduler::CloudScheduler() {

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

      // Check that the right compute_services is passed
      if (compute_services.size() != 1) {
        throw std::runtime_error("This example Cloud Scheduler requires a single compute service");
      }

      ComputeService *compute_service = *compute_services.begin();
      CloudService *cloud_service;

      if (not(cloud_service = dynamic_cast<CloudService *>(compute_service))) {
        throw std::runtime_error("This example Cloud Scheduler can only handle a cloud service");
      }


      // obtain list of execution hosts, if not already done
      if (this->execution_hosts.empty()) {
        this->execution_hosts = cloud_service->getExecutionHosts();
      }

      WRENCH_INFO("There are %ld ready tasks to schedule", ready_tasks.size());
      long scheduled = 0;

      for (auto itc : ready_tasks) {
        //TODO add support to pilot jobs

        unsigned long sum_num_idle_cores = 0;

        // Check that it can run it right now in terms of idle cores
        try {
          std::vector<unsigned long> num_idle_cores = compute_service->getNumIdleCores();
          sum_num_idle_cores = (unsigned long)std::accumulate(num_idle_cores.begin(), num_idle_cores.end(), 0);
        } catch (WorkflowExecutionException &e) {
          // The service has some problem, forget it
          throw std::runtime_error("Unable to get the number of idle cores.");
        }

        // Decision making
        WorkflowJob *job = (WorkflowJob *) job_manager->createStandardJob(itc.second, {});
        if (sum_num_idle_cores - scheduled <= 0) {
          try {
            std::string pm_host = choosePMHostname();
            std::string vm_host = "vm" + std::to_string(VM_ID++) + "_" + pm_host;

            // TODO: provide proper VM RAM requests
            if (cloud_service->createVM(pm_host, vm_host, ((StandardJob *) (job))->getMinimumRequiredNumCores(), 1000)) {
              this->vm_list[pm_host].push_back(vm_host);
            }

          } catch (WorkflowExecutionException &e) {
            // unable to create a new VM, tasks won't be scheduled in this iteration.
            return;
          }
        }
        job_manager->submitJob(job, cloud_service);
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

      std::pair<std::string, unsigned long> min_pm("", ComputeService::ALL_CORES);

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

    /**
     * @brief Schedule and run pilot jobs
     *
     * @param job_manager: a job manager
     * @param workflow: a workflow
     * @param pilot_job_duration: a long pilot jobs should last
     * @param flops: the number of flops that the pilot jobs should be able to do (assuming it constantly uses the CPU) before terminating
     * @param compute_services: a set of compute services available to run jobs
     */
    void CloudScheduler::schedulePilotJobs(JobManager *job_manager,
                                           Workflow *workflow,
                                           double pilot_job_duration,
                                           const std::set<ComputeService *> &compute_services) {
      throw std::runtime_error("CloudScheduler::schedulerPilotJobs(): Not implemented (yet) - don't use pilot jobs for now");
    }
}
