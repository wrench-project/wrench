/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <xbt.h>

#include "logging/TerminalOutput.h"
#include "wms/scheduler/Scheduler.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(scheduler, "Log category for Scheduler");

namespace wrench {

/**
    * @brief Schedule and run pilot jobs
    *
    * @param job_manager: a pointer to a JobManager object
    * @param workflow: a pointer to a Workflow object
    * @param flops: the number of flops that the PilotJob should be able to do before terminating
    * @param compute_services: a set of pointers to ComputeService objects (i.e., compute services available to run jobs)
    */
    void Scheduler::schedulePilotJobs(JobManager *job_manager,
                                      Workflow *workflow,
                                      double flops,
                                      const std::set<ComputeService *> &compute_services) {

      // If there is always a pilot job in the system, do nothing
      if ((job_manager->getRunningPilotJobs().size() > 0)) {
        WRENCH_INFO("There is already a pilot job in the system...");
        return;
      }

      // Submit a pilot job to the first compute service that can support it
      ComputeService *target_service = nullptr;
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
      double pilot_job_duration = flops / target_service->getCoreFlopRate();
      WRENCH_INFO("Submitting a pilot job (1 core, %lf seconds)", pilot_job_duration);

      WorkflowJob *job = (WorkflowJob *) job_manager->createPilotJob(workflow, 1, pilot_job_duration);
      job_manager->submitJob(job, target_service);
    }

    /**
     * @brief Get the total number of flops for a list of tasks
     *
     * @param tasks: list of tasks
     *
     * @return the total number of flops
     */
    double Scheduler::getTotalFlops(std::vector<WorkflowTask *> tasks) {
      double total_flops = 0;
      for (auto it : tasks) {
        total_flops += (*it).getFlops();
      }
      return total_flops;
    }
}