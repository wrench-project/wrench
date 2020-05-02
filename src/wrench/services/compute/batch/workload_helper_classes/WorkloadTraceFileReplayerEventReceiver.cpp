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

WRENCH_LOG_CATEGORY(wrench_core_one_job_wms, "Log category for One Job WMS");

namespace wrench {

    /**
     * @brief main method of the OneJobWMS daemon
     * @return 0 on success
     */
    int WorkloadTraceFileReplayerEventReceiver::main() {

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_YELLOW);

        WRENCH_INFO("Starting!");
        while (true) {

            StandardJob *job = nullptr;

            // Wait for the workflow execution event
            WRENCH_INFO("Waiting for job completion...");
            std::shared_ptr<wrench::WorkflowExecutionEvent> event;
            try {
                event = this->getWorkflow()->waitForNextExecutionEvent();

                if (auto real_event = std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
                    job = real_event->standard_job;
                    // success, do nothing
                    WRENCH_INFO("Received job completion notification");
                } else if (auto real_event = std::dynamic_pointer_cast<wrench::StandardJobFailedEvent>(event)) {
                    job = real_event->standard_job;
                    WRENCH_INFO("Received job failure notification: %s",
                                real_event->failure_cause->toString().c_str());
                } else {
                    throw std::runtime_error(
                            "WorkloadTraceFileReplayerEventReceiver::main(): Unexpected workflow execution event");
                }
            } catch (wrench::WorkflowExecutionException &e) {
                //ignore (network error or something)
                continue;

            }

            // Remove tasks that correspond to the job
            for (auto t : job->getTasks()) {
                this->getWorkflow()->removeTask(t);
            }

            this->job_manager->forgetJob(job);
        }

    }
};
