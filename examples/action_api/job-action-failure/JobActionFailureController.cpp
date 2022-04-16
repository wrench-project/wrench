/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** A Controller that creates a job that experience failures
 **/

#include <iostream>
#include <utility>

#include "JobActionFailureController.h"

#define GFLOP (1000.0 * 1000.0 * 1000.0)
#define MB (1000.0 * 1000.0)
#define GB (1000.0 * 1000.0 * 1000.0)

WRENCH_LOG_CATEGORY(custom_controller, "Log category for JobActionFailureController");

namespace wrench {

    /**
     * @brief Constructor, which calls the super constructor
     *
     * @param bm_cs: a bare-metal compute service
     * @param ss_1: a storage service
     * @param ss_2: a storage service
     * @param hostname: the name of the host on which to start the Controller
     */
    JobActionFailureController::JobActionFailureController(
            std::shared_ptr<BareMetalComputeService> bm_cs,
            std::shared_ptr<StorageService> ss_1,
            std::shared_ptr<StorageService> ss_2,
            const std::string &hostname) : ExecutionController(hostname, "mamj"), bm_cs(std::move(bm_cs)), ss_1(std::move(ss_1)), ss_2(std::move(ss_2)) {}

    /**
     * @brief main method of the SuperCustomActionController daemon
     *
     * @return 0 on completion
     *
     * @throw std::runtime_error
     */
    int JobActionFailureController::main() {
        /* Set the logging output to GREEN */
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);

        WRENCH_INFO("Controller starting on host %s", Simulation::getHostName().c_str());

        /* Create a job manager so that we can create/submit jobs */
        auto job_manager = this->createJobManager();

        /* Specify a 1000-byte file that will be written to at some point in the simulation,
         * but it is too big for the storage service where we will write it! */
        auto output_file = Simulation::addFile("output_file", 100 * GB);

        /* Specify a 10,000-nyte file that will be read from at some point in the simulation */
        auto input_file = Simulation::addFile("input_file", 10 * MB);

        /* Creates an instance of input_file on both storage services. This takes zero simulation time. After
         * all, that file needs to be there somewhere initially if it's indeed some input file */
        wrench::Simulation::createFile(input_file, wrench::FileLocation::LOCATION(ss_1, "/data/"));

        /* Create a job  */
        auto job = job_manager->createCompoundJob("job");

        /* Add a compute action that will work fine */
        auto compute_action = job->addComputeAction("compute", 200 * GFLOP, 100, 3, 4, wrench::ParallelModel::AMDAHL(0.95));

        /* Add a file-write action that will fail */
        auto file_write_action = job->addFileWriteAction("file_write", output_file, wrench::FileLocation::LOCATION(ss_2));

        /* Add a long compute action that will fail when we kill the compute service! */
        auto compute_long = job->addComputeAction("compute_long", 10000 * GFLOP, 100, 1, 20, wrench::ParallelModel::AMDAHL(0.57));

        /* Add a sleep action */
        auto sleep_action = job->addSleepAction("sleep", 10.0);

        /* Create action dependencies */
        job->addActionDependency(compute_action, file_write_action);
        job->addActionDependency(compute_long, sleep_action);

        /* Submit the job to the bare-metal service compute service */
        WRENCH_INFO("Submitting the job %s to the bare-metal service", job->getName().c_str());
        job_manager->submitJob(job, bm_cs);

        /* Sleep for 100 seconds */
        wrench::Simulation::sleep(20);

        /* Kill the bare-metal compute service */
        this->bm_cs->stop(true, wrench::ComputeService::TerminationCause::TERMINATION_COMPUTE_SERVICE_TERMINATED);

        /* Wait for an execution event */
        auto event = this->waitForNextEvent();
        if (auto job_completion_event = std::dynamic_pointer_cast<wrench::CompoundJobFailedEvent>(event)) {
            auto completed_job = job_completion_event->job;
            WRENCH_INFO("Job %s has failed!", completed_job->getName().c_str());
            WRENCH_INFO("Job failure cause: %s", job_completion_event->failure_cause->toString().c_str());
            WRENCH_INFO("It had %lu actions:", completed_job->getActions().size());
            for (auto const &action: completed_job->getActions()) {
                WRENCH_INFO("  * Action %s: in state %s", action->getName().c_str(), action->getStateAsString().c_str());
                if (action->getState() == wrench::Action::State::FAILED) {
                    WRENCH_INFO("    - start date: %.2lf", action->getStartDate());
                    WRENCH_INFO("    - ran on host: %s", action->getExecutionHistory().top().physical_execution_host.c_str());
                    WRENCH_INFO("    - failure date: %.2lf", action->getEndDate());
                    WRENCH_INFO("    - failure cause: %s", action->getFailureCause()->toString().c_str());
                } else if (action->getState() == wrench::Action::State::COMPLETED) {
                    WRENCH_INFO("    - start date: %.2lf", action->getStartDate());
                    WRENCH_INFO("    - ran on host: %s", action->getExecutionHistory().top().physical_execution_host.c_str());
                    WRENCH_INFO("    - completion date: %.2lf", action->getEndDate());
                } else if (action->getState() == wrench::Action::State::KILLED) {
                    WRENCH_INFO("    - start date: %.2lf", action->getStartDate());
                    WRENCH_INFO("    - ran on host: %s",
                                action->getExecutionHistory().top().physical_execution_host.c_str());
                    WRENCH_INFO("    - kill date: %.2lf", action->getEndDate());
                    WRENCH_INFO("    - kill cause: %s", action->getFailureCause()->toString().c_str());
                }
            }
        } else {
            throw std::runtime_error("Unexpected event: " + event->toString());
        }

        WRENCH_INFO("Controller terminating");
        return 0;
    }

}// namespace wrench
