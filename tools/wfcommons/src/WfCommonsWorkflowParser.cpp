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
#include <boost/json.hpp>

WRENCH_LOG_CATEGORY(wfcommons_workflow_parser, "Log category for WfCommonsWorkflowParser");


namespace wrench {

    boost::json::object readJSONFromFile(const std::string& filepath) {
        FILE *file = fopen(filepath.c_str(), "r");
        if (not file) {
            throw std::invalid_argument("Cannot read JSON file " + filepath);
        }

        boost::json::stream_parser p;
        boost::json::error_code ec;
        p.reset();
        while (true) {
            try {
                char buf[1024];
                auto nread = fread(buf, sizeof(char), 1024, file);
                if (nread == 0) {
                    break;
                }
                p.write(buf, nread, ec);
            } catch (std::exception &e) {
                throw std::invalid_argument("Error while reading JSON file " + filepath + ": " + std::string(e.what()));
            }
        }

        p.finish(ec);
        if (ec) {
            throw std::invalid_argument("Error while reading JSON file " + filepath + ": " + ec.message());
        }
        return p.release().as_object();
    }


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
        boost::json::object j;
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

        try {
            j = readJSONFromFile(filename);
        } catch (std::invalid_argument &e) {
            throw std::invalid_argument("WfCommonsWorkflowParser::createWorkflowFromJson(): " + std::string(e.what()));
        }

        boost::json::object workflow_spec;
        try {
            workflow_spec = j["workflow"].as_object();
        } catch (std::out_of_range &e) {
            throw std::invalid_argument("WfCommonsWorkflowParser::createWorkflowFromJson(): Could not find a 'workflow' key");
        }

        std::shared_ptr<wrench::WorkflowTask> task;

        // Gather machine information if any
        std::map<std::string, std::pair<unsigned long, double>> machines;
        for (auto & it : workflow_spec) {
            if (it.key() == "machines") {
                auto machine_specs = it.value().as_array();
                for (auto &m: machine_specs) {
                    std::string name = std::string(m.as_object().at("nodeName").as_string().c_str());
                    auto core_spec = m.at("cpu").as_object();
                    unsigned long num_cores;
                    if (core_spec.find("count") == core_spec.end()) {
                        num_cores = 1;
                    } else {
                        num_cores = core_spec.at("count").to_number<unsigned long>();
                    }
                    double mhz;
                    if (core_spec.find("speed") == core_spec.end()) {
                        mhz = -1.0;
                    } else {
                        mhz = core_spec.at("speed").to_number<double>();
                    }
                    machines[name] = std::make_pair(num_cores, mhz);
                }
            }
        }

        // Process the tasks
        for (auto &it : workflow_spec) {
            if (it.key() == "tasks") {
                auto task_specs = it.value().as_array();

                for (auto &task_spec : task_specs) {
                    auto task_spec_object = task_spec.as_object();
                    std::string name = task_spec_object.at("name").as_string().c_str();
                    double runtime = task_spec_object.at("runtime").to_number<double>();
                    unsigned long min_num_cores, max_num_cores;
                    // Set the default values
                    min_num_cores = min_cores_per_task;
                    max_num_cores = max_cores_per_task;
                    // Overwrite the default is we don't enforce the default values AND the JSON specifies core numbers
                    if ((not enforce_num_cores) and task_spec_object.find("cores") != task_spec_object.end()) {
                        min_num_cores = task_spec_object.at("cores").to_number<unsigned long>();
                        max_num_cores = task_spec_object.at("cores").to_number<unsigned long>();
                    }
                    std::string type = task_spec_object.at("type").as_string().c_str();

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
                    if (task_spec_object.find("machine") != task_spec_object.end()) {
                      execution_machine = std::string(task_spec_object.at("machine").as_string().c_str());
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

                    task = workflow->addTask(name, flop_amount, min_num_cores, max_num_cores, 0.0);

                    // task priority
                    if (task_spec_object.find("priority") != task_spec_object.end()) {
                        task->setPriority(task_spec_object.at("priority").to_number<long>());
                    }

                    // task average CPU
                    if (task_spec_object.find("avgCPU") != task_spec_object.end()) {
                        task->setAverageCPU(task_spec_object.at("avgCPU").to_number<double>());
                    }

                    // task bytes read
                    if (task_spec_object.find("bytesRead") != task_spec_object.end()) {
                        task->setBytesRead(task_spec_object.at("bytesRead").to_number<unsigned long>() * 1000);
                    }

                    // task bytes written
                    if (task_spec_object.find("bytesWritten") != task_spec_object.end()) {
                        task->setBytesWritten(task_spec_object.at("bytesWritten").to_number<unsigned long>()) * 1000;
                    }

                    // task files
                    auto files = task_spec_object.at("files").as_array();

                    for (auto &f: files) {
                        auto f_spec = f.as_object();
                        double size = f_spec.at("size").to_number<double>() * 1000;
                        std::string link = std::string(f_spec.at("link").as_string().c_str());
                        std::string id = std::string(f_spec.at("name").as_string().c_str());
                        std::shared_ptr<wrench::DataFile> workflow_file = nullptr;
                        // Check whether the file already exists
                        try {
                            workflow_file = workflow->getFileByID(id);
                        } catch (const std::invalid_argument &ia) {
                            // making a new file
                            workflow_file = workflow->addFile(id, size_in_bytes);
                        }
                        if (link == "input") {
                            task->addInputFile(workflow_file);
                        } else if (link == "output") {
                            task->addOutputFile(workflow_file);
                        }
                    }
                }

                // since tasks may not be ordered in the JSON file, we need to iterate over all tasks again
                for (auto &task_spec : task_specs) {
                    try {
                        task = workflow->getTaskByID(std::string(task_spec.as_object().at("name").as_string().c_str()));
                    } catch (std::invalid_argument &e) {
                        // Ignored task
                        continue;
                    }
                    auto parents = task_spec.as_object().at("parents").as_array();
                    // task dependencies
                    for (auto &parent: parents) {
                        std::string parent_id = std::string(parent.as_string().c_str());
                        // Ignore transfer jobs declared as parents
                        if (ignored_transfer_jobs.find(parent_id) != ignored_transfer_jobs.end()) {
                            continue;
                        }
                        // Ignore auxiliary jobs declared as parents
                        if (ignored_auxiliary_jobs.find(parent_id) != ignored_auxiliary_jobs.end()) {
                            continue;
                        }
                        try {
                            auto parent_task = workflow->getTaskByID(parent_id);
                            workflow->addControlDependency(parent_task, task, redundant_dependencies);
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
