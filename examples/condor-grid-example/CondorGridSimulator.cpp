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

void generatePlatform(std::string platform_file_path, int disk_speed){

    if (platform_file_path.empty()) {
        throw std::invalid_argument("generatePlatform() platform_file_path cannot be empty");
    }
    if (disk_speed <= 0) {
        throw std::invalid_argument("generatePlatform() disk_speed must be greater than 0");
    }


    std::string xml_string = "<?xml version='1.0'?>\n"
                             "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">\n"
                             "<platform version=\"4.1\">\n"
                             "<zone id=\"AS0\" routing=\"Full\">\n"
                             "  <host id=\"DualCoreHost\" speed=\"1f\" core=\"2\" >\n"
                             "      <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">\n"
                             "          <prop id=\"size\" value=\"100GB\"/>\n"
                             "          <prop id=\"mount\" value=\"/\"/>\n"
                             "      </disk>\n"
                             "      <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">\n"
                             "          <prop id=\"size\" value=\"1000000GB\"/>\n"
                             "          <prop id=\"mount\" value=\"/scratch\"/>\n"
                             "      </disk>\n"
                             "  </host>\n"
                             "  <host id=\"QuadCoreHost\" speed=\"1f\" core=\"4\" >\n"
                             "      <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">\n"
                             "          <prop id=\"size\" value=\"100GB\"/>\n"
                             "          <prop id=\"mount\" value=\"/\"/>\n"
                             "      </disk>\n"
                             "      <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">\n"
                             "          <prop id=\"size\" value=\"1000000GB\"/>\n"
                             "          <prop id=\"mount\" value=\"/scratch\"/>\n"
                             "      </disk>\n"
                             "  </host>\n"
                             "  <host id=\"BatchHost1\" speed=\"3050000000f\" core=\"10\">\n"
                             "      <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">\n"
                             "          <prop id=\"size\" value=\"100GB\"/>\n"
                             "          <prop id=\"mount\" value=\"/\"/>\n"
                             "      </disk>\n"
                             "      <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">\n"
                             "          <prop id=\"size\" value=\"1000000GB\"/>\n"
                             "          <prop id=\"mount\" value=\"/scratch\"/>\n"
                             "      </disk>\n"
                             "  </host>\n"
                             "  <host id=\"BatchHost2\" speed=\"3050000000f\" core=\"10\">\n"
                             "    <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">\n"
                             "        <prop id=\"size\" value=\"100GB\"/>\n"
                             "        <prop id=\"mount\" value=\"/\"/>\n"
                             "    </disk>\n"
                             "    <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">\n"
                             "        <prop id=\"size\" value=\"1000000GB\"/>\n"
                             "        <prop id=\"mount\" value=\"/scratch\"/>\n"
                             "    </disk>\n"
                             "  </host>\n"
                             "  <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>\n"
                             "  <link id=\"2\" bandwidth=\"5000GBps\" latency=\"0us\"/>\n"
                             "  <link id=\"3\" bandwidth=\"5000GBps\" latency=\"0us\"/>\n"
                             "  <link id=\"4\" bandwidth=\"5000GBps\" latency=\"0us\"/>\n"
                             "  <link id=\"5\" bandwidth=\"5000GBps\" latency=\"0us\"/>\n"
                             "  <link id=\"6\" bandwidth=\"5000GBps\" latency=\"0us\"/>\n"
                             "  <route src=\"DualCoreHost\" dst=\"QuadCoreHost\"> <link_ctn id=\"1\"/> </route>\n"
                             "  <route src=\"BatchHost1\" dst=\"BatchHost2\"> <link_ctn id=\"2\"/> </route>\n"
                             "  <route src=\"DualCoreHost\" dst=\"BatchHost1\"> <link_ctn id=\"3\"/> </route>\n"
                             "  <route src=\"DualCoreHost\" dst=\"BatchHost2\"> <link_ctn id=\"4\"/> </route>\n"
                             "  <route src=\"QuadCoreHost\" dst=\"BatchHost1\"> <link_ctn id=\"5\"/> </route>\n"
                             "  <route src=\"QuadCoreHost\" dst=\"BatchHost2\"> <link_ctn id=\"6\"/> </route>\n"
                             "</zone>\n"
                             "</platform>\n";

    pugi::xml_document xml_doc;

    if (xml_doc.load_string(xml_string.c_str(), pugi::parse_doctype)) {

        pugi::xml_node host0 = xml_doc.child("platform").child("zone").child("host");
        pugi::xml_node host1 = host0.next_sibling("host");
        pugi::xml_node host2 = host1.next_sibling("host");
        pugi::xml_node host3 = host2.next_sibling("host");

        pugi::xml_node disk0 = host2.child("disk");
        pugi::xml_node disk1 = host2.child("disk").next_sibling("disk");
        pugi::xml_node disk2 = host3.child("disk");
        pugi::xml_node disk3 = host3.child("disk").next_sibling("disk");

        disk0.attribute("read_bw").set_value(std::string(std::to_string(disk_speed) + "MBps").c_str());
        disk0.attribute("write_bw").set_value(std::string(std::to_string(disk_speed) + "MBps").c_str());
        disk1.attribute("read_bw").set_value(std::string(std::to_string(disk_speed) + "MBps").c_str());
        disk1.attribute("write_bw").set_value(std::string(std::to_string(disk_speed) + "MBps").c_str());
        disk2.attribute("read_bw").set_value(std::string(std::to_string(disk_speed) + "MBps").c_str());
        disk2.attribute("write_bw").set_value(std::string(std::to_string(disk_speed) + "MBps").c_str());
        disk3.attribute("read_bw").set_value(std::string(std::to_string(disk_speed) + "MBps").c_str());
        disk3.attribute("write_bw").set_value(std::string(std::to_string(disk_speed) + "MBps").c_str());

        xml_doc.save_file(platform_file_path.c_str());

    } else {
        throw std::runtime_error("something went wrong with parsing xml string");
    }

}


int main(int argc, char **argv) {

    ///TODO Adding arguments for disk bandwidth.
    ///TODO create a python program that can try multiple iterations of the simulation and records execution times from
    ///std out


    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();

    simulation->init(&argc, argv);

    // Pull platform from provided xml file.
    //char *platform_file = argv[1];
    //simulation->instantiatePlatform(platform_file);
    int disk_speed = std::stoi(std::string(argv[1]));
    double pre_execution_overhead;
    double post_execution_overhead;
    if(argc>2){
        pre_execution_overhead = std::stod(std::string(argv[2]));
        post_execution_overhead = std::stod(std::string(argv[3]));
    }



    std::string platform_file_path = "/tmp/platform.xml";
    generatePlatform(platform_file_path, disk_speed);
    simulation->instantiatePlatform(platform_file_path);



    wrench::WorkflowFile *input_file;
    wrench::WorkflowTask *task1;
    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service2 = nullptr;

    // Get a hostname
    std::string hostname = "DualCoreHost";

    wrench::Workflow *grid_workflow;
    std::unique_ptr<wrench::Workflow> workflow_unique_ptr1;
    workflow_unique_ptr1 = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());
    grid_workflow = workflow_unique_ptr1.get();

    input_file = grid_workflow->addFile("input_file", 10.0);
    //task1 = grid_workflow->addTask("grid_task1", 87867450000.0, 1, 1, 0);
    //task1 = grid_workflow->addTask("grid_task1", 3050000000.0, 1, 1, 0);
    task1 = grid_workflow->addTask("grid_task1", 753350000000.0, 1, 1, 0);
    task1->addInputFile(input_file);




    // Create a Storage Service
    storage_service = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/"}));

    //storage_service2 = simulation->add(
    //       new wrench::SimpleStorageService(batchhostname, {"/"})));

    // Create list of compute services
    std::set<wrench::ComputeService *> compute_services;
    std::string execution_host = wrench::Simulation::getHostnameList()[1];
    std::vector<std::string> execution_hosts;
    execution_hosts.push_back(execution_host);
    compute_services.insert(new wrench::BareMetalComputeService(
            execution_host,
            {std::make_pair(
                    execution_host,
                    std::make_tuple(wrench::Simulation::getHostNumCores(execution_host),
                                    wrench::Simulation::getHostMemoryCapacity(execution_host)))},
            "/scratch"));

    wrench::BatchComputeService *batch_service = nullptr;

    if(argc>2){
        batch_service = new wrench::BatchComputeService("BatchHost1",
                                                             {"BatchHost1", "BatchHost2"},
                                                             "/scratch",
                                                             {
                                                                     {wrench::BatchComputeServiceProperty::SUPPORTS_GRID_UNIVERSE, "true"},
                                                                     {wrench::BatchComputeServiceProperty::GRID_PRE_EXECUTION_DELAY, std::to_string(pre_execution_overhead)},
                                                                     {wrench::BatchComputeServiceProperty::GRID_POST_EXECUTION_DELAY, std::to_string(post_execution_overhead)},
                                                             });
    } else {
        batch_service = new wrench::BatchComputeService("BatchHost1",
                                                             {"BatchHost1", "BatchHost2"},
                                                             "/scratch",
                                                             {
                                                                     {wrench::BatchComputeServiceProperty::SUPPORTS_GRID_UNIVERSE, "true"},
                                                             });
    }



    // Create a HTCondor Service
    compute_service = simulation->add(
            new wrench::HTCondorComputeService(
                    hostname, "local", std::move(compute_services),
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
    simulation->stageFile(input_file, storage_service);

    // Running a "run a single task" simulation
    simulation->launch();

    simulation->getOutput().dumpUnifiedJSON(grid_workflow, "/tmp/workflow_data.json", false, true, false, false, false, true, false);
    auto start_timestamps = simulation->getOutput().getTrace<wrench::CondorGridStartTimestamp>();
    auto end_timestamps = simulation->getOutput().getTrace<wrench::CondorGridEndTimestamp>();

    for (const auto &start_timestamp : start_timestamps) {
        std::cerr << "Started: " << start_timestamp->getContent()->getDate() << std::endl;
    }
    for (const auto &end_timestamp : end_timestamps) {
        std::cerr << "Ended: " << end_timestamp->getContent()->getDate() << std::endl;
    }

    return 0;
}
