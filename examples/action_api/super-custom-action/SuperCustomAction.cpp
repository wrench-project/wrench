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
 **    ./wrench-example-super-custom-action ./four_hosts.xml
 **
 ** Example invocation of the simulator with only WMS logging:
 **    ./wrench-example-super-custom-action ./four_hosts.xml --log=custom_controller.threshold=info
 **
 ** Example invocation of the simulator with full logging:
 **    ./wrench-example-super-custom-action ./four_hosts.xml --wrench-full-log
 **/

#include <iostream>
#include <wrench.h>

#include "SuperCustomActionController.h"// Controller implementation

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

    /* Instantiate a storage service, and add it to the simulation
     * A wrench::StorageService is an abstraction of a service on
     * which files can be written and read.  This particular storage service, which is an instance
     * of wrench::SimpleStorageService, is started on StorageHost in the
     * platform , which has an attached disk mounted at "/". The SimpleStorageService
     * is a basic storage service implementation provided by WRENCH.
     * Throughout the simulation execution, input/output files of workflow tasks will be located
     * in this storage service, and accessed remotely by the compute service. Note that the
     * storage service is configured to use a buffer size of 50MB when transferring data over
     * the network (i.e., to pipeline disk reads/writes and network revs/sends). */
    std::cerr << "Instantiating a SimpleStorageService on StorageHost1..." << std::endl;
    auto storage_service_1 = simulation->add(new wrench::SimpleStorageService(
            "StorageHost1", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "50MB"}}, {}));


    /* Instantiate a bare-metal compute service, and add it to the simulation.
     * A wrench::BareMetalComputeService is an abstraction of a compute service that corresponds
     * to a software infrastructure that can execute tasks on hardware resources.
     * This particular service is started on ComputeHost and has no scratch storage space (mount point argument = "").
     * This means that tasks running on this service will access data only from remote storage services. */
    std::cerr << "Instantiating a bare-metal compute service on ComputeHost1..." << std::endl;
    auto baremetal_service = simulation->add(new wrench::BareMetalComputeService(
            "ComputeHost1", {{"ComputeHost1"}, {"ComputeHost2"}}, "", {}, {}));

    /* Instantiate a cloud compute service, and add it to the simulation.
     * A wrench::CloudComputeService is an abstraction of a compute service that corresponds
     * to a cloud that responds to VM creating requests, and each VM exposes a "bare-metal" compute service.
     * This particular service is started on CloudProviderHost, uses CloudHost1 and CloudHost2
     * as hardware resources, and has access scratch storage space (mount point argument = "/scratch").
     * This means that tasks running on this service will access data only from remote storage services. */
    std::cerr << "Instantiating a cloud compute service on CloudHeadHost..." << std::endl;
    auto cloud_service = simulation->add(new wrench::CloudComputeService(
            "CloudHeadHost", {"CloudHost"}, "/scratch/", {}, {}));

    /* Instantiate an execution execution_controller to be stated on UserHost */
    auto wms = simulation->add(
            new wrench::SuperCustomActionController(baremetal_service, cloud_service, storage_service_1, "UserHost"));

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
