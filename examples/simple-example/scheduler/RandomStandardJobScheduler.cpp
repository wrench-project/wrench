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
#include <numeric>

#include "wrench/simulation/Simulation.h"
#include "RandomStandardJobScheduler.h"
#include "wrench/exceptions/WorkflowExecutionException.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/workflow/job/PilotJob.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(random_standard_job_scheduler, "Log category for Random Scheduler");

namespace wrench {

    /**
     * @brief Schedule and run a set of ready tasks on available compute resources
     *
     * @param compute_services: a set of compute services available to run jobs
     * @param ready_tasks: a map of (ready) workflow tasks
     */

    void RandomStandardJobScheduler::scheduleTasks(const std::set<ComputeService *> &compute_services,
                       const std::map<std::string, std::vector<WorkflowTask *>> &tasks) {

      WRENCH_INFO("There are %ld ready tasks to schedule", tasks.size());
      for (auto itc : tasks) {
        bool successfully_scheduled = false;

        // First: attempt to run the task on a running pilot job
        WRENCH_INFO("Trying to submit task '%s' to a pilot job...", itc.first.c_str());

        double total_flops = Workflow::getSumFlops((*tasks.begin()).second);

        std::set<PilotJob *> running_pilot_jobs = this->job_manager->getRunningPilotJobs();
        for (auto pj : running_pilot_jobs) {
          ComputeService *cs = pj->getComputeService();

          // Check that the pilot job could in principle run this job
          if ((not cs->isUp()) || (not cs->supportsStandardJobs())) {
            continue;
          }

          // Check that it can run it right now in terms of idle cores
          try {
            std::vector<unsigned long> idle_cores = cs->getNumIdleCores();
            unsigned long num_idle_cores = (unsigned long) std::accumulate(idle_cores.begin(), idle_cores.end(), 0);
//            WRENCH_INFO("The compute service says it has %ld idle cores", num_idle_cores);
            if (num_idle_cores <= 0) {
              continue;
            }
          } catch (WorkflowExecutionException &e) {
            // The service has some problem, forget it
            continue;
          }

          // Check that it can run it right now in terms of TTL
          try {
            // Check that the TTL is ok (does a communication with the daemons)
            double ttl = cs->getTTL();
            // TODO: This duration is really hard to compute because we don't know
            // how many cores will be available, we don't know how the core schedule
            // will work out, etc. So right now, if the service couldn't run the job
            // sequentially, we say it can't run it at all. Something to fix at some point.
            // One option is to ask the user to provide the maximum amount of flop that will
            // be required on ONE core assuming min_num_cores cores are available?
            double duration = (total_flops / cs->getCoreFlopRate()[0]);
            if ((ttl > 0) && (ttl < duration)) {
              continue;
            }
          } catch (WorkflowExecutionException &e) {
            // Problem with the service, give up
            continue;
          }

          // We can submit!
          WRENCH_INFO("Submitting task %s for execution to a pilot job", itc.first.c_str());
          WorkflowJob *job = (WorkflowJob *) job_manager->createStandardJob(itc.second, {});
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

          // Check that the compute service could in principle run this job
          if ((not cs->isUp()) || (not cs->supportsStandardJobs())) {
            continue;
          }

          // Get the sum number of idle cores
          unsigned long sum_num_idle_cores;

          // Check that it can run it right now in terms of idle cores
          try {
            std::vector<unsigned long> num_idle_cores = cs->getNumIdleCores();
            sum_num_idle_cores = (unsigned  long)std::accumulate(num_idle_cores.begin(), num_idle_cores.end(), 0);
          } catch (WorkflowExecutionException &e) {
            // The service has some problem, forget it
            continue;
          }

          // Decision making
          if (sum_num_idle_cores <= 0) {
            continue;
          }

          // We can submit!
          WRENCH_INFO("Submitting task %s for execution as a standard job", itc.first.c_str());
          WorkflowJob *job = (WorkflowJob *) job_manager->createStandardJob(itc.second, {});
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
