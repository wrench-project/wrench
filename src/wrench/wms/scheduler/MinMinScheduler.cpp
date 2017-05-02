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
#include "wms/scheduler/MinMinScheduler.h"
#include "workflow_job/StandardJob.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(minmin_scheduler, "Log category for Min-Min Scheduler");

namespace wrench {

    /**
     * @brief Compare the number of flops between to WorkflowTask lists
     *
     * @param lhs: a pair of a task ID and a list of WorkflowTask objects
     * @param rhs: a pair of a task ID and a list of WorkflowTask objects
     *
     * @return whether the number of flops from the left-hand-side WorkflowTask objects is smaller
     */
    bool MinMinScheduler::MinMinComparator::operator()(std::pair<std::string, std::vector<WorkflowTask *>> &lhs,
                                                       std::pair<std::string, std::vector<WorkflowTask *>> &rhs) {

      return getTotalFlops(lhs.second) < getTotalFlops(rhs.second);
    }

    /**
     * @brief Schedule and run a set of ready tasks in available compute resources
     *
     * @param job_manager: a pointer to a JobManager object
     * @param ready_tasks: a vector of ready WorkflowTask objects (i.e., ready tasks in the workflow)
     * @param compute_services: a set of pointers to ComputeService objects (i.e., compute services available to run jobs)
     */
    void MinMinScheduler::scheduleTasks(JobManager *job_manager,
                                        std::map<std::string, std::vector<WorkflowTask *>> ready_tasks,
                                        const std::set<ComputeService *> &compute_services) {

      WRENCH_INFO("There are %ld ready tasks to schedule", ready_tasks.size());

      // Sorting tasks
      std::vector<std::pair<std::string, std::vector<WorkflowTask *>>> min_vector(ready_tasks.begin(),
                                                                                  ready_tasks.end());
      std::sort(min_vector.begin(), min_vector.end(), MinMinComparator());

      for (auto itc : min_vector) {
        bool successfully_scheduled = false;

        double total_flops = getTotalFlops((*ready_tasks.begin()).second);

        // TODO: add pilot job support

        // Second: attempt to run the task on a compute resource
        WRENCH_INFO("Trying to submit task '%s' to a standard compute service...", itc.first.c_str());

        for (auto cs : compute_services) {
          WRENCH_INFO("Asking compute service %s if it can run this standard job...", cs->getName().c_str());

          if (cs->canRunJob(WorkflowJob::STANDARD, 1, total_flops)) {
            WRENCH_INFO("Submitting task %s for execution as a standard job", itc.first.c_str());
            WorkflowJob *job = (WorkflowJob *) job_manager->createStandardJob(itc.second);
            job_manager->submitJob(job, cs);
            successfully_scheduled = true;
            break;
          }
        }
        if (!successfully_scheduled) {
          WRENCH_INFO("no dice");
          break;
        }
      }
    }
};