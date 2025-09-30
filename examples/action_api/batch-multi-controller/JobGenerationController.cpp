/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** An execution controller implementation that generates job specifications and
 ** sends them to batch service controllers
 **/

#include <iostream>

#include "JobGenerationController.h"
#include "BatchServiceController.h"
#include "ControlMessages.h"

WRENCH_LOG_CATEGORY(job_generation_controller, "Log category for JobGenerationController");

namespace wrench {
    /**
     * Constructor
     *
     * @param hostname: the name of the host on which to start the controller
     * @param num_jobs: the number of jobs
     * @param batch_service_controllers: the available batch compute service controllers
     */
    JobGenerationController::JobGenerationController(const std::string& hostname,
                                                     int num_jobs,
                                                     const std::vector<std::shared_ptr<BatchServiceController>>&
                                                     batch_service_controllers) :
        ExecutionController(hostname, "job_generator"),
        _num_jobs(num_jobs),
        _batch_service_controllers(batch_service_controllers) {
    }

    /**
     * @brief main method of the JobGenerationController daemon
     *
     * @return 0 on completion
     */
    int JobGenerationController::main() {
        /* Set the logging output to GREEN */
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);
        WRENCH_INFO("Job generation controller starting");

        /* Initialize and seed a RNG */
        std::uniform_int_distribution<int> dist(100, 1000);
        std::mt19937 rng(42);

        /* Creating a set of jobs */
        WRENCH_INFO("Creating a set of jobs to process:");
        std::vector<std::tuple<std::string, int, int, int>> jobs;
        int now = 0;
        for (int i = 0; i < _num_jobs; i++) {
            std::string job_name = "Job_" + std::to_string(i);
            int arrival_time = now + dist(rng) % 500;
            now = arrival_time;
            int num_compute_nodes = 1 + dist(rng) % 2; // 1 or 2 compute nodes
            int runtime = dist(rng);
            jobs.emplace_back(job_name, arrival_time, runtime, num_compute_nodes);
            WRENCH_INFO("  - %s: arrival=%d compute_nodes=%d runtime=%d", job_name.c_str(), arrival_time, num_compute_nodes, runtime);
        }

        /* Main loop */
        int next_job_to_submit = 0;
        int num_completed_jobs = 0;

        // Set a timer for the arrival of the first job
        this->setTimer(std::get<1>(jobs.at(0)), "submit the next job");

        while (num_completed_jobs < _num_jobs) {

            // Wait for the next event
            auto event = this->waitForNextEvent();

            if (std::dynamic_pointer_cast<TimerEvent>(event)) {
                // If it's a timer event, then we send the job to a randomly selected batch service controller
                auto target_bath_service_controller_index = dist(rng) % 2;
                auto target_batch_service_controller = _batch_service_controllers.at(
                    target_bath_service_controller_index);
                auto [job_name, arrival_time, runtime, num_compute_nodes] = jobs.at(next_job_to_submit);
                WRENCH_INFO("Sending %s to batch service controller #%d",
                            job_name.c_str(), target_bath_service_controller_index);
                target_batch_service_controller->commport->dputMessage(
                    new JobRequestMessage(job_name, num_compute_nodes, runtime, true));
                next_job_to_submit++;

                // Set the timer for the next job, if need be
                if (next_job_to_submit < _num_jobs) {
                    auto next_job_arrival_time = std::get<1>(jobs.at(next_job_to_submit));
                    this->setTimer(next_job_arrival_time, "submit the next job");
                }

            } else if (auto custom_event = std::dynamic_pointer_cast<CustomEvent>(event)) {
                // If it's a job completion notification, then we just take it into account
                if (auto job_notification_message = std::dynamic_pointer_cast<JobNotificationMessage>(
                    custom_event->message)) {
                    WRENCH_INFO("Notified that %s has completed!", job_notification_message->_name.c_str());
                    num_completed_jobs++;
                }
            }
        }
        WRENCH_INFO("Terminating!");
        return 0;
    }

    void JobGenerationController::processEventCustom(const std::shared_ptr<CustomEvent>& event) {
    }
} // namespace wrench
