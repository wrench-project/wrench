/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** An execution controller to execute a workflow
 **/

#define GFLOP (1000.0 * 1000.0 * 1000.0)
#define MBYTE (1000.0 * 1000.0)
#define GBYTE (1000.0 * 1000.0 * 1000.0)

#include <iostream>

#include "Controller.h"

WRENCH_LOG_CATEGORY(controller, "Log category for Controller");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param bare_metal_compute_service: a set of compute services available to run actions
     * @param storage_services: a set of storage services available to store files
     * @param hostname: the name of the host on which to start the WMS
     */
    Controller::Controller(const std::shared_ptr<BareMetalComputeService> &bare_metal_compute_service,
                           const std::shared_ptr<SimpleStorageService> &storage_service,
                           const std::string &hostname) :
            ExecutionController(hostname,"controller"),
            bare_metal_compute_service(bare_metal_compute_service), storage_service(storage_service) {}

    /**
     * @brief main method of the Controller
     *
     * @return 0 on completion
     *
     * @throw std::runtime_error
     */
    int Controller::main() {

        /* Set the logging output to GREEN */
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);
        WRENCH_INFO("Controller starting");

        /* Create a files */
        auto some_file = wrench::Simulation::addFile("some_file", 1 * GBYTE);
        auto some_other_file = wrench::Simulation::addFile("some_other_file", 2 * GBYTE);
        this->storage_service->createFile(some_file, wrench::FileLocation::LOCATION(this->storage_service));

        /* Create a job manager so that we can create/submit jobs */
        auto job_manager = this->createJobManager();

        WRENCH_INFO("Creating a compound job with a file read action followed by a compute action");
        auto job1 = job_manager->createCompoundJob("job1");
        auto fileread = job1->addFileReadAction("fileread", some_file, wrench::FileLocation::LOCATION(this->storage_service));
        auto compute = job1->addComputeAction("compute", 100 * GFLOP, 50 * MBYTE, 1, 3, wrench::ParallelModel::AMDAHL(0.8));
        job1->addActionDependency(fileread, compute);

        WRENCH_INFO("Creating a compound job with a file write action and a (simultaneous) sleep action");
        auto job2 = job_manager->createCompoundJob("job2");
        auto filewrite = job2->addFileWriteAction("filewrite", some_other_file, wrench::FileLocation::LOCATION(this->storage_service));
        auto sleep = job2->addSleepAction("sleep", 20.0);

        WRENCH_INFO("Making the second job depend on the first one");
        job2->addParentJob(job1);

        WRENCH_INFO("Submitting both jobs to the bare-metal compute service");

        job_manager->submitJob(job1, this->bare_metal_compute_service);
        job_manager->submitJob(job2, this->bare_metal_compute_service);

        WRENCH_INFO("Waiting for an execution event...");
        this->waitForAndProcessNextEvent();
        WRENCH_INFO("Waiting for an execution event...");
        this->waitForAndProcessNextEvent();

        WRENCH_INFO("Execution complete!");

        std::vector<std::shared_ptr<wrench::Action>> actions = {fileread, compute, filewrite, sleep};
        for (auto const &a : actions) {
            printf("Action %s: %.2fs - %.2fs\n", a->getName().c_str(), a->getStartDate(), a->getEndDate());
        }

        return 0;
    }

    /**
     * @brief Process a compound job completion event
     *
     * @param event: the event
     */
    void Controller::processEventCompoundJobCompletion(std::shared_ptr<CompoundJobCompletedEvent> event) {
        /* Retrieve the job that this event is for */
        auto job = event->job;
        /* Print info about all actions in the job */
        WRENCH_INFO("Notified that compound job %s has completed:", job->getName().c_str());

    }
}
