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
#include <nlohmann/json.hpp>

WRENCH_LOG_CATEGORY(wfcommons_workflow_parser, "Log category for WfCommonsWorkflowParser");


namespace wrench {

    /**
     * Documentation in .h file
     */
    std::shared_ptr<Workflow> WfCommonsWorkflowParser::createWorkflowFromJSON(const std::string &filename,
                                                                              const std::string &reference_flop_rate,
                                                                              bool redundant_dependencies,
                                                                              unsigned long min_cores_per_task,
                                                                              unsigned long max_cores_per_task,
                                                                              bool enforce_num_cores) {

        std::ifstream file;
        nlohmann::json j;
        std::set<std::string> ignored_auxiliary_jobs;
        std::set<std::string> ignored_transfer_jobs;

        auto workflow = Workflow::createWorkflow();

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

        nlohmann::json workflow_spec;
        try {
            workflow_spec = j.at("workflow");
        } catch (std::out_of_range &e) {
            throw std::invalid_argument("WfCommonsWorkflowParser::createWorkflowFromJson(): Could not find a 'workflow' key");
        }

        std::shared_ptr<wrench::WorkflowTask> task;

        // Gather machine information if any
        std::map<std::string, std::pair<unsigned long, double>> machines;
        for (nlohmann::json::iterator it = workflow_spec.begin(); it != workflow_spec.end(); ++it) {
            if (it.key() == "machines") {
                std::vector<nlohmann::json> machine_specs = it.value();
                for (auto &m: machine_specs) {
                    std::string name = m.at("nodeName");
                    nlohmann::json core_spec = m.at("cpu");
                    unsigned long num_cores = core_spec.at("count");
                    double ghz = core_spec.at("speed");
                    machines[name] = std::make_pair(num_cores, ghz);
                }
            }
        }

        // Process the tasks
        for (nlohmann::json::iterator it = workflow_spec.begin(); it != workflow_spec.end(); ++it) {
            if (it.key() == "tasks") {
                std::vector<nlohmann::json> jobs = it.value();

                for (auto &job: jobs) {
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
                        throw std::invalid_argument("Workflow::createWorkflowFromJson(): Job " + name + " has unknown type " + type);
                    }

                    double flop_amount;
                    std::string execution_machine;
                    if (job.find("machine") != job.end()) {
                        execution_machine = job.at("machine");
                    }
                    if (execution_machine.empty()) {
                        flop_amount = runtime * flop_rate;
                    } else {
                        if (machines.find(execution_machine) == machines.end()) {
                            throw std::invalid_argument("WfCommonsWorkflowParser::createWorkflowFromJSON(): Task " + name +
                                                        " is said to have been executed on machine " + execution_machine +
                                                        "  but no description for that machine is found on the JSON file");
                        }
                        double core_ghz = (machines[execution_machine].second);
                        double total_compute_power_used = core_ghz * (double) min_num_cores;
                        double actual_flop_rate = total_compute_power_used * 1000.0 * 1000.0 * 1000.0;
                        flop_amount = runtime * actual_flop_rate;
                    }

                    task = workflow->addTask(name, flop_amount, min_num_cores, max_num_cores, 0.0);

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

                    for (auto &f: files) {
                        double size = f.at("size");
                        std::string link = f.at("link");
                        std::string id = f.at("name");
                        std::shared_ptr<wrench::DataFile> workflow_file = nullptr;
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
                for (auto &job: jobs) {
                    try {
                        task = workflow->getTaskByID(job.at("name"));
                    } catch (std::invalid_argument &e) {
                        // Ignored task
                        continue;
                    }
                    std::vector<nlohmann::json> parents = job.at("parents");
                    // task dependencies
                    for (auto &parent: parents) {
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
    std::shared_ptr<Workflow> WfCommonsWorkflowParser::createExecutableWorkflowFromJSON(const std::string &filename, const std::string &reference_flop_rate,
                                                                                        bool redundant_dependencies,
                                                                                        unsigned long min_cores_per_task,
                                                                                        unsigned long max_cores_per_task,
                                                                                        bool enforce_num_cores) {
        throw std::runtime_error("WfCommonsWorkflowParser::createExecutableWorkflowFromJSON(): not implemented yet");
    }

};// namespace wrench
