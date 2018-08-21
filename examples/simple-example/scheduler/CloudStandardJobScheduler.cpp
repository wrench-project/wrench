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
                                                  const std::vector<WorkflowTask *> &tasks) {

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

      for (auto task : tasks) {
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

        std::map<WorkflowFile *, StorageService *> file_locations;
        for (auto f : task->getInputFiles()) {
          file_locations.insert(std::make_pair(f, default_storage_service));
        }
        for (auto f : task->getOutputFiles()) {
          file_locations.insert(std::make_pair(f, default_storage_service));
        }

        // Decision making
        WorkflowJob *job = (WorkflowJob *) this->getJobManager()->createStandardJob(task, file_locations);
        unsigned long mim_num_cores = ((StandardJob *) (job))->getMinimumRequiredNumCores();

        if (sum_num_idle_cores < mim_num_cores) {
          try {
            std::string vm_host = cloud_service->createVM(mim_num_cores, task->getMemoryRequirement());

          } catch (WorkflowExecutionException &e) {
            // unable to create a new VM, tasks won't be scheduled in this iteration.
            return;
          }
        }
        this->getJobManager()->submitJob(job, cloud_service);
      }
      WRENCH_INFO("Done with scheduling tasks as standard jobs");
    }

}
