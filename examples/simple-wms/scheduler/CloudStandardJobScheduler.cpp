/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include "CloudStandardJobScheduler.h"
#include <climits>
#include <numeric>

XBT_LOG_NEW_DEFAULT_CATEGORY(cloud_scheduler, "Log category for Cloud Scheduler");

namespace wrench {

    /**
     * @brief Schedule and run a set of ready tasks on available cloud resources
     *
     * @param compute_services: a set of compute services available to run jobs
     * @param tasks: a map of (ready) workflow tasks
     *
     * @throw std::runtime_error
     */
    void CloudStandardJobScheduler::scheduleTasks(const std::set<ComputeService *> &compute_services,
                                                  const std::map<std::string, std::vector<WorkflowTask *>> &tasks) {

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

      WRENCH_INFO("There are %ld ready tasks to schedule", tasks.size());

      for (auto itc : tasks) {
        //TODO add support to pilot jobs

        unsigned long sum_num_idle_cores = 0;

        // Check that it can run it right now in terms of idle cores
        try {
          std::vector<unsigned long> num_idle_cores = compute_service->getNumIdleCores();
          sum_num_idle_cores = (unsigned long) std::accumulate(num_idle_cores.begin(), num_idle_cores.end(), 0);
        } catch (WorkflowExecutionException &e) {
          // The service has some problem, forget it
          throw std::runtime_error("Unable to get the number of idle cores.");
        }

        // Decision making
        WorkflowJob *job = (WorkflowJob *) this->getJobManager()->createStandardJob(itc.second, {});
        unsigned long mim_num_cores = ((StandardJob *) (job))->getMinimumRequiredNumCores();
        double mim_mem = 0;
        for (auto task : itc.second) {
          mim_mem = std::max(mim_mem, task->getMemoryRequirement());
        }

        if (sum_num_idle_cores < mim_num_cores) {
          try {
            std::string pm_host = choosePMHostname();
            std::string vm_host = cloud_service->createVM(pm_host, mim_num_cores, mim_mem);

            if (not vm_host.empty()) {
              this->vm_list[pm_host].push_back(vm_host);
            }

          } catch (WorkflowExecutionException &e) {
            // unable to create a new VM, tasks won't be scheduled in this iteration.
            return;
          }
        }
        this->getJobManager()->submitJob(job, cloud_service);
      }
      WRENCH_INFO("Done with scheduling tasks as standard jobs");
    }

    /**
     * @brief Select a physical host (PM) with the least number of VMs.
     *
     * @return a physical hostname
     */
    std::string CloudStandardJobScheduler::choosePMHostname() {

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
}
