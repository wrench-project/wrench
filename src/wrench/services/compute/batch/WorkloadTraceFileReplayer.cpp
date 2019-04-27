/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <wrench/simgrid_S4U_util/S4U_Simulation.h>
#include <wrench/services/compute/batch/BatchComputeService.h>
#include <wrench-dev.h>
#include "WorkloadTraceFileReplayer.h"
#include "WorkloadTraceFileReplayerEventReceiver.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(workload_trace_file_replayer, "Log category for Trace File Replayer");

namespace wrench {

    /**
     * @brief Constructor
     * @param hostname: the name of the host on which the trace file replayer will be started
     * @param batch_service: the batch service to which it submits jobs
     * @param num_cores_per_node: the number of cores per host on the batch service
     * @param use_actual_runtimes_as_requested_runtimes: if true, use actual runtimes as requested runtimes
     * @param workload_trace: the workload trace to be replayed
     */
    WorkloadTraceFileReplayer::WorkloadTraceFileReplayer(std::string hostname,
                                                         std::shared_ptr<BatchComputeService> batch_service,
                                                         unsigned long num_cores_per_node,
                                                         bool use_actual_runtimes_as_requested_runtimes,
                                                         std::vector<std::tuple<std::string, double, double, double, double, unsigned int>> &workload_trace
    ) :
            WMS(nullptr, nullptr,
                {batch_service}, {},
                {},
                nullptr, hostname,
                "workload_tracefile_replayer"),
            workload_trace(workload_trace),
            batch_service(batch_service),
            num_cores_per_node(num_cores_per_node),
            use_actual_runtimes_as_requested_runtimes(use_actual_runtimes_as_requested_runtimes)
    {}


    int WorkloadTraceFileReplayer::main() {

      // Create a Job Manager
      std::shared_ptr<JobManager> job_manager = this->createJobManager();

      // Create and handle a bogus workflow
      auto workflow = new Workflow();
      this->addWorkflow(workflow);

      // Create the dual WMS that will just receive workflow execution events so that I don't have to
      std::shared_ptr<WorkloadTraceFileReplayerEventReceiver> event_receiver = std::shared_ptr<WorkloadTraceFileReplayerEventReceiver>(
              new WorkloadTraceFileReplayerEventReceiver(this->hostname, job_manager));

      // Start the WorkloadTraceFileReplayerEventReceiver
      event_receiver->addWorkflow(workflow, S4U_Simulation::getClock());
      event_receiver->simulation = this->simulation;
      event_receiver->start(event_receiver, true, false); // Daemonized, no auto-restart

      double core_flop_rate = (*(this->batch_service->getCoreFlopRate().begin())).second;

      unsigned long job_count = 0;

      // Record the start time of the current time (submission times will be just offsets from this time)
      double real_start_time = S4U_Simulation::getClock();

      unsigned long counter = 0;
      for (auto job : this->workload_trace) {

        // Sleep until the submission time
        double sub_time = real_start_time + std::get<1>(job);
        double curtime = S4U_Simulation::getClock();
        double sleeptime = sub_time - curtime;
        if (sleeptime > 0)
          wrench::S4U_Simulation::sleep(sleeptime);

        // Get job information
        std::string job_id = std::get<0>(job);
        double time = std::get<2>(job);
        double requested_time = std::get<3>(job);
        if (this->use_actual_runtimes_as_requested_runtimes) {
          requested_time = time;
        }
        double requested_ram = std::get<4>(job);
        int num_nodes = std::get<5>(job);

        // Create the set of tasks
        std::vector<WorkflowTask *> to_submit;
        for (int i = 0; i < num_nodes; i++) {
          double time_fudge = 1; // 1 second seems to make it all work!
          double task_flops = num_cores_per_node * (core_flop_rate * (time - time_fudge));
          double parallel_efficiency = 1.0;
          WorkflowTask *task = workflow->addTask(this->getName() + "_job_" + std::to_string(job_count) + "_task_" + std::to_string(i),
                                                 task_flops,
                                                 num_cores_per_node, num_cores_per_node, parallel_efficiency,
                                                 requested_ram);
          to_submit.push_back(task);
        }

        // Create a Standard Job with only the tasks
        StandardJob *standard_job = job_manager->createStandardJob(to_submit, {});
        job_count++;

        // Create the batch-specific argument
        std::map<std::string, std::string> batch_job_args;
        batch_job_args["-N"] = std::to_string(num_nodes); // Number of nodes/taks
        batch_job_args["-t"] = std::to_string(1 + requested_time / 60); // Time in minutes (note the +1)
        batch_job_args["-c"] = std::to_string(num_cores_per_node); //number of cores per task
        batch_job_args["-color"] = "green";

        // Submit this job to the batch service
        WRENCH_INFO("#%lu: Submitting a [-N:%s, -t:%s, -c:%s] job",
                    counter++,
                batch_job_args["-N"].c_str(),
                batch_job_args["-t"].c_str(),
                batch_job_args["-c"].c_str());
        try {
          job_manager->submitJob(standard_job, this->batch_service, batch_job_args);
        } catch (WorkflowExecutionException &e) {
          WRENCH_INFO("Couldn't submit a replayed job: %s (ignoring)", e.getCause()->toString().c_str());
        }

      }

      // Memory leak: workflow
      // Not clear how to fix this since we can't delete it until all tasks are completed...
      // And yet when we get there there are still running tasks...
      return 0;
    }

};
