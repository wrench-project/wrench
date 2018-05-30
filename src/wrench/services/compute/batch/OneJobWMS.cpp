/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <wrench/exceptions/WorkflowExecutionException.h>
#include <wrench-dev.h>
#include "OneJobWMS.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(one_job_wms, "Log category for One Job WMS");

namespace wrench {

    /**
     * @brief main method ot the OneJobWMS daemon
     * @return 0 on success
     */
    int OneJobWMS::main() {

      TerminalOutput::setThisProcessLoggingColor(TerminalOutput::Color::COLOR_YELLOW);

      WRENCH_INFO("Starting!");
      auto workflow = new Workflow();

      // Create the set of tasks
      for (int i = 0; i < this->num_nodes; i++) {
        double time_fudge = 1; // 1 second seems to make it all work!
        double task_flops = this->num_cores_per_task * (this->batch_service_core_flop_rate * (this->time - time_fudge));
        double parallel_efficiency = 1.0;
        workflow->addTask(this->job_id + "_task_" + std::to_string(i),
                          task_flops,
                          this->num_cores_per_task, this->num_cores_per_task, parallel_efficiency,
                          this->requested_ram);
      }

      // Create a Job Manager
      std::shared_ptr<JobManager> job_manager = this->createJobManager();

      // Create a Standard Job with only the tasks
      StandardJob *standard_job = job_manager->createStandardJob(workflow->getTasks(), {});

      // Create the batch-specific argument
      std::map<std::string, std::string> batch_job_args;
      batch_job_args["-N"] = std::to_string(this->num_nodes); // Number of nodes/taks
      batch_job_args["-t"] = std::to_string(1 + this->requested_time / 60); // Time in minutes (note the +1)
      batch_job_args["-c"] = std::to_string(this->num_cores_per_task); //number of cores per task

      // Submit this job to the batch service
      WRENCH_INFO("Submitting a [-N:%s, -t:%s, -c:%s] job",
                  batch_job_args["-N"].c_str(), batch_job_args["-t"].c_str(), batch_job_args["-c"].c_str());
      job_manager->submitJob(standard_job, *(this->getAvailableComputeServices().begin()), batch_job_args);

      // Wait for the workflow execution event
      WRENCH_INFO("Waiting for job completion...");
      std::unique_ptr<wrench::WorkflowExecutionEvent> event;
      try {
        event = workflow->waitForNextExecutionEvent();
        switch (event->type) {
          case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
            // success, do nothing
            WRENCH_INFO("Received job completion notification");
            break;
          }
          case wrench::WorkflowExecutionEvent::STANDARD_JOB_FAILURE: {
            // failre, do nothing
            wrench::StandardJobFailedEvent *real_event = dynamic_cast<wrench::StandardJobFailedEvent*>(event.get());
            WRENCH_INFO("Received job failure notification: %s",
                        real_event->failure_cause->toString().c_str());
            break;
          }
          default: {
            throw std::runtime_error(
                    "OneJobWMS::main(): Unexpected workflow execution event: " + std::to_string((int) (event->type)));
          }
        }
      } catch (wrench::WorkflowExecutionException &e) {
        //ignore (network error or something)
      }

      // Clean up
      job_manager->stop();
      delete workflow;

      return 0;
    }
};
