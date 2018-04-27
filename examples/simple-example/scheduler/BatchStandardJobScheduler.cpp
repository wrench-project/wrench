/**
 * Copyright (c) 2017. The WRENCH Team.
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
     * @param ready_tasks: a map of (ready) workflow tasks
     *
     * @throw std::runtime_error
     */
    void BatchStandardJobScheduler::scheduleTasks(const std::set<ComputeService *> &compute_services,
                                                  const std::map<std::string, std::vector<WorkflowTask *>> &tasks) {

      // Check that the right compute_services is passed
      if (compute_services.size() != 1) {
        throw std::runtime_error("This example Batch Scheduler requires a single compute service");
      }

      ComputeService *compute_service = *compute_services.begin();
      BatchService *batch_service;
      if (not(batch_service = dynamic_cast<BatchService *>(compute_service))) {
        throw std::runtime_error("This example Batch Scheduler can only handle a batch service");
      }

      WRENCH_INFO("There are %ld ready tasks to schedule", tasks.size());

      for (auto itc : tasks) {
        //TODO add support to pilot jobs

        WorkflowJob *job = (WorkflowJob *) this->getJobManager()->createStandardJob(itc.second, {});
        std::map<std::string, std::string> batch_job_args;
        batch_job_args["-N"] = "1";
        batch_job_args["-t"] = "2000000"; //time in minutes
        batch_job_args["-c"] = "1"; //number of cores per node
        this->getJobManager()->submitJob(job, batch_service, batch_job_args);
      }
      WRENCH_INFO("Done with scheduling tasks as standard jobs");
    }


}
