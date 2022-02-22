/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/tools/wfcommons/WfCommonsWorkflowParser.h"
#include <wrench-dev.h>
#include <wrench/util/UnitParser.h>

#include <iostream>
#include <vector>
#include <fstream>
#include <pugixml.hpp>
#include <nlohmann/json.hpp>

WRENCH_LOG_CATEGORY(wfcommons_workflow_parser, "Log category for WfCommonsWorkflowParser");


namespace wrench {

    /**
     * Documentation in .h file
     */
    Workflow *WfCommonsWorkflowParser::createWorkflowFromJSON(const std::string &filename,
                                                            const std::string &reference_flop_rate,
                                                            bool redundant_dependencies,
                                                            unsigned long min_cores_per_task,
                                                            unsigned long max_cores_per_task,
                                                            bool enforce_num_cores) {

        std::ifstream file;
        nlohmann::json j;
        std::set<std::string> ignored_auxiliary_jobs;
        std::set<std::string> ignored_transfer_jobs;

        auto workflow = new Workflow();

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
            throw std::invalid_argument("WfCommonsWorkflowParser::createWorkflowFromJson(): Invalid Json file");
        }

        nlohmann::json workflowJobs;
        try {
            workflowJobs = j.at("workflow");
        } catch (std::out_of_range &e) {
            throw std::invalid_argument("WfCommonsWorkflowParser::createWorkflowFromJson(): Could not find a workflow exit");
        }

        wrench::WorkflowTask *task;

        for (nlohmann::json::iterator it = workflowJobs.begin(); it != workflowJobs.end(); ++it) {
            if (it.key() == "tasks") {
                std::vector<nlohmann::json> jobs = it.value();

                for (auto &job : jobs) {
                    std::string name = job.at("name");
                    double runtime = job.at("runtime");
                    unsigned long min_num_cores, max_num_cores;
                    // Set the default values
                    min_num_cores = min_cores_per_task;
                    max_num_cores = max_cores_per_task;
                    // Overwrite the default is we don't enforce the default values AND the JSON specifies core numbers
                    if ((not enforce_num_cores) and job.find("cores") != job.end()) {
                        min_num_cores = job.at("cores");
                        max_num_cores = job.at("cores");
                    }
                    std::string type = job.at("type");

                    if (type == "transfer") {
                        // Ignore,  since this is an abstract workflow
                        ignored_transfer_jobs.insert(name);
                        continue;
                    }
                    if (type == "auxiliary") {
                        // Ignore,  since this is an abstract workflow
                        ignored_auxiliary_jobs.insert(name);
                        continue;
                    }

                    if (type != "compute") {
                        throw std::invalid_argument("WfCommonsWorkflowParser::createWorkflowFromJson(): Job " + name + " has unknown type " + type);
                    }

                    task = workflow->addTask(name, runtime * flop_rate, min_num_cores, max_num_cores, 0.0);

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

                    // task files
                    std::vector<nlohmann::json> files = job.at("files");

                    for (auto &f : files) {
                        double size = f.at("size");
                        std::string link = f.at("link");
                        std::string id = f.at("name");
                        WorkflowFile *workflow_file = nullptr;
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

                    }
                }

                // since tasks may not be ordered in the JSON file, we need to iterate over all tasks again
                for (auto &job : jobs) {
                    try {
                        task = workflow->getTaskByID(job.at("name"));
                    } catch (std::invalid_argument &e) {
                        // Ignored task
                        continue;
                    }
                    std::vector<nlohmann::json> parents = job.at("parents");
                    // task dependencies
                    for (auto &parent : parents) {
                        // Ignore transfer jobs declared as parents
                        if (ignored_transfer_jobs.find(parent) != ignored_transfer_jobs.end()) {
                            continue;
                        }
                        // Ignore auxiliary jobs declared as parents
                        if (ignored_auxiliary_jobs.find(parent) != ignored_auxiliary_jobs.end()) {
                            continue;
                        }
                        try {
                            auto parent_task = workflow->getTaskByID(parent);
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
     * Documentation in .h file
     */
    Workflow *WfCommonsWorkflowParser::createExecutableWorkflowFromJSON(const std::string &filename, const std::string &reference_flop_rate,
                                                                      bool redundant_dependencies,
                                                                      unsigned long min_cores_per_task,
                                                                      unsigned long max_cores_per_task,
                                                                      bool enforce_num_cores) {
        throw std::runtime_error("WfCommonsWorkflowParser::createExecutableWorkflowFromJSON(): not implemented yet");
    }

};
