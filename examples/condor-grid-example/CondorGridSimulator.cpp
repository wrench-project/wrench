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
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <XML platform file>\n";
        exit(1);
    }


    /* Initialize the platform with the XML file */
    simulation->instantiatePlatform(argv[1]);

    /* Create a "workflow" of 10 independent 5-core tasks, each with some input file and an output file */
    long num_tasks = 10;
    auto workflow = new wrench::Workflow();
    for (int i=0; i < num_tasks; i++) {
        auto task   = workflow->addTask("task_" + std::to_string(i), (i+1) * 1000.00, 5, 5, 0);
        auto input  = workflow->addFile("task_" + std::to_string(i) + ".in", 1000);
        auto output = workflow->addFile("task_" + std::to_string(i) + ".out", 1000);
        task->addInputFile(input);
        task->addOutputFile(output);
    }

    /* Create a storage service on the WMS host, that will host all data */
    auto local_ss = simulation->add(new wrench::SimpleStorageService("WMSHost", {"/"}));

    /* Create a batch service */
    std::vector<std::string> batch_nodes = {"BatchNode1", "BatchNode2", "BatchNode3", "BatchNode4"};
    auto batch_cs = simulation->add(new wrench::BatchComputeService(
            "BatchHeadNode",
            batch_nodes,
            "/scratch_batch"));

    /* Create a cloud service */
    std::vector<std::string> cloud_nodes = {"CloudNode1", "CloudNode2"};
    auto cloud_cs = simulation->add(new wrench::CloudComputeService(
            "CloudHeadNode",
            cloud_nodes,
            "/scratch_cloud"));

    /* Create a HTCondor service that has access to the BatchComputeService (the WMS
     * will create VMs on the CloudCompute Service, which will expose BareMetalComputeService instances
     * that will be added to the HTCondor service */

    std::set<std::shared_ptr<wrench::ComputeService>> condor_compute_resources;
    condor_compute_resources.insert(batch_cs);

    auto htcondor_cs = simulation->add(
            new wrench::HTCondorComputeService(
                    "BatchHeadNode", std::move(condor_compute_resources),
                    {
                            {wrench::HTCondorComputeServiceProperty::SUPPORTS_GRID_UNIVERSE, "true"},
                            {wrench::HTCondorComputeServiceProperty::SUPPORTS_STANDARD_JOBS, "true"}
                    },
                    {}));

    std::dynamic_pointer_cast<wrench::HTCondorComputeService>(htcondor_cs)->setLocalStorageService(local_ss);

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    std::string htcondor_host = "HTCondorHost";
    wms = simulation->add(
            new wrench::CondorWMS({htcondor_cs, batch_cs, cloud_cs}, {local_ss}, htcondor_host));

    wms->addWorkflow(workflow);

    // Create a file registry
    simulation->add(new wrench::FileRegistryService("WMSHost"));

    // Staging the input_file on the storage service

    for (auto const &f : workflow->getInputFiles()) {
        simulation->stageFile(f, local_ss);
    }

    // Running the simulation
    simulation->launch();

    /* Gathering some statistics */
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
