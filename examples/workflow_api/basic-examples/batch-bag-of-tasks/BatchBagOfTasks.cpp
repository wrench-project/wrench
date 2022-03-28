/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** This simulator simulates the execution of a bag-of-tasks workflow, that is, of a workflow
 ** in which each task has its own input file and its own output file, and tasks can be
 ** executed completely independently
 **
 **   InputFile #0 -> Task #0 -> OutputFile #1
 **   ...
 **   InputFile #n -> Task #n -> OutputFile #n
 **
 ** The compute platform comprises four hosts, WMSHost,  BatchHeadNode, BatchNode1, and
 **  BatchNode2. On WMSHost runs a simple storage
 ** service and a WMS (defined in class TwoTasksAtATimeBatchWMS). On BatchHeadNode runs a batch_standard_and_pilot_jobs
 ** service, that has access to two hosts: BatchNode1 and BatchNode2. Once the simulation is done,
 ** the completion time of each workflow task is printed.
 **
 ** Example invocation of the simulator for a 10-task workflow, with no logging:
 **    ./wrench-example-batch_standard_and_pilot_jobs-bag-of-tasks 10 ./four_hosts.xml
 **
 ** Example invocation of the simulator for a 10-task workflow, with only WMS logging:
 **    ./wrench-example-batch_standard_and_pilot_jobs-bag-of-tasks 10 ./four_hosts.xml --log=custom_wms.threshold=info
 **
 ** Example invocation of the simulator for a 6-task workflow with full logging:
 **    ./wrench-example-batch_standard_and_pilot_jobs-bag-of-tasks 6 ./four_hosts.xml --wrench-full-log
 **/


#include <iostream>
#include <wrench.h>

#include "TwoTasksAtATimeBatchWMS.h"// WMS implementation

constexpr double TFLOP = 1000000000000.0;

/**
 * @brief The Simulator's main function
 *
 * @param argc: argument count
 * @param argv: argument array
 * @return 0 on success, non-zero otherwise
 */
int main(int argc, char **argv) {

    /* Create a WRENCH simulation object */
    auto simulation = wrench::Simulation::createSimulation();
    ;

    /* Initialize the simulation, which may entail extracting WRENCH-specific and
     * Simgrid-specific command-line arguments that can modify general simulation behavior.
     * Two special command-line arguments are --help-wrench and --help-simgrid, which print
     * details about available command-line arguments. */
    simulation->init(&argc, argv);

    /* Parsing of the command-line arguments for this WRENCH simulation */
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <number of tasks> <xml platform file> [--log=custom_wms.threshold=info]" << std::endl;
        exit(1);
    }

    /* Reading and parsing the platform description file, written in XML following the SimGrid-defined DTD,
     * to instantiate the simulated platform */
    std::cerr << "Instantiating simulated platform..." << std::endl;
    simulation->instantiatePlatform(argv[2]);

    /* Parse the first command-line argument (number of tasks) */
    int num_tasks = 0;
    try {
        num_tasks = std::atoi(argv[1]);
    } catch (std::invalid_argument &e) {
        std::cerr << "Invalid number of tasks (" << e.what() << ")\n";
        exit(1);
    }

    /* Declare a workflow */
    auto workflow = wrench::Workflow::createWorkflow();

    /* Add workflow tasks  and files */
    for (int i = 0; i < num_tasks; i++) {
        /* Create a task: increasing GFlop, 1 to 10 cores, 0.90 parallel efficiency, 10MB memory_manager_service footprint */
        auto task = workflow->addTask("task_" + std::to_string(i), (100 + i * 500) * TFLOP,
                                      1, 10, 1000);
        task->setParallelModel(wrench::ParallelModel::CONSTANTEFFICIENCY(0.9));
        task->addInputFile(workflow->addFile("input_" + std::to_string(i), 10000000));
        task->addOutputFile(workflow->addFile("output_" + std::to_string(i), 10000000));
    }

    /* Instantiate a storage service, and add it to the simulation.
     * A wrench::StorageService is an abstraction of a service on
     * which files can be written and read.  This particular storage service, which is an instance
     * of wrench::SimpleStorageService, is started on WMSHost in the
     * platform , which has an attached disk mounted at "/". The SimpleStorageService
     * is a basic storage service implementation provided by WRENCH.
     * Throughout the simulation execution, input/output files of workflow tasks will be located
     * in this storage service, and accessed remotely by the compute service. Note that the
     * storage service is configured to use a buffer size of 50M when transferring data over
     * the network (i.e., to pipeline disk reads/writes and network revs/sends). */
    std::cerr << "Instantiating a SimpleStorageService on WMSHost..." << std::endl;
    auto storage_service = simulation->add(new wrench::SimpleStorageService(
            "WMSHost", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "50000000"}}, {}));

    /* Instantiate a batch_standard_and_pilot_jobs compute service, and add it to the simulation.
     * A wrench::BatchComputeService is an abstraction of a compute service that corresponds
     * to a batch_standard_and_pilot_jobs scheduled cluster, which responds to job submission requests by placing them
     * in a batch_standard_and_pilot_jobs queue, and granting them exclusive access to compute resources.
     * This particular service is started on BatchProviderHost, uses BatchHost1 and BatchHost2
     * as hardware resources, and has no scratch storage space (mount point argument = "").
     * This means that tasks running on this service will access data only from remote storage services. */
    std::cerr << "Instantiating a BatchComputeService on BatchHeadNode..." << std::endl;
    std::vector<std::string> batch_nodes = {"BatchNode1", "BatchNode2"};
    auto batch_service = simulation->add(new wrench::BatchComputeService(
            "BatchHeadNode", batch_nodes, "", {}, {}));

    /* Instantiate a WMS, to be stated on WMSHost, which is responsible
     * for executing the workflow-> */
    auto wms = simulation->add(
            new wrench::TwoTasksAtATimeBatchWMS(workflow, batch_service, storage_service, "WMSHost"));

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
        simulation->stageFile(f, storage_service);
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
     * many such events there are, and print some information for the first such event.  Also print
     * out how many failures each task has experienced */
    auto trace = simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>();
    for (auto const &item: trace) {
        std::cerr << "Task " << item->getContent()->getTask()->getID() << " completed at hour " << (item->getDate() / 3600);
        std::cerr << " with " << item->getContent()->getTask()->getFailureCount() << " failures)\n";
    }

    return 0;
}
