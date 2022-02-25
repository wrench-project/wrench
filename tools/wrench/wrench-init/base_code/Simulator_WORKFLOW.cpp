/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** This is the main function for a WRENCH simulator. The simulator takes
 ** a input an XML platform description file. It generates a workflow with
 ** a simple diamond structure, instantiates a few services on the platform, and
 ** starts an execution controller to execute the workflow using these services
 ** using a simple greedy algorithm.
 **/

#define GFLOP (1000.0 * 1000.0 * 1000.0)
#define MBYTE (1000.0 * 1000.0)
#define GBYTE (1000.0 * 1000.0 * 1000.0)

#include <iostream>
#include <wrench-dev.h>

#include "Controller.h"

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

    /* Initialize the simulation */
    simulation->init(&argc, argv);

    /* Parsing of the command-line arguments */
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <xml platform file> [--log=controller.threshold=info]" << std::endl;
        exit(1);
    }

    /* Instantiating the simulated platform */
    simulation->instantiatePlatform(argv[1]);

    /* Create a workflow */
    auto workflow = wrench::Workflow::createWorkflow();

    /* Add a workflow task. See examples in the examples/workflow_api/
     * directory for examples of how to create various workflows */
    auto task1 = workflow->addTask("task1", 10 * GFLOP, 2, 10, 2 * GBYTE);
    auto task2 = workflow->addTask("task2", 40 * GFLOP, 2, 10, 4 * GBYTE);
    auto task3 = workflow->addTask("task3", 30 * GFLOP, 2, 10, 4 * GBYTE);
    auto task4 = workflow->addTask("task4", 20 * GFLOP, 2, 10, 2 * GBYTE);

    /* Set a custom parallel efficiency behavior for task 2 */
    task2->setParallelModel(wrench::ParallelModel::AMDAHL(0.8));

    /* Create a couple of workflow files */
    auto task1_input = workflow->addFile("task1_input", 50 * MBYTE);
    auto task1_output = workflow->addFile("task1_output", 20 * MBYTE);
    task1->addInputFile(task1_input);
    task1->addOutputFile(task1_output);
    auto task2_output = workflow->addFile("task2_output", 30 * MBYTE);
    task2->addOutputFile(task2_output);

    /* Create data/control dependencies
     *
     *       task1
     *      /     \
     *   task2   task3
     *      \     /
     *       task4
     */
    workflow->addControlDependency(task1, task2);
    task3->addInputFile(task1_output);
    workflow->addControlDependency(task3, task4);
    task4->addInputFile(task2_output);

    /* Instantiate a storage service on the platform */
    auto storage_service = simulation->add(new wrench::SimpleStorageService(
            "StorageHost", {"/"}, {}, {}));

    /* Instantiate a bare-metal compute service on the platform */
    auto baremetal_service = simulation->add(new wrench::BareMetalComputeService(
            "ComputeHost", {"ComputeHost"}, "", {}, {}));

    /* Instantiate an execution controller */
    auto wms = simulation->add(
            new wrench::Controller(workflow, baremetal_service, storage_service, "UserHost"));

    /* Instantiate a file registry service */
    auto file_registry_service = new wrench::FileRegistryService("UserHost");
    simulation->add(file_registry_service);

    /* Stage input files on the storage service */
    for (auto const &f : workflow->getInputFiles()) {
        simulation->stageFile(f, storage_service);
    }

    /* Launch the simulation */
    simulation->launch();

    /* Print task execution timelines */
    std::vector<std::shared_ptr<wrench::WorkflowTask>> tasks = {task1, task2, task3, task4};
    for (auto const &t : tasks) {
        printf("Task %s: %.2fs - %.2fs\n", t->getID().c_str(), t->getStartDate(), t->getEndDate());
    }

    return 0;
}
