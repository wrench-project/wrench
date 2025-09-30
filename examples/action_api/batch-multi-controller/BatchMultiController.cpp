/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** This simulator simulates the execution of two batch compute services, each used by their own controller.
 ** The controllers "talk to each other" to delegate some jobs to the other controller.  There is a third
 ** controller that submits job requests to either of the previous two controllers.
 **
 ** The compute platform comprises seven hosts:
 **   - UserHost: runs the execution controller that creates job requests and sends them to one of the batch controllers
 **   - Batch1HeadNode, Batch1ComputeNode1,and Batch1ComputeNode2: the first batch system, which runs on Batch1HeadNode, along with a controller
 **   - Batch2HeadNode, Batch2ComputeNode1,and Batch2ComputeNode2: the second batch system, which runs on Batch2HeadNode, along with a controller
 **
 ** Example invocation of the simulator for 10 jobs, with no logging:
 **    ./wrench-example-batch-bag-of-actions 10 ./two_batch_services.xml
 **
 ** Example invocation of the simulator for 10 jobs, with only execution controller logging:
 **    ./wrench-example-batch-bag-of-actions 10 ./two_batch_services.xml --log=batch_service_controller.threshold=info --log=job_generation_controller.threshold=info
 **
 ** Example invocation of the simulator for 10 jobs, with full logging:
 **    ./wrench-example-batch-bag-of-actions 10 ./two_batch_services.xml --wrench-full-log
 **/


#include <iostream>
#include <wrench.h>

#include "JobGenerationController.h"
#include "BatchServiceController.h"

/**
 * @brief The Simulator's main function
 *
 * @param argc: argument count
 * @param argv: argument array
 * @return 0 on success, non-zero otherwise
 */
int main(int argc, char **argv) {

    /* Initialize the simulation. */
    auto simulation = wrench::Simulation::createSimulation();
    simulation->init(&argc, argv);

    /* Parsing of the command-line arguments for this WRENCH simulation */
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <a number of jobs> <xml platform file> [--log=batch_service_controller.threshold=info --log=job_generation_controller.threshold=info]" << std::endl;
        exit(1);
    }

    /* Reading and parsing the platform description file, written in XML following the SimGrid-defined DTD,
     * to instantiate the simulated platform */
    std::cerr << "Instantiating simulated platform..." << std::endl;
    simulation->instantiatePlatform(argv[2]);

    /* Parse the first command-line argument (number of jobs) */
    int num_jobs;
    try {
        num_jobs = std::atoi(argv[1]);
    } catch (std::invalid_argument &e) {
        std::cerr << "Invalid number of jobs (" << e.what() << ")\n";
        exit(1);
    }

    /* Instantiate two batch compute service, and add them to the simulation. */
    std::cerr << "Instantiating a batch compute service on ComputeHost..." << std::endl;
    auto batch_service_1 = simulation->add(new wrench::BatchComputeService(
            "Batch1HeadNode", {"Batch1ComputeNode1", "Batch1ComputeNode2"}, "", {}, {}));
    auto batch_service_2 = simulation->add(new wrench::BatchComputeService(
            "Batch2HeadNode", {"Batch2ComputeNode1", "Batch2ComputeNode2"}, "", {}, {}));

    /* Instantiate an execution controller for each batch compute service */
    auto batch_controller_1 = simulation->add(
        new wrench::BatchServiceController("Batch1HeadNode", batch_service_1));
    auto batch_controller_2 = simulation->add(
        new wrench::BatchServiceController("Batch2HeadNode", batch_service_2));
    batch_controller_1->setPeer(batch_controller_2);
    batch_controller_1->setDaemonized(true);
    batch_controller_2->setPeer(batch_controller_1);
    batch_controller_2->setDaemonized(true);

    /* Instantiate an execution controller that will generate jobs */
    auto wms = simulation->add(new wrench::JobGenerationController(
        "UserHost", num_jobs, {batch_controller_1, batch_controller_2}));
    batch_controller_1->setJobOriginator(wms);
    batch_controller_2->setJobOriginator(wms);

    /* Launch the simulation. This call only returns when the simulation is complete. */
    std::cerr << "Launching the Simulation..." << std::endl;
    try {
        simulation->launch();
    } catch (std::runtime_error &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    std::cerr << "Simulation done!" << std::endl;

    return 0;
}
