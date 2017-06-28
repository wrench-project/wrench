/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

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

      for (auto task : workflow->getTasks()) {
        flops += 1.1 * task->getFlops();
      }

//      double flops = 10000.00; // bogus default
//      if (workflow->getReadyTasks().size() > 0) {
//        // Heuristic: ask for something that can run 2 times the next ready tasks..
//        flops = 1.5 * scheduler->getTotalFlops((*workflow->getReadyTasks().begin()).second);
//      }
      
      scheduler->schedulePilotJobs(job_manager, workflow, flops, compute_services);
    }
}