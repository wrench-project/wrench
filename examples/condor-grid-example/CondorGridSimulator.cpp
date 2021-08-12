/**
 * Copyright (c) 2020. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <iostream>
#include <wrench.h>
#include <pugixml.hpp>

#include "CondorWMS.h" // WMS implementation
#include "CondorTimestamp.h"
#include "wrench/tools/pegasus/PegasusWorkflowParser.h"


/**
 * ./wrench-example-condor-grid-universe [disk-speed in MBps] [bandwidth in MBps, storage service to batch service] ...
 * [Override Pre_execution overhead time in seconds] ...
 * [Override Post_execution overhead time in seconds]
 * @return
 */
int main(int argc, char **argv) {
    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();

    /* Parse WRENCH-specific and SimGrid-specific command-line arguments */
    simulation->init(&argc, argv);

    /* Parse simulator-specific command-line arguments */
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <XML platform file> <# tasks>\n";
        exit(1)
    }

    /* Parse the number of tasks passed as a command-line argument */
    int num_tasks = 0;
    try {
        num_tasks = std::atoi(argv[2]);
    } catch (std::invalid_argument &e) {
        std::cerr << "Invalid number of tasks\n";
        exit(1);
    }

    /* Initialize the platform with the XML file */
    simulation->instantiatePlatform(argv[1]);

    /* Create a workflow of independent 5-core tasks, each with some input file and an output file */
    auto workflow = new wrench::Workflow();
    for (int i=0; i < num_tasks; i++) {
        auto task   = workflow->addTask("task_" + std::to_string(i), (i+1) * 1000.00, 5, 5, 0);
        auto input  = workflow->addFile("task_" + std::to_string(i) + ".in", 1000);
        auto output = workflow->addFile("task_" + std::to_string(i) + ".out", 1000);
        task->addInputFile(input);
        task->addOutputFile(output);
    }

    /* Create a storage service on the WMS host */
    auto local_ss = new wrench::SimpleStorageService("WMSHost", {"/"});

    /* Create a batch service */
    std::vector<std::string> batch_nodes = {"BatchNode1", "BatchNode2", "BatchNode3", "BatchNode4"};
    auto batch_cs = new wrench::BareMetalComputeService(
            "BatchHeadNode",
            batch_nodes,
            "/scratch");

    // Create a HTCondor Service, that can use the batch service and runs on the
    // Cluster Head Node

    std::set<wrench::ComputeService *> condor_compute_resources;
    condor_compute_resources.insert(batch_cs);

    auto htcondor_cs =
            new wrench::HTCondorComputeService(
                    "BatchHeadNode", "mypool", std::move(condor_compute_resources),
                    {
                            {wrench::HTCondorComputeServiceProperty::SUPPORTS_PILOT_JOBS, "false"},
                            {wrench::HTCondorComputeServiceProperty::SUPPORTS_STANDARD_JOBS, "true"},
                            {wrench::HTCondorComputeServiceProperty::SUPPORTS_GRID_UNIVERSE, "true"},
                    },
                    {},
                    batch_service));

    std::dynamic_pointer_cast<wrench::HTCondorComputeService>(compute_service)->setLocalStorageService(storage_service);

    // Create a WMSROBL
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    wms = simulation->add(
            new wrench::CondorWMS({compute_service}, {storage_service}, hostname));

    wms->addWorkflow(grid_workflow);

    // Create a file registry
    simulation->add(new wrench::FileRegistryService(hostname));

    // Staging the input_file on the storage service

    for (auto const &f : grid_workflow->getInputFiles()) {
        simulation->stageFile(f, storage_service);
    }

    // Running a "run a single task" simulation
    simulation->launch();

    simulation->getOutput().dumpUnifiedJSON(grid_workflow, "/tmp/workflow_data.json", false, true, false, false, false, true, false);
    auto start_timestamps = simulation->getOutput().getTrace<wrench::CondorGridStartTimestamp>();
    auto end_timestamp = simulation->getOutput().getTrace<wrench::CondorGridEndTimestamp>().back();
    auto task_finish_timestamps = simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>();

    for (const auto &start_timestamp : start_timestamps) {
        std::cout << "Started: " << start_timestamp->getContent()->getDate() << std::endl;
    }
        std::cout << "Tasks: " << flush;
    for (const auto &task_finish_timestamp : task_finish_timestamps) {
        std::cout << task_finish_timestamp->getContent()->getDate() << ", " << flush;
    }
        std::cout << std::endl;

    std::cout << "Ended: " << end_timestamp->getContent()->getDate() << std::endl;

    return 0;
}
