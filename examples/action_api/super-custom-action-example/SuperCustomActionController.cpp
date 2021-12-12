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
 *
 ** Example invocation of the simulator with only controller logging:
 **    ./wrench-example-multi-action-multi-job ./four_hosts.xml --log=custom_controller.threshold=info
 **
 ** Example invocation of the simulator with full logging:
 **    ./wrench-example-multi-action-multi-job ./four_hosts.xml --wrench-full-log
 **/

#include <iostream>

#include "SuperCustomActionController.h"

#define GFLOP (1000.0 * 1000.0 * 1000.0)
#define MB (1000.0 * 1000.0)

        WRENCH_LOG_CATEGORY(custom_controller, "Log category for SuperCustomActionController");

namespace wrench {

    /**
     * @brief Constructor, which calls the super constructor
     *
     * @param compute_services: a set of compute services available to run tasks
     * @param storage_services: a set of storage services available to store files
     * @param hostname: the name of the host on which to start the Controller
     */
    SuperCustomActionController::SuperCustomActionController(
            std::shared_ptr<BareMetalComputeService> bm_cs,
            std::shared_ptr<CloudComputeService> cloud_cs,
            std::shared_ptr<StorageService> ss_1,
            std::shared_ptr<StorageService> ss_2,
            const std::string &hostname) : ExecutionController(
            hostname,"mamj"), bm_cs(bm_cs), cloud_cs(cloud_cs), ss_1(ss_1), ss_2(ss_2) {}

    /**
     * @brief main method of the SuperCustomActionController daemon
     *
     * @return 0 on completion
     *
     * @throw std::runtime_error
     */
    int SuperCustomActionController::main() {

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

        /* Create a one-action job with a custom action */
        auto job = job_manager->createCompoundJob("job");

        std::map<std::string, unsigned long> num_cores_to_use_for_vm = {{"ComputeHost1", 2}, {"ComputeHost2", 4}};
        auto cloud_service = this->cloud_cs;
        job->addCustomAction("powerful",
                             [num_cores_to_use_for_vm, cloud_service](std::shared_ptr<ActionExecutor> action_executor) {

                                 TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_RED);

                                 WRENCH_INFO("Custom action executing on host %s", action_executor->getHostname().c_str());
                                 // Which host I am running on?
                                 auto execution_host = action_executor->getPhysicalHostname();
                                 // Based on where I am running, pick the number of cores
                                 auto num_cores = num_cores_to_use_for_vm.at(execution_host);
                                 WRENCH_INFO("Custom action creating a %lu-core VM", num_cores);
                                 auto my_vm = cloud_service->createVM(num_cores, 100 * MB);
                                 auto my_vm_cs = cloud_service->startVM(my_vm);
                                 WRENCH_INFO("Custom action creating a job manager");
                                 auto job_manager = action_executor->createJobManager();
                                 WRENCH_INFO("Custom action create a job and submitting it to the VM");
                                 auto job = job_manager->createCompoundJob("custom_job");
                                 job->addSleepAction("custom_sleep", 10);
                                 job_manager->submitJob(job, my_vm_cs);
                                 WRENCH_INFO("Custom action is waiting for its job's completion");
                                 auto event = action_executor->waitForNextEvent();
                                 auto job_completion_event = std::dynamic_pointer_cast<wrench::CompoundJobCompletedEvent>(event);
                                 if (not job_completion_event) {
                                     throw std::runtime_error("Custom action: unexpected event!");
                                 } else {
                                     WRENCH_INFO("Custom action: job has completed!");
                                 }
                             },
                             [](std::shared_ptr<ActionExecutor> action_executor) {
                                 WRENCH_INFO("Custom action terminating");
                             });

        /* Submit the job to the bare-metal service compute service, and forcing the action "powerful"
         * to run on ComputeHost2 by passing (optional) service-specific arguments */
        WRENCH_INFO("Submitting the job %s to the bare-metal service", job->getName().c_str());
        job_manager->submitJob(job, bm_cs, {{"powerful","ComputeHost2"}});

        /* Wait for an execution event */
        auto event = this->waitForNextEvent();
        if (auto job_completion_event = std::dynamic_pointer_cast<wrench::CompoundJobCompletedEvent>(event)) {
            auto completed_job = job_completion_event->job;
            WRENCH_INFO("Job %s has completed!", completed_job->getName().c_str());
            WRENCH_INFO("It had %lu actions:", completed_job->getActions().size());
            for (auto const &action : completed_job->getActions()) {
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

        WRENCH_INFO("Controller terminating");
        return 0;
    }

}
