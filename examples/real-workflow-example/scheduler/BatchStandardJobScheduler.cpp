/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include "BatchStandardJobScheduler.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(batch_scheduler, "Log category for Batch Scheduler");

namespace wrench {


    /**
     * @brief Schedule and run a set of ready tasks on the batch service
     *
     * @param compute_services: a set of compute services available to run jobs
     * @param ready_tasks: a set of (ready) workflow tasks
     *
     * @throw std::runtime_error
     */
    void BatchStandardJobScheduler::scheduleTasks(const std::set<std::shared_ptr<ComputeService>> &compute_services,
                                                  const std::vector<WorkflowTask *> &tasks) {

      // Check that the right compute_services is passed
      if (compute_services.size() != 1) {
        throw std::runtime_error("This example Batch Scheduler requires a single compute service");
      }

      auto compute_service = *compute_services.begin();
      auto batch_service = std::dynamic_pointer_cast<BatchComputeService>(compute_service);

      if (not(batch_service)) {
        throw std::runtime_error("This example Batch Scheduler can only handle a batch service");
      }

      WRENCH_INFO("There are %ld ready tasks to schedule", tasks.size());

      for (auto task : tasks) {
        //TODO add support to pilot jobs

        std::map<WorkflowFile *, std::shared_ptr<FileLocation>> file_locations;
        for (auto f : task->getInputFiles()) {
          file_locations[f] = wrench::FileLocation::LOCATION(default_storage_service);
        }
        for (auto f : task->getOutputFiles()) {
          file_locations[f] = wrench::FileLocation::LOCATION(default_storage_service);
        }

        auto job = this->getJobManager()->createStandardJob(task, file_locations);
        std::map<std::string, std::string> batch_job_args;
        batch_job_args["-N"] = "1";
        batch_job_args["-t"] = "2000000"; //time in minutes
        batch_job_args["-c"] = "1"; //number of cores per node
        this->getJobManager()->submitJob(job, batch_service, batch_job_args);
      }
      WRENCH_INFO("Done with scheduling tasks as standard jobs");
    }

}
