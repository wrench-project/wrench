/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <xbt.h>
#include <set>
#include <numeric>

#include "MinMinStandardJobScheduler.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(minmin_scheduler, "Log category for Min-Min Scheduler");

namespace wrench {

    /**
     * @brief Compare the number of flops between two lists of workflow tasks
     *
     * @param lhs: a pair of a task ID and a list of workflow tasks
     * @param rhs: a pair of a task ID and a list of workflow tasks
     *
     * @return whether the number of flops from the left-hand-side workflow tasks is smaller
     */
    bool MinMinStandardJobScheduler::MinMinComparator::operator()(WorkflowTask *&lhs,
                                                                  WorkflowTask *&rhs) {
      return lhs->getFlops() < rhs->getFlops();
    }

    /**
     * @brief Schedule and run a set of ready tasks on available compute resources
     *
     * @param job_manager: a job manager
     * @param ready_tasks: a vector of ready workflow tasks
     * @param compute_services: a set of compute services available to run jobs
     */
    void MinMinStandardJobScheduler::scheduleTasks(const std::set<ComputeService *> &compute_services,
                                                   const std::vector<WorkflowTask *> &ready_tasks) {

      WRENCH_INFO("There are %ld ready tasks to schedule", ready_tasks.size());

      // Sorting tasks
      std::vector<WorkflowTask *> min_vector(ready_tasks.begin(),
                                                                                  ready_tasks.end());
      std::sort(min_vector.begin(), min_vector.end(), MinMinComparator());

      for (auto task : min_vector) {
        bool successfully_scheduled = false;

        // Second: attempt to run the task on a compute resource
        WRENCH_INFO("Trying to submit task '%s' to a standard compute service...", task->getID().c_str());

        for (auto cs : compute_services) {
          WRENCH_INFO("Asking compute service %s if it can run this standard job...", cs->getName().c_str());

          // Check that the compute service could in principle run this job
          if ((not cs->isUp()) || (not cs->supportsStandardJobs())) {
            continue;
          }

          // Get the number of currently idle cores
          unsigned long sum_num_idle_cores;
          try {
            std::vector<unsigned long> num_idle_cores = cs->getNumIdleCores();
            sum_num_idle_cores = (unsigned long) std::accumulate(num_idle_cores.begin(), num_idle_cores.end(), 0);

          } catch (WorkflowExecutionException &e) {
            // The service has some problem, forget it
            continue;
          }

          // Decision making
          if (sum_num_idle_cores <= 0) {
            continue;
          }

          std::map<WorkflowFile *, StorageService *> file_locations;
          for (auto f : task->getInputFiles()) {
            file_locations.insert(std::make_pair(f, default_storage_service));
          }
          for (auto f : task->getOutputFiles()) {
            file_locations.insert(std::make_pair(f, default_storage_service));
          }

          WRENCH_INFO("Submitting task %s for execution as a standard job", task->getID().c_str());
          WorkflowJob *job = (WorkflowJob *) job_manager->createStandardJob(task, file_locations);
          job_manager->submitJob(job, cs);
          successfully_scheduled = true;
          break;
        }
        if (not successfully_scheduled) {
          WRENCH_INFO("no dice");
          break;
        }
      }
    }
}
