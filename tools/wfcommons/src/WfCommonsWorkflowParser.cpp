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
                                                                              bool ignore_cycle_creating_dependencies,
                                                                              unsigned long min_cores_per_task,
                                                                              unsigned long max_cores_per_task,
                                                                              bool enforce_num_cores) {
        std::ifstream file;
        nlohmann::json j;
        std::set<std::string> ignored_auxiliary_jobs;
        std::set<std::string> ignored_transfer_jobs;

        auto workflow = Workflow::createWorkflow();
        workflow->enableTopBottomLevelDynamicUpdates(false);

        double flop_rate;

        try {
            flop_rate = UnitParser::parse_compute_speed(reference_flop_rate);
        } catch (std::invalid_argument &e) {
            throw;
        }

        // handle exceptions when opening the json file
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try {
            file.open(filename);
            file >> j;
        } catch (const std::ifstream::failure &e) {
            throw std::invalid_argument("WfCommonsWorkflowParser::createWorkflowFromJson(): Invalid Json file");
        }

        // Check schema version
        nlohmann::json schema_version;
        try {
            schema_version = j.at("schemaVersion");
        } catch (std::out_of_range &e) {
            throw std::invalid_argument("WfCommonsWorkflowParser::createWorkflowFromJson(): Could not find a 'schema_version' key");
        }
        if (schema_version != "1.4") {
            throw std::invalid_argument("WfCommonsWorkflowParser::createWorkflowFromJson(): Only handles WfFormat schema version 1.4");
        }

        nlohmann::json workflow_spec;
        try {
            workflow_spec = j.at("workflow");
        } catch (std::out_of_range &e) {
            throw std::invalid_argument("WfCommonsWorkflowParser::createWorkflowFromJson(): Could not find a 'workflow' key");
        }


        // Gather machine information if any
        std::map<std::string, std::pair<unsigned long, double>> machines;
        for (nlohmann::json::iterator it = workflow_spec.begin(); it != workflow_spec.end(); ++it) {
            if (it.key() == "machines") {
                std::vector<nlohmann::json> machine_specs = it.value();
                for (auto &m: machine_specs) {
                    std::string name = m.at("nodeName");
                    nlohmann::json core_spec = m.at("cpu");
                    unsigned long num_cores;
                    try {
                        num_cores = core_spec.at("count");
                    } catch (nlohmann::detail::out_of_range &e) {
                        num_cores = 1;
                    }
                    double mhz;
                    try {
                        mhz = core_spec.at("speed");
                    } catch (nlohmann::detail::out_of_range &e) {
                        mhz = -1.0;// unknown
                    }
                    machines[name] = std::make_pair(num_cores, mhz);
                }
            }
        }

        std::shared_ptr<wrench::WorkflowTask> workflow_task;


        // Process the tasks
        for (nlohmann::json::iterator it = workflow_spec.begin(); it != workflow_spec.end(); ++it) {
            if (it.key() == "tasks") {
                std::vector<nlohmann::json> tasks = it.value();

                for (auto &task: tasks) {
                    std::string name = task.at("name");
                    double runtime = task.at("runtimeInSeconds");
                    unsigned long min_num_cores, max_num_cores;
                    // Set the default values
                    min_num_cores = min_cores_per_task;
                    max_num_cores = max_cores_per_task;
                    // Overwrite the default is we don't enforce the default values AND the JSON specifies core numbers
                    if ((not enforce_num_cores) and task.find("cores") != task.end()) {
                        min_num_cores = task.at("cores");
                        max_num_cores = task.at("cores");
                    }
                    std::string type = task.at("type");

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
                    if (task.find("machine") != task.end()) {
                        execution_machine = task.at("machine");
                    }
                    if (execution_machine.empty()) {
                        flop_amount = runtime * flop_rate;
                    } else {
                        if (machines.find(execution_machine) == machines.end()) {
                            throw std::invalid_argument("WfCommonsWorkflowParser::createWorkflowFromJSON(): Task " + name +
                                                        " is said to have been executed on machine " + execution_machine +
                                                        "  but no description for that machine is found on the JSON file");
                        }
                        if (machines[execution_machine].second >= 0) {
                            double core_ghz = (machines[execution_machine].second) / 1000.0;
                            double total_compute_power_used = core_ghz * (double) min_num_cores;
                            double actual_flop_rate = total_compute_power_used * 1000.0 * 1000.0 * 1000.0;
                            flop_amount = runtime * actual_flop_rate;
                        } else {
                            flop_amount = (double) min_num_cores * runtime * flop_rate;// Assume a min-core execution
                        }
                    }

                    workflow_task = workflow->addTask(name, flop_amount, min_num_cores, max_num_cores, 0.0);

                    // task priority
                    try {
                        workflow_task->setPriority(task.at("priority"));
                    } catch (nlohmann::json::out_of_range &e) {
                        // do nothing
                    }

                    // task average CPU
                    try {
                        workflow_task->setAverageCPU(task.at("avgCPU"));
                    } catch (nlohmann::json::out_of_range &e) {
                        // do nothing
                    }

                    // task bytes read
                    try {
                        workflow_task->setBytesRead(task.at("readBytes"));
                    } catch (nlohmann::json::out_of_range &e) {
                        // do nothing
                    }

                    // task bytes written
                    try {
                        workflow_task->setBytesWritten(task.at("writtenBytes"));
                    } catch (nlohmann::json::out_of_range &e) {
                        // do nothing
                    }

                    // task files
                    std::vector<nlohmann::json> files = task.at("files");

                    for (auto &f: files) {
                        double size_in_bytes = f.at("sizeInBytes");
                        std::string link = f.at("link");
                        std::string id = f.at("name");
                        std::shared_ptr<wrench::DataFile> workflow_file = nullptr;
                        // Check whether the file already exists
                        try {
                            workflow_file = workflow->getFileByID(id);
                        } catch (const std::invalid_argument &ia) {
                            // making a new file
                            workflow_file = workflow->addFile(id, size_in_bytes);
                        }
                        if (link == "input") {
                            workflow_task->addInputFile(workflow_file);
                        } else if (link == "output") {
                            workflow_task->addOutputFile(workflow_file);
                        }
                    }
                }

                // since tasks may not be ordered in the JSON file, we need to iterate over all tasks again
                for (auto &task: tasks) {
                    try {
                        workflow_task = workflow->getTaskByID(task.at("name"));
                    } catch (std::invalid_argument &e) {
                        // Ignored task
                        continue;
                    }
                    std::vector<nlohmann::json> parents = task.at("parents");
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
                            workflow->addControlDependency(parent_task, workflow_task, redundant_dependencies);
                        } catch (std::invalid_argument &e) {
                            // do nothing
                        } catch (std::runtime_error &e) {
                            if (ignore_cycle_creating_dependencies) {
                                // nothing
                            } else {
                                throw;
                            }
                        }
                    }
                }
            }
        }
        file.close();
        workflow->enableTopBottomLevelDynamicUpdates(true);
        workflow->updateAllTopBottomLevels();

        return workflow;
    }


    /**
     * Documentation in .h file
     */
    std::shared_ptr<Workflow> WfCommonsWorkflowParser::createExecutableWorkflowFromJSON(const std::string &filename, const std::string &reference_flop_rate,
                                                                                        bool redundant_dependencies,
                                                                                        bool ignore_cycle_creating_dependencies,
                                                                                        unsigned long min_cores_per_task,
                                                                                        unsigned long max_cores_per_task,
                                                                                        bool enforce_num_cores) {
        throw std::runtime_error("WfCommonsWorkflowParser::createExecutableWorkflowFromJSON(): not implemented yet");
    }

};// namespace wrench
