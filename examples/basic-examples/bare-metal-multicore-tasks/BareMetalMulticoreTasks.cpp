/**
 * Copyright (c) 2017-2018. The WRENCH Team.
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
 ** There are four tasks in this workflow, none of which has input/output files, and each of which
 ** uses a different parallel model:
 **   - task_default: the default (ideal) parallel model
 **   - task_constant_efficiency: a model where the task has a specified constant efficiency
 **   - task_amdahl: a model that specifies the fraction of the task's work that can be parallelized perfectly
 **
 ** The compute platform comprises two hosts, WMSHost and ComputeHost. On WMSHost runs a  a WMS
 ** (defined in class OneTaskAtATimeWMS). On ComputeHost runs a bare metal
 ** compute service, that has access to the 10 cores of that host. Once the simulation is done,
 ** the execution time of each workflow task is printed.
 **
 ** Example invocation of the simulator with no logging:
 **    ./wrench-example-bare-metal-multicore-tasks./two_hosts.xml
 **
 ** Example invocation of the simulator with only WMS logging:
 **    ./wrench-example-bare-metal-multicore-tasks./two_hosts.xml --log=custom_wms.threshold=info
 **
 ** Example invocation of the simulator with full logging:
 **    ./wrench-example-bare-metal-multicore-tasks ./two_hosts.xml --wrench-full-log
 **/


#include <iostream>
#include <wrench.h>

#include "OneTaskAtATimeWMS.h" // WMS implementation

#define GFLOP (1000.0 * 1000.0 * 1000.0)

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

    /* Add a task with the default parallel model behavior */
    auto task_default = workflow.addTask("task_default", 10 * GFLOP, 1, 10, 0.0);

    /* Add a task with the "constant efficiency" parallel model behavior */
    auto task_constant_efficiency = workflow.addTask("task_constant_efficiency", 10 * GFLOP, 1, 10, 0.0);
    task_constant_efficiency->setParallelModel(wrench::ParallelModel::CONSTANTEFFICIENCY(0.8));

    /* Add a task with the "amdahl" parallel model behavior */
    auto task_amdahl = workflow.addTask("task_amdahl", 10 * GFLOP, 1, 10, 0.0);
    task_amdahl->setParallelModel(wrench::ParallelModel::AMDAHL(0.8));


    /* Instantiate a bare-metal compute service, and add it to the simulation.
     * A wrench::bare_metal is an abstraction of a compute service that corresponds
     * to a software infrastructure that can execute tasks on hardware resources.
     * This particular service is started on ComputeHost and has no scratch storage space (mount point argument = "").
     * This means that tasks running on this service will access data only from remote storage services. */
    std::cerr << "Instantiating a bare_metal on ComputeHost..." << std::endl;
    auto baremetal_service = simulation.add(new wrench::BareMetalComputeService(
            "ComputeHost", {"ComputeHost"}, "", {}, {}));

    /* Instantiate a WMS, to be stated on WMSHost, which is responsible
     * for executing the workflow. */

    auto wms = simulation.add(
            new wrench::OneTaskAtATimeWMS({baremetal_service}, "WMSHost"));

    /* Associate the workflow to the WMS */
    wms->addWorkflow(&workflow);

    /* Launch the simulation. This call only returns when the simulation is complete. */
    std::cerr << "Launching the Simulation..." << std::endl;
    try {
        simulation.launch();
    } catch (std::runtime_error &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    std::cerr << "Simulation done!" << std::endl;

    /* Print all task execution information */
    std::vector<wrench::WorkflowTask*> tasks = {task_default, task_constant_efficiency, task_amdahl};
    for (auto const &task : tasks) {
        std::cerr << "Task " + task->getID() + ": ";
        std::cerr << task->getFlops() << " on " << task->getNumCoresAllocated() <<" ran in ";
        std::cerr << (task->getComputationEndDate() - task->getComputationStartDate()) << " seconds"  << std::endl;
    }

    return 0;
}
