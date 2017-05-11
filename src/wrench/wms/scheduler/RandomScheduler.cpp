/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <xbt.h>
#include <set>

#include "logging/TerminalOutput.h"
#include "simgrid_S4U_util/S4U_Mailbox.h"
#include "simulation/Simulation.h"
#include "workflow_job/PilotJob.h"
#include "workflow_job/StandardJob.h"
#include "wms/scheduler/RandomScheduler.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(random_scheduler, "Log category for Random Scheduler");

namespace wrench {

    /**
     * @brief Schedule and run a set of ready tasks in available compute resources
     *
     * @param job_manager: a pointer to a JobManager object
     * @param ready_tasks: a map of ready WorkflowTask objects (i.e., ready tasks in the workflow)
     * @param compute_services: a set of pointers to ComputeService objects (compute services available to run jobs)
     */
    void RandomScheduler::scheduleTasks(JobManager *job_manager,
                                        std::map<std::string, std::vector<WorkflowTask *>> ready_tasks,
                                        const std::set<ComputeService *> &compute_services) {


      // TODO: Refactor to avoid code duplication

      WRENCH_INFO("There are %ld ready tasks to schedule", ready_tasks.size());
      for (auto itc : ready_tasks) {
        bool successfully_scheduled = false;

        // First: attempt to run the task on a running pilot job
        WRENCH_INFO("Trying to submit task '%s' to a pilot job...", itc.first.c_str());

        double total_flops = getTotalFlops((*ready_tasks.begin()).second);

        std::set<PilotJob *> running_pilot_jobs = job_manager->getRunningPilotJobs();
        for (auto pj : running_pilot_jobs) {
          ComputeService *cs = pj->getComputeService();

          if (not cs->canRunJob(WorkflowJob::STANDARD, 1, total_flops)) {
            continue;
          }

          // We can submit!
          WRENCH_INFO("Submitting task %s for execution to a pilot job", itc.first.c_str());
          WorkflowJob *job = (WorkflowJob *) job_manager->createStandardJob(itc.second);
          job_manager->submitJob(job, cs);
          successfully_scheduled = true;
          break;
        }

        if (successfully_scheduled) {
          continue;
        } else {
          WRENCH_INFO("no dice!");
        }

        // Second: attempt to run the task on a compute resource
        WRENCH_INFO("Trying to submit task '%s' to a standard compute service...", itc.first.c_str());

        for (auto cs : compute_services) {
          WRENCH_INFO("Asking compute service %s if it can run this standard job...", cs->getName().c_str());
          bool can_run_job = cs->canRunJob(WorkflowJob::STANDARD, 1, total_flops);
          if (can_run_job) {
            WRENCH_INFO("Compute service %s says it can run this standard job!", cs->getName().c_str());
          } else {
            WRENCH_INFO("Compute service %s says it CANNOT run this standard job :(", cs->getName().c_str());
          }

          if (not can_run_job) { continue; }

          // We can submit!
          WRENCH_INFO("Submitting task %s for execution as a standard job", itc.first.c_str());
          WorkflowJob *job = (WorkflowJob *) job_manager->createStandardJob(itc.second);
          job_manager->submitJob(job, cs);
          successfully_scheduled = true;
          break;
        }

        if (not successfully_scheduled) {
          WRENCH_INFO("no dice");
          break;
        }

      }
      WRENCH_INFO("Done with scheduling tasks as standard jobs");
    }
}
