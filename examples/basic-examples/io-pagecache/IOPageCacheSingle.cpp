/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** This simulator simulates the execution of a chain workflow, that is, of a workflow
 ** in which each task has at most a single parent and at most a single child:
 **
 **   File #0 -> Task #0 -> File #1 -> Task #1 -> File #2 -> .... -> Task #n-1 -> File #n
 **
 ** The compute platform comprises two hosts, WMSHost and ComputeHost. On WMSHost runs a simple storage
 ** service and a WMS (defined in class OneTaskAtATimeWMS). On ComputeHost runs a bare metal
 ** compute service, that has access to the 10 cores of that host. Once the simulation is done,
 ** the completion time of each workflow task is printed.
 **
 ** Example invocation of the simulator for a 10-task workflow, with no logging:
 **    ./wrench-example-bare-metal-chain 10 ./two_hosts.xml
 **
 ** Example invocation of the simulator for a 10-task workflow, with only WMS logging:
 **    ./wrench-example-bare-metal-chain 10 ./two_hosts.xml --log=custom_wms.threshold=info
 **
 ** Example invocation of the simulator for a 5-task workflow with full logging:
 **    ./wrench-example-bare-metal-chain 5 ./two_hosts.xml --wrench-full-log
 **/


#include <iostream>
#include <wrench.h>
#include <wrench/services/memory/MemoryManager.h>
#include "ConcurrentPipelineWMS.h" // WMS implementation

/**
 * @brief The Simulator's main function
 *
 * @param argc: argument count
 * @param argv: argument array
 * @return 0 on success, non-zero otherwise
 */

wrench::Workflow *generate_workflow(int num_pipes, int num_tasks, int core_per_task,
                                    long flops, long file_size, long mem_required) {
    wrench::Workflow *workflow = new wrench::Workflow();

    for (int i = 0; i < num_pipes; i++) {

        /* Add workflow tasks */
        for (int j = 0; j < num_tasks; j++) {
            /* Create a task: 1GFlop, single core */
            auto task = workflow->addTask("task_" + std::to_string(i) + "_" + std::to_string(j),
                                          flops, 1, core_per_task, mem_required);
        }

        /* Add workflow files */
        for (int j = 0; j < num_tasks + 1; j++) {
            workflow->addFile("file_" + std::to_string(i) + "_" + std::to_string(j), file_size);
        }

        /* Create tasks and set input/output files for each task */
        for (int j = 0; j < num_tasks; j++) {

            auto task = workflow->getTaskByID("task_" + std::to_string(i) + "_" + std::to_string(j));

            task->addInputFile(workflow->getFileByID("file_" + std::to_string(i) + "_" + std::to_string(j)));
            task->addOutputFile(workflow->getFileByID("file_" + std::to_string(i) + "_" + std::to_string(j + 1)));
        }
    }

    return workflow;
}

void export_output_single(wrench::SimulationOutput output, int num_tasks, std::string filename) {
    auto read_start = output.getTrace<wrench::SimulationTimestampFileReadStart>();
    auto read_end = output.getTrace<wrench::SimulationTimestampFileReadCompletion>();
    auto write_start = output.getTrace<wrench::SimulationTimestampFileWriteStart>();
    auto write_end = output.getTrace<wrench::SimulationTimestampFileWriteCompletion>();
    auto task_start = output.getTrace<wrench::SimulationTimestampTaskStart>();
    auto task_end = output.getTrace<wrench::SimulationTimestampTaskCompletion>();

    FILE *log_file = fopen(filename.c_str(), "w");
    fprintf(log_file, "type, start, end\n");

    for (int i = 0; i < num_tasks; i++) {
        std::cerr << "Task " << read_end[i]->getContent()->getTask()->getID()
                  << " completed at " << task_end[i]->getDate()
                  << " in " << task_end[i]->getDate() - task_start[i]->getDate()
                  << std::endl;


        fprintf(log_file, "read, %lf, %lf\n", read_start[i]->getDate(), read_end[i]->getDate());
        fprintf(log_file, "write, %lf, %lf\n", write_start[i]->getDate(), write_end[i]->getDate());
    }

    fclose(log_file);
}

int main(int argc, char **argv) {

    int num_tasks = 3;

    wrench::Simulation simulation;
    simulation.init(&argc, argv);

    if (argc < 4) {
        std::cerr << "Usage: " << argv[0]
                  << " <file_size_gb> <cpu_time_sec> <xml_platform_file> --wrench-pagecache-simulation [--log=custom_wms.threshold=info]"
                  << std::endl;
        exit(1);
    }

    long file_size_gb = 0;
    try {
        file_size_gb = std::atoi(argv[1]);
    } catch (std::invalid_argument &e) {
        std::cerr << "Invalid file size\n";
        exit(1);
    }
    long mem_req_gb = file_size_gb;

    double cpu_time_sec = 0;
    try {
        cpu_time_sec = std::atof(argv[2]);
    } catch (std::invalid_argument &e) {
        std::cerr << "Invalid cpu time\n";
        exit(1);
    }

    std::cerr << "Instantiating simulated platform..." << std::endl;
    simulation.instantiatePlatform(argv[3]);

    /* Declare a workflow */
    wrench::Workflow *workflow = generate_workflow(1, num_tasks, 1, cpu_time_sec * 1000000000,
                                                   file_size_gb * 1000000000, mem_req_gb * 1000000000);

    std::cerr << "Instantiating a SimpleStorageService on host01..." << std::endl;
    auto storage_service = simulation.add(new wrench::SimpleStorageService(
            "host01", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "100000000"}}, {}));

    std::cerr << "Instantiating a bare_metal_standard_jobs on ComputeHost..." << std::endl;
    auto baremetal_service = simulation.add(new wrench::BareMetalComputeService(
            "host01", {"host01"}, "", {}, {}));

    auto wms = simulation.add(
            new wrench::ConcurrentPipelineWMS({baremetal_service}, {storage_service}, "host01"));

    wms->addWorkflow(workflow);

    std::cerr << "Instantiating a FileRegistryService on host01 ..." << std::endl;
    auto file_registry_service = new wrench::FileRegistryService("host01");
    simulation.add(file_registry_service);

    std::cerr << "Staging task input files..." << std::endl;
    for (auto const &f : workflow->getInputFiles()) {
        simulation.stageFile(f, storage_service);
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

    std::string sub_dir = "original_";
    if (wrench::Simulation::isPageCachingEnabled()) {
        sub_dir = "pagecache_";
    }

    {
        std::string filename = "output_single_" + sub_dir + to_string(file_size_gb) + "gb_sim_time.csv";
        export_output_single(simulation.getOutput(), num_tasks, filename);
        std::cerr << "Written output to file " + filename << "\n";
    }

    if (wrench::Simulation::isPageCachingEnabled()) {
        std::string filename = "output_single_" + sub_dir + to_string(file_size_gb) + "gb_sim_mem.csv";
        simulation.getMemoryManagerByHost("host01")->export_log(filename);
        std::cerr << "Written output to file " + filename << "\n";
    }

    return 0;
}
