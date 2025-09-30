/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** An batch service controller implementation that submits jobs to a batch compute
 ** service or delegate jobs to a peer...
 **/

#include "BatchServiceController.h"
#include "JobGenerationController.h"

#include "ControlMessages.h"

WRENCH_LOG_CATEGORY(batch_service_controller, "Log category for BatchServiceController");

namespace wrench {
    /**
     * @brief Constructor, which calls the super constructor
     *
     * @param hostname: the name of the host on which to start the controller
     * @param batch_compute_service: the batch compute service this controller is in charge of
     */
    BatchServiceController::BatchServiceController(const std::string& hostname,
                                                   const std::shared_ptr<BatchComputeService>& batch_compute_service) :
        ExecutionController(hostname, "batch"),
        _batch_compute_service(batch_compute_service) {
    }

    /**
     * @brief main method of the GreedyExecutionController daemon
     *
     * @return 0 on completion
     */
    int BatchServiceController::main() {
        WRENCH_INFO("Batch service execution controller starting");

        // Create my job manager
        _job_manager = this->createJobManager();

        while (true) {
            this->waitForAndProcessNextEvent();
        }
    }

    void BatchServiceController::processEventCustom(const std::shared_ptr<CustomEvent>& event) {
        if (auto job_request_message = std::dynamic_pointer_cast<JobRequestMessage>(event->message)) {
            WRENCH_INFO("Received a job request message for %s: %d compute nodes for %d seconds",
                        job_request_message->_name.c_str(), job_request_message->_num_compute_nodes,
                        job_request_message->_runtime);
            // Decide whether to forward or not based on some random criterion
            if (job_request_message->_can_forward && (job_request_message->_num_compute_nodes % 2)) {
                // Forward the job
                WRENCH_INFO("Decided to forward this job to my peer!");
                this->_peer->commport->dputMessage(
                    new JobRequestMessage(
                        job_request_message->_name,
                        job_request_message->_num_compute_nodes,
                        job_request_message->_runtime,
                        false));
            }
            else {
                // Do the job myself
                WRENCH_INFO("Doing this job myself!");
                auto job = _job_manager->createCompoundJob(job_request_message->_name);
                job->addSleepAction("", job_request_message->_runtime);
                job->addSleepAction("", job_request_message->_runtime);
                std::map<string, string> job_args;
                job_args["-N"] = std::to_string(job_request_message->_num_compute_nodes);
                job_args["-t"] = std::to_string(job_request_message->_runtime);
                job_args["-c"] = "10";
                _job_manager->submitJob(job, _batch_compute_service, job_args);
            }
        }
    }


    void BatchServiceController::processEventCompoundJobCompletion(
        const std::shared_ptr<CompoundJobCompletedEvent>& event) {
        auto job_name = event->job->getName();
        WRENCH_INFO("%s, which I ran locally, has completed. Notifying the job generating controller...",
                    job_name.c_str());
        _originator->commport->dputMessage(new JobNotificationMessage(job_name));
    }
} // namespace wrench
