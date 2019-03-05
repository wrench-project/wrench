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
#include "WorkloadTraceFileReplayerEventReceiver.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(one_job_wms, "Log category for One Job WMS");

namespace wrench {

    /**
     * @brief main method of the OneJobWMS daemon
     * @return 0 on success
     */
    int WorkloadTraceFileReplayerEventReceiver::main() {

      TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_YELLOW);

      WRENCH_INFO("Starting!");
      while (true) {

        StandardJob *job;

        // Wait for the workflow execution event
        WRENCH_INFO("Waiting for job completion...");
        std::unique_ptr<wrench::WorkflowExecutionEvent> event;
        try {
          event = this->getWorkflow()->waitForNextExecutionEvent();
          switch (event->type) {
            case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
              auto real_event = dynamic_cast<wrench::StandardJobCompletedEvent *>(event.get());
              job = real_event->standard_job;
              // success, do nothing
              WRENCH_INFO("Received job completion notification");
              break;
            }
            case wrench::WorkflowExecutionEvent::STANDARD_JOB_FAILURE: {
              // failure, do nothing
              auto real_event = dynamic_cast<wrench::StandardJobFailedEvent *>(event.get());
              job = real_event->standard_job;
              WRENCH_INFO("Received job failure notification: %s",
                          real_event->failure_cause->toString().c_str());
              break;
            }
            default: {
              throw std::runtime_error(
                      "WorkloadTraceFileReplayerEventReceiver::main(): Unexpected workflow execution event: " +
                      std::to_string((int) (event->type)));
            }
          }
        } catch (wrench::WorkflowExecutionException &e) {
          //ignore (network error or something)
          return 0;

        }

        // Remove tasks that correspond to the job
        for (auto t : job->getTasks()) {
          this->getWorkflow()->removeTask(t);
        }

        this->job_manager->forgetJob(job);
      }
      return 0;

    }
};
