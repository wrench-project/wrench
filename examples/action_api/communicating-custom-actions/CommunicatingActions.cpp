/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** This simulator simulates the execution of a multi-action job where
 ** the actions communicator using collective communications.
 **
 ** Example invocation of the simulator with no logging:
 **    ./wrench-example-communicating-actions ./dragonfly_cluster.xml
 **
 ** Example invocation of the simulator with only WMS logging:
 **    ./wrench-example-communicating-actions ./dragonfly_cluster.xml --log=custom_controller.threshold=info
 **
 ** Example invocation of the simulator with full logging:
 **    ./wrench-example-communicating-actions ./dragonfly_cluster.xml --wrench-full-log
 **/

#include <iostream>
#include <wrench.h>

#include "CommunicatingActionsController.h"// Controller implementation

#define KB 1000.0
#define MB (1000 * KB)
#define GB (1000 * MB)
#define GB (1000 * MB)
#define TB (1000 * GB)
#define MBps MB

/**
 * @brief The Simulator's main function
 *
 * @param argc: argument count
 * @param argv: argument array
 * @return 0 on success, non-zero otherwise
 */
int main(int argc, char **argv) {

    /*
     * Create a WRENCH simulation object
     */
    auto simulation = wrench::Simulation::createSimulation();

    /* Initialize the simulation, which may entail extracting WRENCH-specific and
     * Simgrid-specific command-line arguments that can modify general simulation behavior.
     * Two special command-line arguments are --help-wrench and --help-simgrid, which print
     * details about available command-line arguments. */
    simulation->init(&argc, argv);

    /* Parsing of the command-line arguments for this WRENCH simulation */
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <xml platform file> [--log=custom_controller.threshold=info]" << std::endl;
        exit(1);
    }

    /* Reading and parsing the platform description file, written in XML following the SimGrid-defined DTD,
     * to instantiate the simulated platform */
    std::cerr << "Instantiating simulated platform..." << std::endl;
    simulation->instantiatePlatform(argv[1]);

    /* Get the list of available host names */
    auto host_list = simulation->getHostnameList();

    /* Pick one of those hosts as the head node */
    auto head_node = host_list.at(0);

    /* Instantiate a batch compute service */
    std::cerr << "Instantiating a batch compute service..." << std::endl;
    auto batch_service = simulation->add(new wrench::BatchComputeService(
            head_node, host_list, "", {}, {}));

    /* Create a new disk on the platform */
    wrench::Simulation::createNewDisk(head_node, "disk0", 100 * MBps, 50 * MBps, 50 * TB, "/");

    /* Instantiate a storage service on the head node, with a 50MB buffer size */
    std::cerr << "Instantiating a SimpleStorageService..." << std::endl;
    auto storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
            head_node, {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "50MB"}}, {}));

    /* Instantiate an execution execution_controller to be stated on UserHost */
    auto wms = simulation->add(
            new wrench::CommunicatingActionsController(batch_service, storage_service, host_list.at(0)));

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
