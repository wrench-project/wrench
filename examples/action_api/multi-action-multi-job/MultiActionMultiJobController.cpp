/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** A Controller that creates a job with a powerful custom action
 **/

#include <iostream>

#include "MultiActionMultiJobController.h"

#define GFLOP (1000.0 * 1000.0 * 1000.0)
#define MB (1000.0 * 1000.0)

WRENCH_LOG_CATEGORY(custom_controller, "Log category for MultiActionMultiJobController");

namespace wrench {

    /**
     * @brief Constructor, which calls the super constructor
     *
     * @param bm_cs: a bare-metal compute service
     * @param cloud_cs: a cloud compute service
     * @param ss_1: a storage service
     * @param ss_2: a storage service
     * @param hostname: the name of the host on which to start the Controller
     */
    MultiActionMultiJobController::MultiActionMultiJobController(
            std::shared_ptr<BareMetalComputeService> bm_cs,
            std::shared_ptr<CloudComputeService> cloud_cs,
            std::shared_ptr<StorageService> ss_1,
            std::shared_ptr<StorageService> ss_2,
            const std::string &hostname) : ExecutionController(hostname, "mamj"),
                                           bm_cs(bm_cs), cloud_cs(cloud_cs), ss_1(ss_1), ss_2(ss_2) {}

    /**
     * @brief main method of the MultiActionMultiJobController daemon
     *
     * @return 0 on completion
     *
     * @throw std::runtime_error
     */
    int MultiActionMultiJobController::main() {

        /* Set the logging output to GREEN */
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);

        WRENCH_INFO("Controller starting on host %s", Simulation::getHostName().c_str());

        /* Create a job manager so that we can create/submit jobs */
        auto job_manager = this->createJobManager();

        /* Specify a 1000-byte file that will be written to at some point in the simulation */
        auto output_file = Simulation::addFile("output_file", 100 * MB);

        /* Specify a 10,000-nyte file that will be read from at some point in the simulation */
        auto input_file = Simulation::addFile("input_file", 200 * MB);

        /* Creates an instance of input_file on both storage services. This takes zero simulation time. After
         * all, that file needs to be there somewhere initially if it's indeed some input file */
        ss_1->createFile(input_file, wrench::FileLocation::LOCATION(ss_1, "/data/"));

        /* Create a one-action job with one action that simply sleeps for 10 seconds */
        auto job1 = job_manager->createCompoundJob("job1");
        job1->addSleepAction("sleep_10", 10.0);

        /* Create a two-action job where the first action computes something, and the second action
         * write data to data_file somewhere */
        auto job2 = job_manager->createCompoundJob("job2");
        auto job2_compute_action = job2->addComputeAction("compute", 1000 * GFLOP, 5 * MB, 2, 8, wrench::ParallelModel::AMDAHL(0.8));
        // If no action name is given, a unique name will be picked
        auto job2_file_write_action = job2->addFileWriteAction("", output_file, wrench::FileLocation::LOCATION(ss_1));
        job2->addActionDependency(job2_compute_action, job2_file_write_action);

        /* Create a one-action job with a file-copy action */
        auto job3 = job_manager->createCompoundJob("job3");

        // A small data structure that, just for fun, specifies a target storage service
        // to used based on the custom action is running
        std::map<std::string, std::shared_ptr<StorageService>> ss_to_use = {{"ComputeHost1", ss_1}, {"ComputeHost2", ss_2}};

        // Create the custom action with two lambdas
        job3->addCustomAction(
                "file_copy",
                0, 0,
                [ss_to_use, input_file](std::shared_ptr<ActionExecutor> action_executor) {
                    WRENCH_INFO("Custom action executing on host %s", action_executor->getHostname().c_str());

                    // Which host I am running on?
                    auto execution_host = action_executor->getPhysicalHostname();
                    // Based on where I am running, pick a storage service
                    auto target_ss = ss_to_use.at(execution_host);
                    WRENCH_INFO("Custom action about to read file from storage service on host %s",
                                target_ss->getHostname().c_str());
                    // Read a input_file from the target storage service (which takes some time)
                    target_ss->readFile(input_file, wrench::FileLocation::LOCATION(target_ss, "/data/"));
                    // Sleep for 5 seconds
                    WRENCH_INFO("Custom action got the file and now is sleeping for 5 seconds");
                    Simulation::sleep(5.0);
                    WRENCH_INFO("Custom action deleting the file!");
                    // Deleted the input file from the target storage service!
                    target_ss->deleteFile(input_file, wrench::FileLocation::LOCATION(target_ss, "/data/"));
                },
                [](std::shared_ptr<ActionExecutor> action_executor) {
                    WRENCH_INFO("Custom action terminating");
                });

        /* Making job3 depend on job1 */
        job3->addParentJob(job1);

        /* Create and start a 5-core VM on the cloud compute service */
        WRENCH_INFO("Creating a VM on the cloud compute service");
        auto my_vm = cloud_cs->createVM(5, 100.0);
        auto my_vm_cs = cloud_cs->startVM(my_vm);
        WRENCH_INFO("VM '%s' created and started", my_vm.c_str());

        /* Submit job1 to the VM (which really is a bare-metal service) */
        WRENCH_INFO("Submitting job %s to the VM", job1->getName().c_str());
        job_manager->submitJob(job1, my_vm_cs);

        /* Submit job2 to the bare-metal service */
        WRENCH_INFO("Submitting job %s to the bare-metal compute service", job2->getName().c_str());
        job_manager->submitJob(job2, bm_cs);

        /* Submit job3 to the bare-metal service */
        WRENCH_INFO("Submitting job %s to the VM", job3->getName().c_str());
        job_manager->submitJob(job3, bm_cs);

        /* Wait for an react fo execution events. We should be getting 3 "job completed" events */
        int num_events = 0;
        while (num_events < 3) {
            auto event = this->waitForNextEvent();
            if (auto job_completion_event = std::dynamic_pointer_cast<wrench::CompoundJobCompletedEvent>(event)) {
                auto completed_job = job_completion_event->job;
                WRENCH_INFO("Job %s has completed!", completed_job->getName().c_str());
                WRENCH_INFO("It had %lu actions:", completed_job->getActions().size());
                for (auto const &action: completed_job->getActions()) {
                    WRENCH_INFO("  - Action %s ran on host %s (physical: %s)",
                                action->getName().c_str(),
                                action->getExecutionHistory().top().execution_host.c_str(),
                                action->getExecutionHistory().top().physical_execution_host.c_str());
                    WRENCH_INFO("     - it used %lu cores for computation, and %.2lf bytes of RAM",
                                action->getExecutionHistory().top().num_cores_allocated,
                                action->getExecutionHistory().top().ram_allocated);
                    WRENCH_INFO("     - it started at time %.2lf and finished at time %.2lf",
                                action->getExecutionHistory().top().start_date,
                                action->getExecutionHistory().top().end_date);
                }
            } else {
                throw std::runtime_error("Unexpected event: " + event->toString());
            }
            num_events++;
        }

        WRENCH_INFO("Controller terminating");
        return 0;
    }

}// namespace wrench
