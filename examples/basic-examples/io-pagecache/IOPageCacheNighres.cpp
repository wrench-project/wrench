/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

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


void export_output_single(wrench::SimulationOutput output, int num_tasks, std::string filename) {
    auto read_start = output.getTrace<wrench::SimulationTimestampFileReadStart>();
    auto read_end = output.getTrace<wrench::SimulationTimestampFileReadCompletion>();
    auto write_start = output.getTrace<wrench::SimulationTimestampFileWriteStart>();
    auto write_end = output.getTrace<wrench::SimulationTimestampFileWriteCompletion>();
    auto task_start = output.getTrace<wrench::SimulationTimestampTaskStart>();
    auto task_end = output.getTrace<wrench::SimulationTimestampTaskCompletion>();

    FILE *log_file = fopen(filename.c_str(), "w");
    fprintf(log_file, "type, start, end, duration\n");

    for (int i = 0; i < num_tasks; i++) {
        std::cerr << "Task " << read_end[i]->getContent()->getTask()->getID()
                  << " completed at " << task_end[i]->getDate()
                  << " in " << task_end[i]->getDate() - task_start[i]->getDate()
                  << std::endl;


        fprintf(log_file, "read, %lf, %lf, %lf\n", read_start[i]->getDate(), read_end[i]->getDate(),
                read_end[i]->getDate() - read_start[i]->getDate());
        fprintf(log_file, "write, %lf, %lf, %lf\n", write_start[i]->getDate(), write_end[i]->getDate(),
                write_end[i]->getDate() - write_start[i]->getDate());
    }

    fclose(log_file);
}

int main(int argc, char **argv) {

    wrench::Simulation simulation;
    simulation.init(&argc, argv);

    if (argc < 3) {
        std::cerr << "Usage: " << argv[0]
                  << "<xml_platform_file> <workflow_file> --wrench-pagecache-simulation [--log=custom_wms.threshold=info]"
                  << std::endl;
        exit(1);
    }
    char *platform_file = argv[1];
    char *workflow_file = argv[2];

    std::cerr << "Instantiating simulated platform..." << std::endl;
    simulation.instantiatePlatform(platform_file);

    wrench::Workflow *workflow = wrench::PegasusWorkflowParser::createWorkflowFromDAX(workflow_file, "1Gf");
//    wrench::Workflow *workflow = nighres_workflow(16000000000);

    std::cerr << "Instantiating a SimpleStorageService on host01..." << std::endl;
    auto storage_service = simulation.add(new wrench::SimpleStorageService(
            "host01", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "100000000"}}, {}));

    std::cerr << "Instantiating a bare_metal on ComputeHost..." << std::endl;
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

    std::string mode = "original";
    if (wrench::Simulation::isPageCachingEnabled()) {
        mode = "pagecache";
    }

    {
        std::string filename =  "nighres/dump_nighres_" + mode + "_sim_time.json";
        simulation.getOutput().dumpUnifiedJSON(workflow, filename);
        std::cerr << "Written output to file " + filename << "\n";
    }

//    {
//        std::string filename = "nighres/nighres_" + mode + "_sim_time.csv";
//        export_output_single(simulation.getOutput(), 4, filename);
//        std::cerr << "Written output to file " + filename << "\n";
//    }
//
//    if (wrench::Simulation::isPageCachingEnabled()) {
//        std::string filename = "nighres/nighres_sim_mem.csv";
//        simulation.getMemoryManagerByHost("host01")->export_log(filename);
//        std::cerr << "Written output to file " + filename << "\n";
//    }

    exit(0);
}
