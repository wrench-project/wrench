/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <wrench/exceptions/ExecutionException.h>
#include <wrench-dev.h>
#include "wrench/services/compute/batch/workload_helper_classes/WorkloadTraceFileReplayerEventReceiver.h"

WRENCH_LOG_CATEGORY(wrench_core_one_job_wms, "Log category for WorkloadTraceFileReplayerEventReceiver");

namespace wrench {

    /**
     * @brief main method of the WorkloadTraceFileReplayerEventReceiver daemon
     * @return 0 on success
     */
    int WorkloadTraceFileReplayerEventReceiver::main() {
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_YELLOW);

        WRENCH_INFO("Starting!");
        while (true) {
            std::shared_ptr<StandardJob> job = nullptr;

            // Wait for the workflow execution event
            WRENCH_INFO("Waiting for job completion...");
            std::shared_ptr<wrench::ExecutionEvent> event;
            try {
                event = this->waitForNextEvent();

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
            } catch (wrench::ExecutionException &e) {
                //ignore (network error or something)
                continue;
            }

            // DO NOT Remove tasks that correspond to the job, since people may still care! And
            // besides, timestamps have been generated that correspond to these tasks!
            //            for (auto t : job->getTasks()) {
            //                this->getWorkflow()->removeTask(t);
            //            }

            //            this->job_manager->forgetJob(job);
        }
    }
};// namespace wrench
