/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** This simulator simulates the execution of three multi-action jobs,
 ** where job3 depends on job 2
 **
 ** The compute platform comprises four hosts, UserHost, StorageHost1, ComputeHost1, CloudHeadHost, and CloudComputeHost1.
 ** On WMSHost runs a WMS (defined in class MultiActionMultiJobWMS). On StorageHost1
 ** runs a storage services. On ComputeHost1 runs a bare-metal
 ** compute service, that has access to the 10 cores of that host.  On CloudHeadHost runs
 ** a cloud service, that has access to the 10 cores of host CloudComputeHost1. This cloud-service
 ** has a scratch space (i.e., a local storage service "mounted" at /scratch/)
 ** Once the simulation is done, various action/job information are printed.
 **
 ** Example invocation of the simulator with no logging:
 **    ./wrench-example-multi-action-multi-job ./four_hosts.xml
 **
 ** Example invocation of the simulator with only WMS logging:
 **    ./wrench-example-multi-action-multi-job ./four_hosts.xml --log=custom_wms.threshold=info
 **
 ** Example invocation of the simulator with full logging:
 **    ./wrench-example-multi-action-multi-job ./four_hosts.xml --wrench-full-log
 **/

#include <iostream>
#include <wrench.h>

#include "MultiActionMultiJobWMS.h" // WMS implementation

/**
 * @brief The Simulator's main function
 *
 * @param argc: argument count
 * @param argv: argument array
 * @return 0 on success, non-zero otherwise
 */
int main(int argc, char **argv) {

    /*
     * Declare a WRENCH simulation object
     */
    wrench::Simulation simulation;

    /* Initialize the simulation, which may entail extracting WRENCH-specific and
     * Simgrid-specific command-line arguments that can modify general simulation behavior.
     * Two special command-line arguments are --help-wrench and --help-simgrid, which print
     * details about available command-line arguments. */
    simulation.init(&argc, argv);

    /* Parsing of the command-line arguments for this WRENCH simulation */
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <xml platform file> [--log=custom_wms.threshold=info]" << std::endl;
        exit(1);
    }

    /* Reading and parsing the platform description file, written in XML following the SimGrid-defined DTD,
     * to instantiate the simulated platform */
    std::cerr << "Instantiating simulated platform..." << std::endl;
    simulation.instantiatePlatform(argv[1]);

    /* Declare a workflow */
    wrench::Workflow workflow;

    /* Instantiate a storage service, and add it to the simulation.
     * A wrench::StorageService is an abstraction of a service on
     * which files can be written and read.  This particular storage service, which is an instance
     * of wrench::SimpleStorageService, is started on StorageHost1 in the
     * platform , which has an attached disk mounted at "/". The SimpleStorageService
     * is a basic storage service implementation provided by WRENCH.
     * Throughout the simulation execution, input/output files of workflow tasks will be located
     * in this storage service, and accessed remotely by the compute service. Note that the
     * storage service is configured to use a buffer size of 50M when transferring data over
     * the network (i.e., to pipeline disk reads/writes and network revs/sends). */
    std::cerr << "Instantiating a SimpleStorageService on StorageHost1..." << std::endl;
    auto storage_service1 = simulation.add(new wrench::SimpleStorageService(
            "StorageHost1", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "50000000"}}, {}));

    /* Instantiate a bare-metal compute service, and add it to the simulation.
     * A wrench::bare_metal_standard_jobs is an abstraction of a compute service that corresponds
     * to a software infrastructure that can execute tasks on hardware resources.
     * This particular service is started on ComputeHost and has no scratch storage space (mount point argument = "").
     * This means that tasks running on this service will access data only from remote storage services. */
    std::cerr << "Instantiating a bare-metal compute service on ComputeHost1..." << std::endl;
    auto baremetal_service = simulation.add(new wrench::BareMetalComputeService(
            "ComputeHost1", {"ComputeHost1"}, "", {}, {}));

    /* Instantiate a cloud compute service, and add it to the simulation.
     * A wrench::CloudComputeService is an abstraction of a compute service that corresponds
     * to a cloud that responds to VM creating requests, and each VM exposes a "bare-metal" compute service.
     * This particular service is started on CloudProviderHost, uses CloudHost1 and CloudHost2
     * as hardware resources, and has access scratch storage space (mount point argument = "/scratch").
     * This means that tasks running on this service will access data only from remote storage services. */
    std::cerr << "Instantiating a cloud compute service on CloudHeadHost..." << std::endl;
    auto cloud_service = simulation.add(new wrench::BareMetalComputeService(
            "CloudHeadHost", {"CloudHost1"}, "/scratch/", {}, {}));

    /* Instantiate an execution execution_controller to be stated on UserHost */
    auto wms = simulation.add(
            new wrench::MultiActionMultiJobController("UserHost", baremetal_service, cloud_service, storage_service);

    /* It is necessary to store, or "stage", input files that only input. The getInputFiles()
     * method of the Workflow class returns the set of all workflow files that are not generated
     * by workflow tasks, and thus are only input files. These files are then staged on the storage service. */
    std::cerr << "Staging task input files..." << std::endl;
    for (auto const &f : workflow.getInputFiles()) {
        simulation.stageFile(f, storage_service1);
    }

    /* Launch the simulation. This call only returns when the simulation is complete. */
    std::cerr << "Launching the Simulation..." << std::endl;
    try {
        simulation.launch();
    } catch (std::runtime_error &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    std::cerr << "Simulation done!" << std::endl;

    /* Simulation results can be examined via simulation.output, which provides access to traces
     * of events. In the code below, we print the  retrieve the trace of all task completion events, print how
     * many such events there are, and print some information for the first such event. */
    auto trace = simulation.getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>();
    for (auto const &item : trace) {
        std::cerr << "Task "  << item->getContent()->getTask()->getID() << " completed at time " << item->getDate()  << std::endl;
    }

    return 0;
}
