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

void generatePlatform(std::string platform_file_path,
                      int disk_speed,
                      int batch_bandwidth = 0,
                      long batch_per_node_flops = 0 ){

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
                             "          <prop id=\"size\" value=\"1000000GB\"/>\n"
                             "          <prop id=\"mount\" value=\"/\"/>\n"
                             "      </disk>\n"
                             "      <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">\n"
                             "          <prop id=\"size\" value=\"1000000GB\"/>\n"
                             "          <prop id=\"mount\" value=\"/scratch\"/>\n"
                             "      </disk>\n"
                             "  </host>\n"
                             "  <host id=\"QuadCoreHost\" speed=\"1f\" core=\"4\" >\n"
                             "      <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">\n"
                             "          <prop id=\"size\" value=\"1000000GB\"/>\n"
                             "          <prop id=\"mount\" value=\"/\"/>\n"
                             "      </disk>\n"
                             "      <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">\n"
                             "          <prop id=\"size\" value=\"1000000GB\"/>\n"
                             "          <prop id=\"mount\" value=\"/scratch\"/>\n"
                             "      </disk>\n"
                             "  </host>\n"
                             "  <host id=\"BatchHost1\" speed=\"1666666666f\" core=\"10\">\n"
                             "      <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">\n"
                             "          <prop id=\"size\" value=\"1000000GB\"/>\n"
                             "          <prop id=\"mount\" value=\"/\"/>\n"
                             "      </disk>\n"
                             "      <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">\n"
                             "          <prop id=\"size\" value=\"1000000GB\"/>\n"
                             "          <prop id=\"mount\" value=\"/scratch\"/>\n"
                             "      </disk>\n"
                             "  </host>\n"
                             "  <host id=\"BatchHost2\" speed=\"1666666666f\" core=\"10\">\n"
                             "    <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">\n"
                             "        <prop id=\"size\" value=\"100000GB\"/>\n"
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

        pugi::xml_node link0 = xml_doc.child("platform").child("zone").child("link");
        pugi::xml_node link1 = link0.next_sibling("link");
        pugi::xml_node link2 = link1.next_sibling("link");
        pugi::xml_node link3 = link2.next_sibling("link");
        pugi::xml_node link4 = link3.next_sibling("link");
        pugi::xml_node link5 = link4.next_sibling("link");




        //Setting two links that are between DualCoreHost (storage service) and the two batch service hosts.
        if(batch_bandwidth>0){
            link2.attribute("bandwidth").set_value(std::string(std::to_string(batch_bandwidth) + "MBps").c_str());
            link3.attribute("bandwidth").set_value(std::string(std::to_string(batch_bandwidth) + "MBps").c_str());
            link4.attribute("bandwidth").set_value(std::string(std::to_string(batch_bandwidth) + "MBps").c_str());
            link5.attribute("bandwidth").set_value(std::string(std::to_string(batch_bandwidth) + "MBps").c_str());
        }

        if(batch_per_node_flops>0){
            host2.attribute("speed").set_value(std::string(std::to_string(batch_per_node_flops) + "f").c_str());
            host3.attribute("speed").set_value(std::string(std::to_string(batch_per_node_flops) + "f").c_str());
        }

        xml_doc.save_file(platform_file_path.c_str());

    } else {
        throw std::runtime_error("something went wrong with parsing xml string");
    }

}



/**
 * ./wrench-example-condor-grid-universe [disk-speed in MBps] [bandwidth in MBps, storage service to batch service] ...
 * [Override Pre_execution overhead time in seconds] ...
 * [Override Post_execution overhead time in seconds]
 * @return
 */
int main(int argc, char **argv) {
    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();

    simulation->init(&argc, argv);

    // Pull platform from provided xml file.
    //char *platform_file = argv[1];
    //simulation->instantiatePlatform(platform_file);
    int disk_speed = std::stoi(std::string(argv[1]));
    double pre_execution_overhead;
    double post_execution_overhead;
    int batch_bandwidth;
    long batch_per_node_flops;

    bool diamond_exec = false;
    bool three_task = false;
    bool harpoon_join = false;
    bool ten_split = false;
    bool join_merge_join_merge = false;
    bool three_four_split = false;

    if(argc>2){
        batch_bandwidth = std::stoi(std::string(argv[2]));
    }
    if(argc>3){
        pre_execution_overhead = std::stod(std::string(argv[3]));
        post_execution_overhead = std::stod(std::string(argv[4]));
    }
    if(argc>5){
        batch_per_node_flops = std::stol(std::string(argv[5]));
        //printf("%ld\n",batch_per_node_flops);
    }
    if(argc==7 && std::stod(std::string(argv[6]))==1){
        three_task = true;
    } else if (argc==7 && std::stod(std::string(argv[6]))==2) {
        diamond_exec = true;
    } else if (argc==7 && std::stod(std::string(argv[6]))==3){
        harpoon_join = true;
    } else if (argc==7 && std::stod(std::string(argv[6]))==4) {
        ten_split = true;
    } else if (argc==7 && std::stod(std::string(argv[6]))==5) {
        join_merge_join_merge = true;
    } else if (argc==7 && std::stod(std::string(argv[6]))==6) {
        three_four_split = true;
    }




    std::string platform_file_path = "/tmp/platform.xml";
    if(argc<3){
        generatePlatform(platform_file_path, disk_speed);
    } else {
        generatePlatform(platform_file_path, disk_speed, batch_bandwidth, batch_per_node_flops);
    }
    simulation->instantiatePlatform(platform_file_path);



    wrench::WorkflowFile *input_file1, *output_file1, *input_file2, *output_file2,
            *input_file3, *output_file3, *input_file4, *output_file4,
            *input_file5, *output_file5, *input_file6, *output_file6,
            *input_file7, *output_file7, *input_file8, *output_file8,
            *input_file9, *output_file9, *input_file10, *output_file10,
            *input_file11, *output_file11, *input_file12, *output_file12,
            *input_file13, *output_file13, *input_file14, *output_file14,
            *input_file15, *output_file15, *input_file16, *output_file16,
            *input_file17, *output_file17;
    wrench::WorkflowTask *task1, *task2, *task3, *task4, *task5, *task6, *task7,
            *task8, *task9, *task10, *task11, *task12, *task13, *task14, *task15,
            *task16, *task17;
    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service2 = nullptr;

    // Get a hostname
    std::string hostname = "DualCoreHost";

    wrench::Workflow *grid_workflow;
    std::unique_ptr<wrench::Workflow> workflow_unique_ptr1;
    workflow_unique_ptr1 = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());
    grid_workflow = workflow_unique_ptr1.get();

    input_file1 = grid_workflow->addFile("input_file1", 10000000000.0);
    output_file1 = grid_workflow->addFile("output_file1", 10000000000.0);


    //task1 = grid_workflow->addTask("grid_task1", 87867450000.0, 1, 1, 0);
    //task1 = grid_workflow->addTask("grid_task1", 3050000000.0, 1, 1, 0);

    task1 = grid_workflow->addTask("grid_task1", 500000000000.0, 1, 1, 0);
    task1->addInputFile(input_file1);
    task1->addOutputFile(output_file1);
    if(diamond_exec){
        input_file2 = grid_workflow->addFile("input_file2", 10000000000.0);
        output_file2 = grid_workflow->addFile("output_file2", 10000000000.0);
        input_file3 = grid_workflow->addFile("input_file3", 10000000000.0);
        output_file3 = grid_workflow->addFile("output_file3", 10000000000.0);
        input_file4 = grid_workflow->addFile("input_file4", 10000000000.0);
        output_file4 = grid_workflow->addFile("output_file4", 10000000000.0);

        task2 = grid_workflow->addTask("grid_task2", 500000000000.0, 1, 1, 0);
        task2->addInputFile(input_file2);
        task2->addOutputFile(output_file2);
        task3 = grid_workflow->addTask("grid_task3", 500000000000.0, 1, 1, 0);
        task3->addInputFile(input_file3);
        task3->addOutputFile(output_file3);
        task4 = grid_workflow->addTask("grid_task4", 500000000000.0, 1, 1, 0);
        task4->addInputFile(input_file4);
        task4->addOutputFile(output_file4);


        grid_workflow->addControlDependency(task1, task2);
        grid_workflow->addControlDependency(task1, task3);
        grid_workflow->addControlDependency(task3, task4);
        grid_workflow->addControlDependency(task2, task4);
    } else if (harpoon_join) {
        input_file2 = grid_workflow->addFile("input_file2", 10000000000.0);
        output_file2 = grid_workflow->addFile("output_file2", 10000000000.0);
        input_file3 = grid_workflow->addFile("input_file3", 10000000000.0);
        output_file3 = grid_workflow->addFile("output_file3", 10000000000.0);
        input_file4 = grid_workflow->addFile("input_file4", 10000000000.0);
        output_file4 = grid_workflow->addFile("output_file4", 10000000000.0);
        input_file5 = grid_workflow->addFile("input_file5", 10000000000.0);
        output_file5 = grid_workflow->addFile("output_file5", 10000000000.0);
        input_file6 = grid_workflow->addFile("input_file6", 10000000000.0);
        output_file6 = grid_workflow->addFile("output_file6", 10000000000.0);
        input_file7 = grid_workflow->addFile("input_file7", 10000000000.0);
        output_file7 = grid_workflow->addFile("output_file7", 10000000000.0);
        input_file8 = grid_workflow->addFile("input_file8", 10000000000.0);
        output_file8 = grid_workflow->addFile("output_file8", 10000000000.0);

        task2 = grid_workflow->addTask("grid_task2", 500000000000.0, 1, 1, 0);
        task2->addInputFile(input_file2);
        task2->addOutputFile(output_file2);
        task3 = grid_workflow->addTask("grid_task3", 500000000000.0, 1, 1, 0);
        task3->addInputFile(input_file3);
        task3->addOutputFile(output_file3);
        task4 = grid_workflow->addTask("grid_task4", 500000000000.0, 1, 1, 0);
        task4->addInputFile(input_file4);
        task4->addOutputFile(output_file4);
        task5 = grid_workflow->addTask("grid_task5", 500000000000.0, 1, 1, 0);
        task5->addInputFile(input_file5);
        task5->addOutputFile(output_file5);
        task6 = grid_workflow->addTask("grid_task6", 500000000000.0, 1, 1, 0);
        task6->addInputFile(input_file6);
        task6->addOutputFile(output_file6);
        task7 = grid_workflow->addTask("grid_task7", 500000000000.0, 1, 1, 0);
        task7->addInputFile(input_file7);
        task7->addOutputFile(output_file7);
        task8 = grid_workflow->addTask("grid_task8", 500000000000.0, 1, 1, 0);
        task8->addInputFile(input_file8);
        task8->addOutputFile(output_file8);

        grid_workflow->addControlDependency(task1, task2);
        grid_workflow->addControlDependency(task1, task3);
        grid_workflow->addControlDependency(task1, task4);

        grid_workflow->addControlDependency(task2, task5);
        grid_workflow->addControlDependency(task3, task6);
        grid_workflow->addControlDependency(task4, task7);

        grid_workflow->addControlDependency(task5, task8);
        grid_workflow->addControlDependency(task6, task8);
        grid_workflow->addControlDependency(task7, task8);

    } else if (ten_split) {
        input_file2 = grid_workflow->addFile("input_file2", 10000000000.0);
        output_file2 = grid_workflow->addFile("output_file2", 10000000000.0);
        input_file3 = grid_workflow->addFile("input_file3", 10000000000.0);
        output_file3 = grid_workflow->addFile("output_file3", 10000000000.0);
        input_file4 = grid_workflow->addFile("input_file4", 10000000000.0);
        output_file4 = grid_workflow->addFile("output_file4", 10000000000.0);
        input_file5 = grid_workflow->addFile("input_file5", 10000000000.0);
        output_file5 = grid_workflow->addFile("output_file5", 10000000000.0);
        input_file6 = grid_workflow->addFile("input_file6", 10000000000.0);
        output_file6 = grid_workflow->addFile("output_file6", 10000000000.0);
        input_file7 = grid_workflow->addFile("input_file7", 10000000000.0);
        output_file7 = grid_workflow->addFile("output_file7", 10000000000.0);
        input_file8 = grid_workflow->addFile("input_file8", 10000000000.0);
        output_file8 = grid_workflow->addFile("output_file8", 10000000000.0);
        input_file9 = grid_workflow->addFile("input_file9", 10000000000.0);
        output_file9 = grid_workflow->addFile("output_file9", 10000000000.0);
        input_file10 = grid_workflow->addFile("input_file10", 10000000000.0);
        output_file10 = grid_workflow->addFile("output_file10", 10000000000.0);
        input_file11 = grid_workflow->addFile("input_file11", 10000000000.0);
        output_file11 = grid_workflow->addFile("output_file11", 10000000000.0);
        input_file12 = grid_workflow->addFile("input_file12", 10000000000.0);
        output_file12 = grid_workflow->addFile("output_file12", 10000000000.0);


        task2 = grid_workflow->addTask("grid_task2", 500000000000.0, 1, 1, 0);
        task2->addInputFile(input_file2);
        task2->addOutputFile(output_file2);
        task3 = grid_workflow->addTask("grid_task3", 500000000000.0, 1, 1, 0);
        task3->addInputFile(input_file3);
        task3->addOutputFile(output_file3);
        task4 = grid_workflow->addTask("grid_task4", 500000000000.0, 1, 1, 0);
        task4->addInputFile(input_file4);
        task4->addOutputFile(output_file4);
        task5 = grid_workflow->addTask("grid_task5", 500000000000.0, 1, 1, 0);
        task5->addInputFile(input_file5);
        task5->addOutputFile(output_file5);
        task6 = grid_workflow->addTask("grid_task6", 500000000000.0, 1, 1, 0);
        task6->addInputFile(input_file6);
        task6->addOutputFile(output_file6);
        task7 = grid_workflow->addTask("grid_task7", 500000000000.0, 1, 1, 0);
        task7->addInputFile(input_file7);
        task7->addOutputFile(output_file7);
        task8 = grid_workflow->addTask("grid_task8", 500000000000.0, 1, 1, 0);
        task8->addInputFile(input_file8);
        task8->addOutputFile(output_file8);
        task9 = grid_workflow->addTask("grid_task9", 500000000000.0, 1, 1, 0);
        task9->addInputFile(input_file9);
        task9->addOutputFile(output_file9);
        task10 = grid_workflow->addTask("grid_task10", 500000000000.0, 1, 1, 0);
        task10->addInputFile(input_file10);
        task10->addOutputFile(output_file10);
        task11 = grid_workflow->addTask("grid_task11", 500000000000.0, 1, 1, 0);
        task11->addInputFile(input_file11);
        task11->addOutputFile(output_file11);
        task12 = grid_workflow->addTask("grid_task12", 500000000000.0, 1, 1, 0);
        task12->addInputFile(input_file12);
        task12->addOutputFile(output_file12);

        grid_workflow->addControlDependency(task1, task2);
        grid_workflow->addControlDependency(task1, task3);
        grid_workflow->addControlDependency(task1, task4);
        grid_workflow->addControlDependency(task1, task5);
        grid_workflow->addControlDependency(task1, task6);
        grid_workflow->addControlDependency(task1, task7);
        grid_workflow->addControlDependency(task1, task8);
        grid_workflow->addControlDependency(task1, task9);
        grid_workflow->addControlDependency(task1, task10);
        grid_workflow->addControlDependency(task1, task11);

        grid_workflow->addControlDependency(task2, task12);
        grid_workflow->addControlDependency(task3, task12);
        grid_workflow->addControlDependency(task4, task12);
        grid_workflow->addControlDependency(task5, task12);
        grid_workflow->addControlDependency(task6, task12);
        grid_workflow->addControlDependency(task7, task12);
        grid_workflow->addControlDependency(task8, task12);
        grid_workflow->addControlDependency(task9, task12);
        grid_workflow->addControlDependency(task10, task12);
        grid_workflow->addControlDependency(task11, task12);
    } else if (three_task){
        input_file2 = grid_workflow->addFile("input_file2", 10000000000.0);
        output_file2 = grid_workflow->addFile("output_file2", 10000000000.0);
        input_file3 = grid_workflow->addFile("input_file3", 10000000000.0);
        output_file3 = grid_workflow->addFile("output_file3", 10000000000.0);

        task2 = grid_workflow->addTask("grid_task2", 500000000000.0, 1, 1, 0);
        task2->addInputFile(input_file2);
        task2->addOutputFile(output_file2);
        task3 = grid_workflow->addTask("grid_task3", 500000000000.0, 1, 1, 0);
        task3->addInputFile(input_file3);
        task3->addOutputFile(output_file3);

        grid_workflow->addControlDependency(task1,task2);
        grid_workflow->addControlDependency(task2,task3);
    } else if (join_merge_join_merge) {
        input_file2 = grid_workflow->addFile("input_file2", 10000000000.0);
        output_file2 = grid_workflow->addFile("output_file2", 10000000000.0);
        input_file3 = grid_workflow->addFile("input_file3", 10000000000.0);
        output_file3 = grid_workflow->addFile("output_file3", 10000000000.0);
        input_file4 = grid_workflow->addFile("input_file4", 10000000000.0);
        output_file4 = grid_workflow->addFile("output_file4", 10000000000.0);
        input_file5 = grid_workflow->addFile("input_file5", 10000000000.0);
        output_file5 = grid_workflow->addFile("output_file5", 10000000000.0);
        input_file6 = grid_workflow->addFile("input_file6", 10000000000.0);
        output_file6 = grid_workflow->addFile("output_file6", 10000000000.0);
        input_file7 = grid_workflow->addFile("input_file7", 10000000000.0);
        output_file7 = grid_workflow->addFile("output_file7", 10000000000.0);
        input_file8 = grid_workflow->addFile("input_file8", 10000000000.0);
        output_file8 = grid_workflow->addFile("output_file8", 10000000000.0);
        input_file9 = grid_workflow->addFile("input_file9", 10000000000.0);
        output_file9 = grid_workflow->addFile("output_file9", 10000000000.0);
        input_file10 = grid_workflow->addFile("input_file10", 10000000000.0);
        output_file10 = grid_workflow->addFile("output_file10", 10000000000.0);
        input_file11 = grid_workflow->addFile("input_file11", 10000000000.0);
        output_file11 = grid_workflow->addFile("output_file11", 10000000000.0);
        input_file12 = grid_workflow->addFile("input_file12", 10000000000.0);
        output_file12 = grid_workflow->addFile("output_file12", 10000000000.0);
        input_file13 = grid_workflow->addFile("input_file13", 10000000000.0);
        output_file13 = grid_workflow->addFile("output_file13", 10000000000.0);


        task2 = grid_workflow->addTask("grid_task2", 500000000000.0, 1, 1, 0);
        task2->addInputFile(input_file2);
        task2->addOutputFile(output_file2);
        task3 = grid_workflow->addTask("grid_task3", 500000000000.0, 1, 1, 0);
        task3->addInputFile(input_file3);
        task3->addOutputFile(output_file3);
        task4 = grid_workflow->addTask("grid_task4", 500000000000.0, 1, 1, 0);
        task4->addInputFile(input_file4);
        task4->addOutputFile(output_file4);
        task5 = grid_workflow->addTask("grid_task5", 500000000000.0, 1, 1, 0);
        task5->addInputFile(input_file5);
        task5->addOutputFile(output_file5);
        task6 = grid_workflow->addTask("grid_task6", 500000000000.0, 1, 1, 0);
        task6->addInputFile(input_file6);
        task6->addOutputFile(output_file6);
        task7 = grid_workflow->addTask("grid_task7", 500000000000.0, 1, 1, 0);
        task7->addInputFile(input_file7);
        task7->addOutputFile(output_file7);
        task8 = grid_workflow->addTask("grid_task8", 500000000000.0, 1, 1, 0);
        task8->addInputFile(input_file8);
        task8->addOutputFile(output_file8);
        task9 = grid_workflow->addTask("grid_task9", 500000000000.0, 1, 1, 0);
        task9->addInputFile(input_file9);
        task9->addOutputFile(output_file9);
        task10 = grid_workflow->addTask("grid_task10", 500000000000.0, 1, 1, 0);
        task10->addInputFile(input_file10);
        task10->addOutputFile(output_file10);
        task11 = grid_workflow->addTask("grid_task11", 500000000000.0, 1, 1, 0);
        task11->addInputFile(input_file11);
        task11->addOutputFile(output_file11);
        task12 = grid_workflow->addTask("grid_task12", 500000000000.0, 1, 1, 0);
        task12->addInputFile(input_file12);
        task12->addOutputFile(output_file12);
        task13 = grid_workflow->addTask("grid_task13", 500000000000.0, 1, 1, 0);
        task13->addInputFile(input_file13);
        task13->addOutputFile(output_file13);

        grid_workflow->addControlDependency(task1, task2);
        grid_workflow->addControlDependency(task1, task3);
        grid_workflow->addControlDependency(task1, task4);
        grid_workflow->addControlDependency(task1, task5);
        grid_workflow->addControlDependency(task1, task6);

        grid_workflow->addControlDependency(task2, task7);
        grid_workflow->addControlDependency(task3, task7);
        grid_workflow->addControlDependency(task4, task7);
        grid_workflow->addControlDependency(task5, task7);
        grid_workflow->addControlDependency(task6, task7);

        grid_workflow->addControlDependency(task7, task8);
        grid_workflow->addControlDependency(task7, task9);
        grid_workflow->addControlDependency(task7, task10);
        grid_workflow->addControlDependency(task7, task11);
        grid_workflow->addControlDependency(task7, task12);

        grid_workflow->addControlDependency(task8, task13);
        grid_workflow->addControlDependency(task9, task13);
        grid_workflow->addControlDependency(task10, task13);
        grid_workflow->addControlDependency(task11, task13);
        grid_workflow->addControlDependency(task12, task13);
    } else if (three_four_split) {
        input_file2 = grid_workflow->addFile("input_file2", 10000000000.0);
        output_file2 = grid_workflow->addFile("output_file2", 10000000000.0);
        input_file3 = grid_workflow->addFile("input_file3", 10000000000.0);
        output_file3 = grid_workflow->addFile("output_file3", 10000000000.0);
        input_file4 = grid_workflow->addFile("input_file4", 10000000000.0);
        output_file4 = grid_workflow->addFile("output_file4", 10000000000.0);
        input_file5 = grid_workflow->addFile("input_file5", 10000000000.0);
        output_file5 = grid_workflow->addFile("output_file5", 10000000000.0);
        input_file6 = grid_workflow->addFile("input_file6", 10000000000.0);
        output_file6 = grid_workflow->addFile("output_file6", 10000000000.0);
        input_file7 = grid_workflow->addFile("input_file7", 10000000000.0);
        output_file7 = grid_workflow->addFile("output_file7", 10000000000.0);
        input_file8 = grid_workflow->addFile("input_file8", 10000000000.0);
        output_file8 = grid_workflow->addFile("output_file8", 10000000000.0);
        input_file9 = grid_workflow->addFile("input_file9", 10000000000.0);
        output_file9 = grid_workflow->addFile("output_file9", 10000000000.0);
        input_file10 = grid_workflow->addFile("input_file10", 10000000000.0);
        output_file10 = grid_workflow->addFile("output_file10", 10000000000.0);
        input_file11 = grid_workflow->addFile("input_file11", 10000000000.0);
        output_file11 = grid_workflow->addFile("output_file11", 10000000000.0);
        input_file12 = grid_workflow->addFile("input_file12", 10000000000.0);
        output_file12 = grid_workflow->addFile("output_file12", 10000000000.0);
        input_file13 = grid_workflow->addFile("input_file13", 10000000000.0);
        output_file13 = grid_workflow->addFile("output_file13", 10000000000.0);
        input_file14 = grid_workflow->addFile("input_file14", 10000000000.0);
        output_file14 = grid_workflow->addFile("output_file14", 10000000000.0);
        input_file15 = grid_workflow->addFile("input_file15", 10000000000.0);
        output_file15 = grid_workflow->addFile("output_file15", 10000000000.0);
        input_file16 = grid_workflow->addFile("input_file16", 10000000000.0);
        output_file16 = grid_workflow->addFile("output_file16", 10000000000.0);
        input_file17 = grid_workflow->addFile("input_file17", 10000000000.0);
        output_file17 = grid_workflow->addFile("output_file17", 10000000000.0);


        task2 = grid_workflow->addTask("grid_task2", 500000000000.0, 1, 1, 0);
        task2->addInputFile(input_file2);
        task2->addOutputFile(output_file2);
        task3 = grid_workflow->addTask("grid_task3", 500000000000.0, 1, 1, 0);
        task3->addInputFile(input_file3);
        task3->addOutputFile(output_file3);
        task4 = grid_workflow->addTask("grid_task4", 500000000000.0, 1, 1, 0);
        task4->addInputFile(input_file4);
        task4->addOutputFile(output_file4);
        task5 = grid_workflow->addTask("grid_task5", 500000000000.0, 1, 1, 0);
        task5->addInputFile(input_file5);
        task5->addOutputFile(output_file5);
        task6 = grid_workflow->addTask("grid_task6", 500000000000.0, 1, 1, 0);
        task6->addInputFile(input_file6);
        task6->addOutputFile(output_file6);
        task7 = grid_workflow->addTask("grid_task7", 500000000000.0, 1, 1, 0);
        task7->addInputFile(input_file7);
        task7->addOutputFile(output_file7);
        task8 = grid_workflow->addTask("grid_task8", 500000000000.0, 1, 1, 0);
        task8->addInputFile(input_file8);
        task8->addOutputFile(output_file8);
        task9 = grid_workflow->addTask("grid_task9", 500000000000.0, 1, 1, 0);
        task9->addInputFile(input_file9);
        task9->addOutputFile(output_file9);
        task10 = grid_workflow->addTask("grid_task10", 500000000000.0, 1, 1, 0);
        task10->addInputFile(input_file10);
        task10->addOutputFile(output_file10);
        task11 = grid_workflow->addTask("grid_task11", 500000000000.0, 1, 1, 0);
        task11->addInputFile(input_file11);
        task11->addOutputFile(output_file11);
        task12 = grid_workflow->addTask("grid_task12", 500000000000.0, 1, 1, 0);
        task12->addInputFile(input_file12);
        task12->addOutputFile(output_file12);
        task13 = grid_workflow->addTask("grid_task13", 500000000000.0, 1, 1, 0);
        task13->addInputFile(input_file13);
        task13->addOutputFile(output_file13);
        task14 = grid_workflow->addTask("grid_task14", 500000000000.0, 1, 1, 0);
        task14->addInputFile(input_file14);
        task14->addOutputFile(output_file14);
        task15 = grid_workflow->addTask("grid_task15", 500000000000.0, 1, 1, 0);
        task15->addInputFile(input_file15);
        task15->addOutputFile(output_file15);
        task16 = grid_workflow->addTask("grid_task16", 500000000000.0, 1, 1, 0);
        task16->addInputFile(input_file16);
        task16->addOutputFile(output_file16);
        task17 = grid_workflow->addTask("grid_task17", 500000000000.0, 1, 1, 0);
        task17->addInputFile(input_file17);
        task17->addOutputFile(output_file17);


        grid_workflow->addControlDependency(task1, task2);
        grid_workflow->addControlDependency(task1, task3);
        grid_workflow->addControlDependency(task1, task4);

        grid_workflow->addControlDependency(task2, task5);
        grid_workflow->addControlDependency(task2, task6);
        grid_workflow->addControlDependency(task2, task7);
        grid_workflow->addControlDependency(task2, task8);

        grid_workflow->addControlDependency(task3, task9);
        grid_workflow->addControlDependency(task3, task10);
        grid_workflow->addControlDependency(task3, task11);
        grid_workflow->addControlDependency(task3, task12);

        grid_workflow->addControlDependency(task4, task13);
        grid_workflow->addControlDependency(task4, task14);
        grid_workflow->addControlDependency(task4, task15);
        grid_workflow->addControlDependency(task4, task16);

        grid_workflow->addControlDependency(task5, task17);
        grid_workflow->addControlDependency(task6, task17);
        grid_workflow->addControlDependency(task7, task17);
        grid_workflow->addControlDependency(task8, task17);
        grid_workflow->addControlDependency(task9, task17);
        grid_workflow->addControlDependency(task10, task17);
        grid_workflow->addControlDependency(task11, task17);
        grid_workflow->addControlDependency(task12, task17);
        grid_workflow->addControlDependency(task13, task17);
        grid_workflow->addControlDependency(task14, task17);
        grid_workflow->addControlDependency(task15, task17);
        grid_workflow->addControlDependency(task16, task17);
    }





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

    if(argc>3){
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
    simulation->stageFile(input_file1, storage_service);

    if(diamond_exec){
        simulation->stageFile(input_file2, storage_service);
        simulation->stageFile(input_file3, storage_service);
        simulation->stageFile(input_file4, storage_service);
    } else if (harpoon_join){
        simulation->stageFile(input_file2, storage_service);
        simulation->stageFile(input_file3, storage_service);
        simulation->stageFile(input_file4, storage_service);
        simulation->stageFile(input_file5, storage_service);
        simulation->stageFile(input_file6, storage_service);
        simulation->stageFile(input_file7, storage_service);
        simulation->stageFile(input_file8, storage_service);
    } else if (ten_split){
        simulation->stageFile(input_file2, storage_service);
        simulation->stageFile(input_file3, storage_service);
        simulation->stageFile(input_file4, storage_service);
        simulation->stageFile(input_file5, storage_service);
        simulation->stageFile(input_file6, storage_service);
        simulation->stageFile(input_file7, storage_service);
        simulation->stageFile(input_file8, storage_service);
        simulation->stageFile(input_file9, storage_service);
        simulation->stageFile(input_file10, storage_service);
        simulation->stageFile(input_file11, storage_service);
        simulation->stageFile(input_file12, storage_service);
    } else if (three_task){
        simulation->stageFile(input_file2, storage_service);
        simulation->stageFile(input_file3, storage_service);
    } else if (join_merge_join_merge){
        simulation->stageFile(input_file2, storage_service);
        simulation->stageFile(input_file3, storage_service);
        simulation->stageFile(input_file4, storage_service);
        simulation->stageFile(input_file5, storage_service);
        simulation->stageFile(input_file6, storage_service);
        simulation->stageFile(input_file7, storage_service);
        simulation->stageFile(input_file8, storage_service);
        simulation->stageFile(input_file9, storage_service);
        simulation->stageFile(input_file10, storage_service);
        simulation->stageFile(input_file11, storage_service);
        simulation->stageFile(input_file12, storage_service);
        simulation->stageFile(input_file13, storage_service);
    } else if (three_four_split) {
        simulation->stageFile(input_file2, storage_service);
        simulation->stageFile(input_file3, storage_service);
        simulation->stageFile(input_file4, storage_service);
        simulation->stageFile(input_file5, storage_service);
        simulation->stageFile(input_file6, storage_service);
        simulation->stageFile(input_file7, storage_service);
        simulation->stageFile(input_file8, storage_service);
        simulation->stageFile(input_file9, storage_service);
        simulation->stageFile(input_file10, storage_service);
        simulation->stageFile(input_file11, storage_service);
        simulation->stageFile(input_file12, storage_service);
        simulation->stageFile(input_file13, storage_service);
        simulation->stageFile(input_file14, storage_service);
        simulation->stageFile(input_file15, storage_service);
        simulation->stageFile(input_file16, storage_service);
        simulation->stageFile(input_file17, storage_service);
    }


    // Running a "run a single task" simulation
    simulation->launch();

    simulation->getOutput().dumpUnifiedJSON(grid_workflow, "/tmp/workflow_data.json", false, true, false, false, false, true, false);
    auto start_timestamps = simulation->getOutput().getTrace<wrench::CondorGridStartTimestamp>();
    auto end_timestamp = simulation->getOutput().getTrace<wrench::CondorGridEndTimestamp>().back();

    for (const auto &start_timestamp : start_timestamps) {
        std::cout << "Started: " << start_timestamp->getContent()->getDate() << std::endl;
    }
    /**
    for (const auto &end_timestamp : end_timestamps) {

    }
     */
    std::cout << "Ended: " << end_timestamp->getContent()->getDate() << std::endl;

    return 0;
}
