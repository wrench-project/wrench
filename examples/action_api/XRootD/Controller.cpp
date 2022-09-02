
/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** An execution controller to execute a multi-action job, where the actions
 ** read files from and XRootD deployment
 **/

#define GFLOP (1000.0 * 1000.0 * 1000.0)
#define MBYTE (1000.0 * 1000.0)
#define GBYTE (1000.0 * 1000.0 * 1000.0)

#include <iostream>
#include <iomanip>
#include <utility>
#include <wrench/services/storage/xrootd/Node.h>
#include "Controller.h"

/*
 * Helper function for pretty-printed output
 */
std::string padLong(long l){
    return (l < 10 ? "0"+std::to_string(l) : std::to_string(l));
}

std::string padDouble(double l){
    return (l < 10 ? "0"+std::to_string(l) : std::to_string(l));
}

std::string formatDate(double time){
    if(time < 0){
        return "Not Started";
    }
    long seconds = (long)time;
    double ms = time - (double)seconds;
    long minutes = seconds / 60;
    seconds %= 60;
    long hours = minutes / 60;
    minutes %= 60;
    long days = hours / 24;
    hours %= 24;

    return std::to_string(days)+"-"+padLong(hours)+':'+padLong(minutes)+':'+padDouble((double)seconds+ms);
}

WRENCH_LOG_CATEGORY(controller, "Log category for Controller");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param bare_metal_compute_service: a bare-metal compute services available to run actions
     * @param root: The root node of an XRootD deployment
     * @param xrootd_manager: The XRootD manager
     * @param hostname: the name of the host on which to start the Controller
     */
    Controller::Controller(const std::shared_ptr<BareMetalComputeService> &bare_metal_compute_service,
                           XRootD::XRootDDeployment *xrootd_deployment,
                           const std::string &hostname) : ExecutionController(hostname, "controller"),
                                                          bare_metal_compute_service(bare_metal_compute_service),
                                                          xrootd_deployment(xrootd_deployment) {}

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

        /* Add a bunch of 1-byte files to the simulation, which will
         * then be stored at storage servers in the XRootD tree
         */
        auto some_file = wrench::Simulation::addFile("some_file", 12 * MBYTE);

        /* This is the XRootD tree that was constructed before the simulation was launched.
         * We will create replicas of the "some_file" file at nodes leaf4, leaf8, leaf9, which
         * we marked with a '*' below.

            root
         /   |   \
    leaf1  leaf2  super1
                 /   |   \
            leaf3  leaf4*  super2
                         /   |   \
                    leaf5  leaf6  super3
                                 /   |   \
                            leaf7  leaf8*  super4
                                         /   |   \
                                    leaf9*  leaf10  leaf11
    */

        auto root = this->xrootd_deployment->getRootSupervisor();

        /*
         * Let's create the replicas of the "some_file" file here and there
         */

        WRENCH_INFO("Creating file replicas in the XRootD tree");
        auto super1 = root->getChild(2);
        auto leaf4 = super1->getChild(1);
        auto super2 = super1->getChild(2);
        auto super3 = super2->getChild(2);
        auto leaf8 = super3->getChild(1);
        auto leaf9 = super3->getChild(2)->getChild(0);

        leaf4->createFile(some_file);
        leaf8->createFile(some_file);
        leaf9->createFile(some_file);

        /* Create a job manager so that we can create/submit jobs */
        auto job_manager = this->createJobManager();

        /* Create a compound job that just reads the file */
        WRENCH_INFO("Submitting a job that should successfully read the file from XRootD");
        auto job1 = job_manager->createCompoundJob("job1");
        auto fileread1 = job1->addFileReadAction("fileread1", some_file, root);

        /* Submit the job that will succeed */
        job_manager->submitJob(job1, this->bare_metal_compute_service);

        /* Wait and process the next event, which will be a job success */
        this->waitForAndProcessNextEvent();

        /* Delete the file replica at leaf4 and submit a similar job, which now will fail
         * because the root's cache still says that the file should be at leaf 4 */
        WRENCH_INFO("Deleting the file replica at leaf4");
        leaf4->deleteFile(some_file);

        WRENCH_INFO("Submitting a job that will fail to read the file from XRootD");
        auto job2 = job_manager->createCompoundJob("job2");
        auto fileread2 = job2->addFileReadAction("fileread2", some_file, root);
        job_manager->submitJob(job2, this->bare_metal_compute_service);
        this->waitForAndProcessNextEvent();

        /* Sleep 2 hours, which is long enough for the XRootD cache to be cleared (based
         * on the {wrench::XRootD::Property::CACHE_MAX_LIFETIME,"3600"} property passed
         * to the XRootDDeployment constructor in Simulator.cpp
         */
        WRENCH_INFO("Sleeping 2 hours, letting the XRootD cache be cleared");
        wrench::Simulation::sleep(7200);

        /* Submit yet another similar job that will succeed because now that the cache
         * has been cleared, the XRootD root will search for the file
         */
        WRENCH_INFO("Submitting a job that should successfully read the file from XRootD");
        auto job3 = job_manager->createCompoundJob("job3");
        auto fileread3 = job3->addFileReadAction("fileread3", some_file, root);
        job_manager->submitJob(job3, this->bare_metal_compute_service);
        this->waitForAndProcessNextEvent();

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
        WRENCH_INFO("Notified that compound job %s has successfully completed", job->getName().c_str());
    }

    /**
     * @brief Process a compound job completion event
     *
     * @param event: the event
     */
    void Controller::processEventCompoundJobFailure(std::shared_ptr<CompoundJobFailedEvent> event) {
        /* Retrieve the job that this event is for */
        auto job = event->job;
        auto action = *(job->getActions().begin());

        WRENCH_INFO("Notified that compound job %s has failed (%s)", job->getName().c_str(),
                    action->getFailureCause()->toString().c_str());
    }

}// namespace wrench
