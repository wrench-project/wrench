/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <map>
#include <xbt/log.h>

#include "CriticalPathPilotJobScheduler.h"

WRENCH_LOG_CATEGORY(critical_path_scheduler, "Log category for Critical Path Scheduler");

namespace wrench {


    /**
     * @brief Schedule a pilot job for the length of the critical path
     *
     * @param scheduler: a scheduler implementation
     * @param workflow: a workflow to execute
     * @param job_manager: a job manager
     * @param compute_services: a set of compute services available to run jobs
     */
    void CriticalPathPilotJobScheduler::schedulePilotJobs(const std::set<std::shared_ptr<ComputeService>> &compute_services) {

    double flops = 0;
      std::set<WorkflowTask *> root_tasks;

      for (auto task : workflow->getTasks()) {
        flops = (std::max)(flops, task->getFlops() + this->getFlops(workflow, workflow->getTaskChildren(task)));

        if (workflow->getTaskParents(task).size() == 0) {
          root_tasks.insert(task);
        }
      }

      unsigned long max_parallel = this->getMaxParallelization(workflow, root_tasks);

      double total_flops = flops * (max_parallel <= compute_services.size() ?
                                    max_parallel : max_parallel - compute_services.size());

      // If there is always a pilot job in the system, do nothing
      if ((this->getJobManager()->getNumRunningPilotJobs() > 0)) {
        WRENCH_INFO("There is already a pilot job in the system...");
        return;
      }

      // Submit a pilot job to the first compute service that can support it
      std::shared_ptr<ComputeService> target_service = nullptr;
      for (auto cs : compute_services) {
        if (cs->isUp() && cs->supportsPilotJobs()) {
          target_service = cs;
          break;
        }
      }
      if (target_service == nullptr) {
        WRENCH_INFO("No compute service supports pilot jobs");
        return;
      }

      // Submit a pilot job
      // TODO: Henri added the factor 2 below because running with the "one task" example,
      // TODO: the pilot job was just a bit too short and the WMS was in an infinite loop
      // TODO: (which is an annoying but simulation-valid behavior)
      double pilot_job_duration = 2.0 * flops / target_service->getCoreFlopRate()[0];
      WRENCH_INFO("Submitting a pilot job (1 host, 1 core, %lf seconds)", pilot_job_duration);

      double pilot_job_duration_in_minutes = ceil(pilot_job_duration/60.0);
      //TODO: For now we are asking for a pilot job that requires no RAM
      auto job = this->getJobManager()->createPilotJob();
      this->getJobManager()->submitJob(job, target_service, {{"-N","1"},{"-c","1"},{"-t",std::to_string(pilot_job_duration_in_minutes)}});

    }

    /**
     * @brief Get the total number of flops recursively of the critical path for a given task
     *
     * @param workflow: a pointer to the workflow object
     * @param tasks: a vector of children tasks
     *
     * @return
     */
    double CriticalPathPilotJobScheduler::getFlops(Workflow *workflow, const std::vector<WorkflowTask *> &tasks) {
      double max_flops = 0;

      for (auto task : tasks) {
        if (this->flopsMap.find(task) == this->flopsMap.end()) {
          double flops = task->getFlops() + getFlops(workflow, workflow->getTaskChildren(task));
          this->flopsMap[task] = flops;
        }
        max_flops = (std::max)(this->flopsMap[task], max_flops);
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
    unsigned long
    CriticalPathPilotJobScheduler::getMaxParallelization(Workflow *workflow, const std::set<WorkflowTask *> &tasks) {
      unsigned long count = tasks.size();
      std::set<WorkflowTask *> children;

      for (auto task : tasks) {
        std::vector<WorkflowTask *> children_vector = workflow->getTaskChildren(task);
        std::copy(children_vector.begin(), children_vector.end(), std::inserter(children, children.end()));
      }
      if (children.size() > 0) {
        count = (std::max)(count, getMaxParallelization(workflow, children));
      }

      return count;
    }
}
