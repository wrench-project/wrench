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

#include "CondorWMS.h" // WMS implementation
#include "wrench/tools/pegasus/PegasusWorkflowParser.h"


/**
 ** This simulator simulates the execution of 10 independent tasks on HTCondor,
 ** where HTCondor has access for a batch compute service and a bare-metal compute
 ** service that runs on a VM.
 **
 ** Example invocation of the simulator with useful WMS logging:
 **    ./wrench-example-condor-grid-universe condor-grid-hosts.xml --log=custom_wms.threshold=info
 **/

int main(int argc, char **argv) {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();

    // Parse WRENCH-specific and SimGrid-specific command-line arguments
    simulation->init(&argc, argv);

    // Parse simulator-specific command-line arguments
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <XML platform file>\n";
        exit(1);
    }

    // Initialize the platform with the XML file
    simulation->instantiatePlatform(argv[1]);

    // Create a "workflow" of independent 5-core tasks, each with some input file and an output file
    long num_tasks = 10;
    auto workflow = new wrench::Workflow();
    for (int i=0; i < num_tasks; i++) {
        auto task   = workflow->addTask("task_" + std::to_string(i), 1000.0 *1000.0*1000.00*1000.00, 5, 5, 0);
        auto input  = workflow->addFile("task_" + std::to_string(i) + ".in", 100.0*1000*1000);
        auto output = workflow->addFile("task_" + std::to_string(i) + ".out", 100.0*1000*1000);
        task->addInputFile(input);
        task->addOutputFile(output);
    }

    // Create a storage service on the WMS host, that will host all data
    auto local_ss = simulation->add(new wrench::SimpleStorageService("WMSHost", {"/"}));

    // Create a 4-node batch service
    auto batch_cs = simulation->add(new wrench::BatchComputeService(
            "BatchHeadNode",
            {"BatchNode1", "BatchNode2", "BatchNode3", "BatchNode4"},
            "/scratch_batch",
            {},
            {}));

    // Create a 2-node cloud service
    auto cloud_cs = simulation->add(new wrench::CloudComputeService(
            "CloudHeadNode",
            {"CloudNode1", "CloudNode2"},
            "/scratch_cloud"));

    // Create a HTCondor service that has access to the BatchComputeService (the WMS
    // will create VMs on the CloudCompute Service, which will expose BareMetalComputeService instances
    // that will be added to the HTCondor service). Below we set all possible overhead values (see documentation).
    auto htcondor_cs = simulation->add(
            new wrench::HTCondorComputeService(
                    "BatchHeadNode",
                    {batch_cs},
                    {
                            {wrench::HTCondorComputeServiceProperty::NEGOTIATOR_OVERHEAD, "1.0"},
                            {wrench::HTCondorComputeServiceProperty::GRID_PRE_EXECUTION_DELAY, "10.0"},
                            {wrench::HTCondorComputeServiceProperty::GRID_POST_EXECUTION_DELAY, "10.0"},
                            {wrench::HTCondorComputeServiceProperty::NON_GRID_PRE_EXECUTION_DELAY, "5.0"},
                            {wrench::HTCondorComputeServiceProperty::NON_GRID_POST_EXECUTION_DELAY, "5.0"}
                    },
                    {}));

    // Set the default local storage service
    std::dynamic_pointer_cast<wrench::HTCondorComputeService>(htcondor_cs)->setLocalStorageService(local_ss);

    // Create a WMS
    auto wms = simulation->add(
            new wrench::CondorWMS({htcondor_cs, batch_cs, cloud_cs}, {local_ss}, "HTCondorHost"));

    // Add the workflow to the WMS
    wms->addWorkflow(workflow);

    // Create a file registry
    simulation->add(new wrench::FileRegistryService("WMSHost"));

    // Stage the input_file on the storage service
    for (auto const &f : workflow->getInputFiles()) {
        simulation->stageFile(f, local_ss);
    }

    // Run the simulation
    std::cerr << "Launching simulation of execution...\n";
    simulation->launch();
    std::cerr << "Simulation execution finished at time " << workflow->getCompletionDate() << "\n";
    std::cerr << "(run with --log=custom_wms.threshold=info to see WMS log)\n";

    return 0;
}
