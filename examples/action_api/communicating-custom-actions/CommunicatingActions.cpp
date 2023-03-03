/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** This simulator simulates the execution of a one-action job, where the
 ** action is a powerful custom action
 **
 ** The compute platform comprises 6 hosts hosts, UserHost, StorageHost1, StorageHost2, ComputeHost1,
 ** ComputeHost2, CloudHeadHost, and CloudComputeHost.
 ** On UserHost runs a controller (defined in class MultiActionMultiJobController). On StorageHost1 and
 ** StorageHost2 run two storage services.  On ComputeHost1 runs a bare-metal
 ** compute service, that has access to the 10 cores of that host on on ComputeHost2.  On CloudHeadHost runs
 ** a cloud service, that has access to the 10 cores of host CloudComputeHost. This cloud-service
 ** has a scratch space (i.e., a local storage service "mounted" at /scratch/)
 **
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

    /* Instantiate a batch compute service, and add it to the simulation.
     * A wrench::BatchComputeService is an abstraction of a compute service that corresponds
     * to a batch-scheduled compute platform, i.e., managed by a batch scheduler that has jobs
     * in a queue waiting for gaining access to compute resources.
     * This particular service is started on BatchHeadNode and has no scratch storage space (mount point argument = "").
     * This means that actions running on this service will access data files only from remote storage services. */
    std::cerr << "Instantiating a batch compute service..." << std::endl;
    auto batch_service = simulation->add(new wrench::BatchComputeService(
            host_list.at(0), host_list, "", {}, {}));


    /* Instantiate an execution execution_controller to be stated on UserHost */
    auto wms = simulation->add(
            new wrench::CommunicatingActionsController(batch_service, host_list.at(0)));

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
