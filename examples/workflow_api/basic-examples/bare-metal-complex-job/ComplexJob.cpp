/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** This simulator simulates the execution of a one-task workflow, where the task  is:
 **
 **   {InFile #0, InFile #1} -> Task -> {OutFile #0, OutFile #1}
 **
 ** The compute platform comprises four hosts, WMSHost, StorageHost1, StorageHost2, and ComputeHost.
 ** On WMSHost runs a WMS (defined in class ComplexJobWMS). On ComputeHost runs a bare metal
 ** compute service, that has access to the 10 cores of that host. On StorageHost1 and
 ** StorageHost2 run two storage services.  Once the simulation is done,
 ** the completion time of each workflow task is printed.
 **
 ** Example invocation of the simulator with no logging:
 **    ./wrench-example-bare-metal-complex-job ./four_hosts.xml
 **
 ** Example invocation of the simulator with only WMS logging:
 **    ./wrench-example-bare-metal-complex-job ./four_hosts.xml --log=custom_wms.threshold=info
 **
 ** Example invocation of the simulator with full logging:
 **    ./wrench-example-bare-metal-complex-job ./four_hosts.xml --wrench-full-log
 **/

#include <iostream>
#include <wrench.h>

#include "ComplexJobWMS.h"// WMS implementation

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
        std::cerr << "Usage: " << argv[0] << " <xml platform file> [--log=custom_wms.threshold=info]" << std::endl;
        exit(1);
    }

    /* Reading and parsing the platform description file, written in XML following the SimGrid-defined DTD,
     * to instantiate the simulated platform */
    std::cerr << "Instantiating simulated platform..." << std::endl;
    simulation->instantiatePlatform(argv[1]);

    /* Declare a workflow */
    auto workflow = wrench::Workflow::createWorkflow();

    /* Add the workflow task and files */
    auto task = workflow->addTask("task", 10000000000.0, 1, 10, 10000000);
    task->setParallelModel(wrench::ParallelModel::CONSTANTEFFICIENCY(0.9));
    task->addInputFile(workflow->addFile("infile_1", 100000000));
    task->addInputFile(workflow->addFile("infile_2", 10000000));
    task->addOutputFile(workflow->addFile("outfile_1", 200000000));
    task->addOutputFile(workflow->addFile("outfile_2", 5000000));

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
    auto storage_service1 = simulation->add(new wrench::SimpleStorageService(
            "StorageHost1", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "50000000"}}, {}));

    /* Instantiate another one on StorageHost2 */
    std::cerr << "Instantiating a SimpleStorageService on StorageHost2..." << std::endl;
    auto storage_service2 = simulation->add(new wrench::SimpleStorageService(
            "StorageHost2", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "50000000"}}, {}));

    /* Instantiate a bare-metal compute service, and add it to the simulation.
     * A wrench::BareMetalComputeService is an abstraction of a compute service that corresponds
     * to a software infrastructure that can execute tasks on hardware resources.
     * This particular service is started on ComputeHost and has no scratch storage space (mount point argument = "").
     * This means that tasks running on this service will access data only from remote storage services. */
    std::cerr << "Instantiating a bare-metal compute service on ComputeHost..." << std::endl;
    auto baremetal_service = simulation->add(new wrench::BareMetalComputeService(
            "ComputeHost", {"ComputeHost"}, "", {}, {}));

    /* Instantiate a WMS, to be stated on WMSHost, which is responsible
     * for executing the workflow-> */
    auto wms = simulation->add(
            new wrench::ComplexJobWMS(workflow, baremetal_service, storage_service1, storage_service2, "WMSHost"));

    /* Instantiate a file registry service to be started on WMSHost. This service is
     * essentially a replica catalog that stores <file , storage service> pairs so that
     * any service, in particular a WMS, can discover where workflow files are stored. */
    std::cerr << "Instantiating a FileRegistryService on WMSHost ..." << std::endl;
    auto file_registry_service = new wrench::FileRegistryService("WMSHost");
    simulation->add(file_registry_service);

    /* It is necessary to store, or "stage", input files that only input. The getInputFiles()
     * method of the Workflow class returns the set of all workflow files that are not generated
     * by workflow tasks, and thus are only input files. These files are then staged on the storage service. */
    std::cerr << "Staging task input files..." << std::endl;
    for (auto const &f: workflow->getInputFiles()) {
        simulation->stageFile(f, storage_service1);
    }

    /* Launch the simulation. This call only returns when the simulation is complete. */
    std::cerr << "Launching the Simulation..." << std::endl;
    try {
        simulation->launch();
    } catch (std::runtime_error &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    std::cerr << "Simulation done!" << std::endl;

    /* Simulation results can be examined via simulation->getOutput(), which provides access to traces
     * of events. In the code below, we print the  retrieve the trace of all task completion events, print how
     * many such events there are, and print some information for the first such event. */
    auto trace = simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>();
    for (auto const &item: trace) {
        std::cerr << "Task " << item->getContent()->getTask()->getID() << " completed at time " << item->getDate() << std::endl;
    }

    return 0;
}
