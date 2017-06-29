/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <map>

#include "wms/scheduler/pilot_job/CriticalPathScheduler.h"

namespace wrench {

    /**
     * @brief Schedule a
     *
     * @param scheduler: a scheduler implementation
     * @param workflow: a workflow to execute
     * @param job_manager: a job manager
     * @param compute_services: a set of compute services available to run jobs
     */
    void CriticalPathScheduler::schedule(Scheduler *scheduler,
                                         Workflow *workflow,
                                         JobManager *job_manager,
                                         const std::set<ComputeService *> &compute_services) {

      double flops = 0;
      std::set<WorkflowTask *> root_tasks;

      for (auto task : workflow->getTasks()) {
        flops = std::max(flops, task->getFlops() + this->getFlops(workflow, workflow->getTaskChildren(task)));

        if (workflow->getTaskParents(task).size() == 0) {
          root_tasks.insert(task);
        }
      }

      long max_parallel = this->getMaxParallelization(workflow, root_tasks);

      double total_flops = flops * (max_parallel <= compute_services.size() ?
                                    max_parallel : max_parallel - compute_services.size());

      scheduler->schedulePilotJobs(job_manager, workflow, total_flops, compute_services);
    }

    /**
     * @brief
     *
     * @param workflow: a pointer to the workflow object
     * @param tasks:
     *
     * @return
     */
    double CriticalPathScheduler::getFlops(Workflow *workflow, const std::vector<WorkflowTask *> tasks) {
      double max_flops = 0;

      for (auto task : tasks) {
        if (this->flopsMap.find(task) == this->flopsMap.end()) {
          double flops = task->getFlops() + getFlops(workflow, workflow->getTaskChildren(task));
          this->flopsMap[task] = flops;
        }
        max_flops = std::max(this->flopsMap[task], max_flops);
      }
      return max_flops;
    }

    /**
     * @brief Get the maximal number of jobs that can run in parallel
     *
     * @param workflow: a pointer to the workflow object
     * @param tasks: set of children tasks in a level
     *
     * @return the maximal number of jobs that can run in parallel
     */
    long CriticalPathScheduler::getMaxParallelization(Workflow *workflow, const std::set<WorkflowTask *> tasks) {
      long count = tasks.size();

      std::set<WorkflowTask *> children;

      for (auto task : tasks) {
        std::vector<WorkflowTask *> children_vector = workflow->getTaskChildren(task);
        std::copy(children_vector.begin(), children_vector.end(), std::inserter(children, children.end()));
      }
      if (children.size() > 0) {
        count = std::max(count, getMaxParallelization(workflow, children));
      }

      return count;
    }
}
