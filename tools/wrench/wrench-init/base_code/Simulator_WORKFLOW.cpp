/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#define GFLOP (1000.0 * 1000.0 * 1000.0)
#define MBYTE (1000.0 * 1000.0)
#define GBYTE (1000.0 * 1000.0 * 1000.0)

#include <iostream>
#include <wrench-dev.h>

#include "Controller.h"

/**
 * @brief Functor to instantiate a simulated platform, instead of
 * loading it from an XML file. This function directly uses SimGrid's s4u API
 * (see the SimGrid documentation). This function creates a platform that's
 * identical to that described in the file platform.xml located in the ../data/.
 */
namespace sg4 = simgrid::s4u;
class PlatformCreator {

public:
    explicit PlatformCreator() {}

    void operator()() const {
        create_platform();
    }

private:

    void create_platform() const {
        // Get the top-level zone
        auto zone = simgrid::s4u::Engine::get_instance()->get_netzone_root();

        // Create the UserHost host with its disk
        auto user_host = zone->add_host("UserHost", "10Gf");
        user_host->set_core_count(1);
        auto user_host_disk = user_host->add_disk("hard_drive",
                                                   "100MBps",
                                                   "100MBps");
        user_host_disk->set_property("size", "50000GiB");
        user_host_disk->set_property("mount", "/");

        // Create the HeadHost host with its disk
        auto head_host = zone->add_host("HeadHost", "10Gf");
        head_host->set_core_count(1);
        auto head_host_disk = head_host->add_disk("hard_drive",
                                                   "100MBps",
                                                   "100MBps");
        head_host_disk->set_property("size", "50000GiB");
        head_host_disk->set_property("mount", "/");

        // Create a ComputeHost
        auto compute_host = zone->add_host("ComputeHost", "100Gf");
        compute_host->set_core_count(10);
        auto compute_host_disk = compute_host->add_disk("hard_drive",
                                                   "100MBps",
                                                   "100MBps");
        compute_host_disk->set_property("size", "5000GiB");
        compute_host_disk->set_property("mount", "/");

        // Create a single network link
        auto network_link = zone->add_link("network_link", "50MBps")->set_latency("20us");

        // Add routes
        {
            sg4::LinkInRoute network_link_in_route{network_link};
            zone->add_route(user_host,
                            compute_host,
                            {network_link_in_route});
        }
        {
            sg4::LinkInRoute network_link_in_route{network_link};
            zone->add_route(user_host,
                            head_host,
                            {network_link_in_route});
        }
        {
            sg4::LinkInRoute network_link_in_route{network_link};
            zone->add_route(head_host,
                            compute_host,
                            {network_link_in_route});
        }

        zone->seal();
    }
};

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
    if ((argc != 1) && ((argc != 3) || (argc == 3 && (std::string(argv[1]).compare("--platform_file"))))) {
        std::cerr << "Usage: " << argv[0] << " [--platform_file <path to XML file>] [--log=controller.threshold=info | --wrench-full-log]" << std::endl;
        exit(1);
    }

    /* Instantiating the platform */
    if (argc == 1) {
        simulation->instantiatePlatform(PlatformCreator());
    } else {
        /* Instantiating the simulated platform */
        simulation->instantiatePlatform(argv[2]);
    }

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
    auto task1_input = wrench::Simulation::addFile("task1_input", 50 * MBYTE);
    auto task1_output = wrench::Simulation::addFile("task1_output", 20 * MBYTE);
    task1->addInputFile(task1_input);
    task1->addOutputFile(task1_output);
    auto task2_output = wrench::Simulation::addFile("task2_output", 30 * MBYTE);
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

    /* Instantiate a storage service on the user's host */
    auto storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
            "UserHost", {"/"}, {}, {}));

    /* Instantiate a bare-metal compute service on the platform */
    auto baremetal_service = simulation->add(new wrench::BareMetalComputeService(
            "HeadHost", {"ComputeHost"}, "", {}, {}));

    /* Instantiate an execution controller */
    auto controller = simulation->add(
            new wrench::Controller(workflow, baremetal_service, storage_service, "UserHost"));

    /* Stage input files on the storage service */
    for (auto const &f: workflow->getInputFiles()) {
        storage_service->createFile(f);
    }

    /* Launch the simulation */
    simulation->launch();

    /* Print task execution timelines */
    std::vector<std::shared_ptr<wrench::WorkflowTask>> tasks = {task1, task2, task3, task4};
    for (auto const &t: tasks) {
        printf("Task %s: %.2fs - %.2fs\n", t->getID().c_str(), t->getStartDate(), t->getEndDate());
    }

    return 0;
}
