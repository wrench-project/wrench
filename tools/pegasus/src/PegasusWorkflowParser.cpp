/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "../include/PegasusWorkflowParser.h"
#include <wrench-dev.h>
#include <wrench/util/UnitParser.h>

#include <iostream>
#include <vector>
#include <fstream>
#include <pugixml.hpp>
#include <nlohmann/json.hpp>

WRENCH_LOG_NEW_DEFAULT_CATEGORY(pegasus_workflow_parser, "Log category for PegasusWorkflowParser");


namespace wrench {

   /**
    * @brief Create a workflow based on a DAX or a JSON file
    *
    * @param filename: the path to the DAX (with .dax extension) or JSON (with .json extension) file
    * @param reference_flop_rate: a reference compute speed (in flops/sec), assuming a task's computation is purely flops.
    *                             This is needed because JSON files specify task execution times in seconds,
    *                             but the WRENCH simulation needs some notion of "amount of computation" to
    *                             apply reasonable scaling. (Because the XML platform description specifies host
    *                             compute speeds in flops/sec). The times in the JSON file are thus assumed to be
     *                            obtained on an machine with flop rate reference_flop_rate.
    * @param redundant_dependencies: whether DAG redundant dependencies should be kept in the graph
    *
    * @throw std::invalid_argument
    */
    Workflow *PegasusWorkflowParser::createWorkflowFromDAXorJSON(const std::string &filename, const std::string &reference_flop_rate,
                                     bool redundant_dependencies) {
        std::istringstream ss(filename);
        std::string token;
        std::vector<std::string> tokens;

        while (std::getline(ss, token, '.')) {
            tokens.push_back(token);
        }

        if (tokens.size() < 2) {
            throw std::invalid_argument(
                    "PegasusWorkflowParser::createWorkflowFromDAXorJSON(): workflow file name must end with '.dax' or '.json'");
        }
        std::string extension = tokens[tokens.size() - 1];

        if (extension == "dax") {
            return createWorkflowFromDAX(filename, reference_flop_rate, redundant_dependencies);
        } else if (extension == "json") {
            return createWorkflowFromJSON(filename, reference_flop_rate, redundant_dependencies);
        } else {
            throw std::invalid_argument(
                    "Workflow::createWorkflowFromDAXorJSON(): workflow file name must end with '.dax' or '.json'");
        }
    }


    /**
     * @brief Create a workflow based on a JSON file
     *
     * @param filename: the path to the JSON file
     * @param reference_flop_rate: a reference compute speed (in flops/sec), assuming a task's computation is purely flops.
     *                             This is needed because JSON files specify task execution times in seconds,
     *                             but the WRENCH simulation needs some notion of "amount of computation" to
     *                             apply reasonable scaling. (Because the XML platform description specifies host
     *                             compute speeds in flops/sec). The times in the JSON file are thus assumed to be
     *                             obtained on an machine with flop rate reference_flop_rate.
     * @param redundant_dependencies: whether DAG redundant dependencies should be kept in the graph
     *
     * @throw std::invalid_argument
     */
    Workflow *PegasusWorkflowParser::createWorkflowFromJSON(const std::string &filename, const std::string &reference_flop_rate,
                                bool redundant_dependencies) {

        std::ifstream file;
        nlohmann::json j;

        auto *workflow = new Workflow();

        double flop_rate;

        try {
            flop_rate = UnitParser::parse_compute_speed(reference_flop_rate);
        } catch (std::invalid_argument &e) {
            throw;
        }

        //handle the exceptions of opening the json file
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try {
            file.open(filename);
            file >> j;
        } catch (const std::ifstream::failure &e) {
            throw std::invalid_argument("Workflow::createWorkflowFromJson(): Invalid Json file");
        }

        nlohmann::json workflowJobs;
        try {
            workflowJobs = j.at("workflow");
        } catch (std::out_of_range &e) {
            throw std::invalid_argument("Workflow::createWorkflowFromJson(): Could not find a workflow exit");
        }

        wrench::WorkflowTask *task;

        for (nlohmann::json::iterator it = workflowJobs.begin(); it != workflowJobs.end(); ++it) {
            if (it.key() == "jobs") {
                std::vector<nlohmann::json> jobs = it.value();

                for (auto &job : jobs) {
                    std::string name = job.at("name");
                    double runtime = job.at("runtime");
                    unsigned long num_procs = 1;
                    task = workflow->addTask(name, runtime * flop_rate, num_procs, num_procs, 1.0, 0.0);

                    // task priority
                    try {
                        task->setPriority(job.at("priority"));
                    } catch (nlohmann::json::out_of_range &e) {
                        // do nothing
                    }

                    // task average CPU
                    try {
                        task->setAverageCPU(job.at("avgCPU"));
                    } catch (nlohmann::json::out_of_range &e) {
                        // do nothing
                    }

                    // task bytes read
                    try {
                        task->setBytesRead(job.at("bytesRead"));
                    } catch (nlohmann::json::out_of_range &e) {
                        // do nothing
                    }

                    // task bytes written
                    try {
                        task->setBytesWritten(job.at("bytesWritten"));
                    } catch (nlohmann::json::out_of_range &e) {
                        // do nothing
                    }

                    // task type
                    std::string type = job.at("type");
                    if (type == "transfer") {
                        // TODO: DO SOMETHING APPROPRIATE HERE
                    } else if (type == "auxiliary") {
                        // TODO: DO SOMETHING APPROPRIATE HERE
                    } else {
                        throw std::invalid_argument("Workflow::createWorkflowFromJson(): Job " + name + " has uknown type " + type);
                    }

                    // task files
                    std::vector<nlohmann::json> files = job.at("files");

                    for (auto &f : files) {
                        double size = f.at("size");
                        std::string link = f.at("link");
                        std::string id = f.at("name");
                        wrench::WorkflowFile *workflow_file = nullptr;
                        // Check whether the file already exists
                        try {
                            workflow_file = workflow->getFileByID(id);
                        } catch (const std::invalid_argument &ia) {
                            // making a new file
                            workflow_file = workflow->addFile(id, size);
                        }
                        if (link == "input") {
                            task->addInputFile(workflow_file);
                        } else if (link == "output") {
                            task->addOutputFile(workflow_file);
                        }
                        if (type == "transfer") {
                            task->addSrcDest(workflow_file, f.at("src"), f.at("dest"));
                        }
                    }
                }

                // since tasks may not be ordered in the JSON file, we need to iterate over all tasks again
                for (auto &job : jobs) {
                    task = workflow->getTaskByID(job.at("name"));
                    std::vector<nlohmann::json> parents = job.at("parents");
                    // task dependencies
                    for (auto &parent : parents) {
                        try {
                            WorkflowTask *parent_task = workflow->getTaskByID(parent);
                            workflow->addControlDependency(parent_task, task, redundant_dependencies);
                        } catch (std::invalid_argument &e) {
                            // do nothing
                        }
                    }
                }
            }
        }
        file.close();

        return workflow;
    }


    /**
     * @brief Create a workflow based on a DAX file
     *
     * @param filename: the path to the DAX file
     * @param reference_flop_rate: a reference compute speed (in flops/sec), assuming a task's computation is purely flops.
     *                             This is needed because DAX files specify task execution times in seconds,
     *                             but the WRENCH simulation needs some notion of "amount of computation" to
     *                             apply reasonable scaling. (Because the XML platform description specifies host
     *                             compute speeds in flops/sec). The times in the DAX file are thus assumed to be
     *                             obtained on an machine with flop rate reference_flop_rate.
     * @param redundant_dependencies: whether DAG redundant dependencies should be kept in the graph
     *
     * @throw std::invalid_argument
     */
    Workflow *PegasusWorkflowParser::createWorkflowFromDAX(const std::string &filename, const std::string &reference_flop_rate,
                               bool redundant_dependencies) {

        WRENCH_INFO("HERE");
        pugi::xml_document dax_tree;

        auto *workflow = new Workflow();

        double flop_rate;

        try {
            flop_rate = UnitParser::parse_compute_speed(reference_flop_rate);
        } catch (std::invalid_argument &e) {
            throw;
        }

        if (not dax_tree.load_file(filename.c_str())) {
            throw std::invalid_argument("Workflow::createWorkflowFromDAX(): Invalid DAX file");
        }

        // Get the root node
        pugi::xml_node dag = dax_tree.child("adag");

        // Iterate through the "job" nodes
        for (pugi::xml_node job = dag.child("job"); job; job = job.next_sibling("job")) {
            WorkflowTask *task;
            // Get the job attributes
            std::string id = job.attribute("id").value();
            std::string name = job.attribute("name").value();
            double runtime = std::strtod(job.attribute("runtime").value(), nullptr);
            int num_procs = 1;
            bool found_one = false;
            for (std::string tag : {"numprocs", "num_procs", "numcores", "num_cores"}) {
                if (job.attribute(tag.c_str())) {
                    if (found_one) {
                        throw std::invalid_argument(
                                "Workflow::createWorkflowFromDAX(): multiple \"number of cores/procs\" specification for task " +
                                id);
                    } else {
                        found_one = true;
                        num_procs = std::stoi(job.attribute(tag.c_str()).value());
                    }
                }
            }


            // Create the task
            // If the DAX says num_procs = x, then we set min_cores=1, max_cores=x, efficiency=1.0
            task = workflow->addTask(id, runtime * flop_rate, 1, num_procs, 1.0, 0.0);

            // Go through the children "uses" nodes
            for (pugi::xml_node uses = job.child("uses"); uses; uses = uses.next_sibling("uses")) {
                // getMessage the "uses" attributes
                // TODO: There are several attributes that we're ignoring for now...
                std::string id = uses.attribute("file").value();

                double size = std::strtod(uses.attribute("size").value(), nullptr);
                std::string link = uses.attribute("link").value();
                // Check whether the file already exists
                WorkflowFile *file = nullptr;
                try {
                    file = workflow->getFileByID(id);
                } catch (std::invalid_argument &e) {
                    file = workflow->addFile(id, size);
                }
                if (link == "input") {
                    task->addInputFile(file);
                }
                if (link == "output") {
                    task->addOutputFile(file);
                }
                // TODO: Are there other types of "link" values?
            }
        }

        // Iterate through the "child" nodes to handle control dependencies
        for (pugi::xml_node child = dag.child("child"); child; child = child.next_sibling("child")) {

            WorkflowTask *child_task = workflow->getTaskByID(child.attribute("ref").value());

            // Go through the children "parent" nodes
            for (pugi::xml_node parent = child.child("parent"); parent; parent = parent.next_sibling("parent")) {
                std::string parent_id = parent.attribute("ref").value();

                WorkflowTask *parent_task = workflow->getTaskByID(parent_id);
                workflow->addControlDependency(parent_task, child_task, redundant_dependencies);
            }
        }

        return workflow;
    }

};