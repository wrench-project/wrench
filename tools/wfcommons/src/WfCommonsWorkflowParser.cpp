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
                                                                              bool ignore_machine_specs,
                                                                              bool redundant_dependencies,
                                                                              bool ignore_cycle_creating_dependencies,
                                                                              unsigned long min_cores_per_task,
                                                                              unsigned long max_cores_per_task,
                                                                              bool enforce_num_cores,
                                                                              bool ignore_avg_cpu,
                                                                              bool show_warnings) {
        std::ifstream file;
        // handle exceptions when opening the json file
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try {
            file.open(filename);
            std::stringstream buffer;
            buffer << file.rdbuf();
            return WfCommonsWorkflowParser::createWorkflowFromJSONString(
                    buffer.str(), reference_flop_rate, ignore_machine_specs,
                    redundant_dependencies,
                    ignore_cycle_creating_dependencies,
                    min_cores_per_task,
                    max_cores_per_task,
                    enforce_num_cores,
                    ignore_avg_cpu,
                    show_warnings);
        } catch (const std::ifstream::failure &e) {
            throw std::invalid_argument("WfCommonsWorkflowParser::createWorkflowFromJson(): Invalid JSON file (" + std::string(e.what()) + ")");
        }
    }

    /**
    * Documentation in .h file
    */
    std::shared_ptr<Workflow> WfCommonsWorkflowParser::createWorkflowFromJSONString(const std::string &json_string,
                                                                                    const std::string &reference_flop_rate,
                                                                                    bool ignore_machine_specs,
                                                                                    bool redundant_dependencies,
                                                                                    bool ignore_cycle_creating_dependencies,
                                                                                    unsigned long min_cores_per_task,
                                                                                    unsigned long max_cores_per_task,
                                                                                    bool enforce_num_cores,
                                                                                    bool ignore_avg_cpu,
                                                                                    bool show_warnings) {

        std::set<std::string> ignored_auxiliary_jobs;
        std::set<std::string> ignored_transfer_jobs;

        // Create a new workflow object. Note that we do not use the name
        // in the WfInstance but instead generate a generic unique name
        // in the constructor below
        auto workflow = Workflow::createWorkflow();
        workflow->enableTopBottomLevelDynamicUpdates(false);

        double flop_rate;

        try {
            flop_rate = UnitParser::parse_compute_speed(reference_flop_rate);
        } catch (std::invalid_argument &e) {
            throw;
        }

        nlohmann::json j = nlohmann::json::parse(json_string);

        // Check schema version
        nlohmann::json schema_version;
        try {
            schema_version = j.at("schemaVersion");
        } catch (std::out_of_range &e) {
            throw std::invalid_argument("WfCommonsWorkflowParser::createWorkflowFromJson(): Could not find a 'schema_version' key");
        }
        if (schema_version != "1.5") {
            throw std::invalid_argument("WfCommonsWorkflowParser::createWorkflowFromJson(): Only handles WfFormat schema version 1.5 "
                                        "(use the script at https://github.com/wfcommons/WfFormat/tree/main/tools/ to update your workflow instances).");
        }

        nlohmann::json workflow_spec;
        try {
            workflow_spec = j.at("workflow");
        } catch (std::out_of_range &e) {
            throw std::invalid_argument("WfCommonsWorkflowParser::createWorkflowFromJson(): Could not find a 'workflow' key");
        }


        // Require the workflow/execution key
        if (not workflow_spec.contains("execution")) {
            throw std::invalid_argument("WfCommonsWorkflowParser::createWorkflowFromJson(): The WfInstance doesn't contain a 'workflow/execution' key. "
                                        "Although this key isn't required in the WfInstances format, WRENCH requires it to determine task "
                                        "flop rates based on measured task execution times.");
        }

        // Gather machine information if any
        std::map<std::string, std::pair<unsigned long, double>> machines;
        if (workflow_spec.at("execution").contains("machines")) {
            auto machine_specs = workflow_spec.at("execution").at("machines");
            for (auto const &machine_spec: machine_specs) {
                std::string name = machine_spec.at("nodeName");
                nlohmann::json core_spec = machine_spec.at("cpu");
                unsigned long num_cores;
                try {
                    num_cores = core_spec.at("coreCount");
                } catch (nlohmann::detail::out_of_range &e) {
                    num_cores = 1;
                }
                double mhz;
                try {
                    mhz = core_spec.at("speedInMHz");
                } catch (nlohmann::detail::out_of_range &e) {
                    if (show_warnings) std::cerr << "[WARNING]: Machine " + name + " does not define a speed\n";
                    mhz = -1.0;// unknown
                }
                machines[name] = std::make_pair(num_cores, mhz);
            }
        }

        // Process the files, if any
        if (workflow_spec.at("specification").contains("files")) {
            auto file_specs = workflow_spec.at("specification").at("files");
            for (auto const &file_spec: file_specs) {
                std::string file_name = file_spec.at("id");
                std::shared_ptr<DataFile> data_file;
                try {
                    Simulation::getFileByID(file_name);
                } catch (const std::invalid_argument &e) {
                    // making a new file
                    double file_size = file_spec.at("sizeInBytes");
                    Simulation::addFile(file_name, file_size);
                }
            }
        }

        // Create the tasks with input/output files
        auto task_specs = workflow_spec.at("specification").at("tasks");
        for (const auto &task_spec: task_specs) {
            auto task_name = task_spec.at("name");
            auto task_id = task_spec.at("id");
            // Create a task with all kinds of default fields for now
            //            std::cerr << "CREATED TASK: " << task_id << "\n";
            auto task = workflow->addTask(task_id, 0.0, 1, 1, 0.0);

            if (task_spec.contains("inputFiles")) {
                for (auto const &f: task_spec.at("inputFiles")) {
                    auto file = Simulation::getFileByID(f);
                    task->addInputFile(file);
                    //                    std::cerr << "  ADDED INPUT FILE: " << file->getID() << "\n";
                }
            }
            if (task_spec.contains("outputFiles")) {
                for (auto const &f: task_spec.at("outputFiles")) {
                    auto file = Simulation::getFileByID(f);
                    task->addOutputFile(file);
                    //                    std::cerr << "  ADDED OUTPUT FILE: " << file->getID() << "\n";
                }
            }
        }

        // Fill in the task specifications based on the execution
        auto task_execs = workflow_spec.at("execution").at("tasks");
        for (const auto &task_exec: task_execs) {
            auto task = workflow->getTaskByID(task_exec.at("id"));

            // Deal with the runtime
            double avg_cpu = -1.0;
            try {
                avg_cpu = task_exec.at("avgCPU");
            } catch (nlohmann::json::out_of_range &e) {
                // do nothing
            }

            double num_cores = -1.0;
            try {
                num_cores = task_exec.at("coreCount");
            } catch (nlohmann::json::out_of_range &e) {
                // do nothing
            }

            double runtimeInSeconds = task_exec.at("runtimeInSeconds");
            // Scale runtime based on avgCPU unless disabled
            if (not ignore_avg_cpu) {
                if ((num_cores < 0) and (avg_cpu < 0)) {
                    if (show_warnings) std::cerr << "[WARNING]: Task " << task->getID() << " does not specify a number of cores or an avgCPU: "
                                                                                           "Assuming 1 core and avgCPU at 100%.\n";
                    num_cores = 1.0;
                    avg_cpu = 100.0;
                } else if (num_cores < 0) {
                    if (show_warnings) std::cerr << "[WARNING]: Task " << task->getID() << " has avgCPU " << avg_cpu
                                                 << "% but does not specify the number of cores:"
                                                 << "Assuming " << std::ceil(avg_cpu / 100.0) << " cores\n";
                    num_cores = std::ceil(avg_cpu / 100.0);
                } else if (avg_cpu < 0) {
                    if (show_warnings) std::cerr << "[WARNING]: Task " + task->getID() + " does not specify avgCPU: "
                                                                                         "Assuming 100%.\n";
                    avg_cpu = 100.0 * num_cores;
                } else if (avg_cpu > 100 * num_cores) {
                    if (show_warnings) {
                        std::cerr << "[WARNING]: Task " << task->getID() << " specifies " << (unsigned long) num_cores << " cores and avgCPU " << avg_cpu << "%, "
                                  << "which is impossible: Assuming avgCPU " << 100.0 * num_cores << " instead.\n";
                    }
                    avg_cpu = 100.0 * num_cores;
                }
            } else {
                avg_cpu = 100.0 * num_cores;
            }

            runtimeInSeconds = runtimeInSeconds * avg_cpu / (100.0 * num_cores);

            // Deal with the number of cores
            unsigned long min_num_cores, max_num_cores;
            // Set the default values
            min_num_cores = min_cores_per_task;
            max_num_cores = max_cores_per_task;
            // Overwrite the default is we don't enforce the default values AND the JSON specifies core numbers
            if ((not enforce_num_cores) and task_exec.contains("coreCount")) {
                min_num_cores = task_exec.at("coreCount");
                max_num_cores = task_exec.at("coreCount");
            }

            // Deal with the flop amount
            double flop_amount;
            std::string execution_machine;
            if (task_exec.contains("machines")) {
                std::vector<std::string> machines = task_exec.at("machines");
                if (machines.size() > 1) {
                    throw std::invalid_argument("WfCommonsWorkflowParser::createWorkflowFromJSON(): Task " + task->getID() +
                                                " was executed on multiple machines, which WRENCH currently does not support");
                }
                execution_machine = machines.at(0);
            }
            if (ignore_machine_specs or execution_machine.empty()) {
                flop_amount = runtimeInSeconds * flop_rate;
            } else {
                if (machines.find(execution_machine) == machines.end()) {
                    throw std::invalid_argument("WfCommonsWorkflowParser::createWorkflowFromJSON(): Task " + task->getID() +
                                                " is said to have been executed on machine " + execution_machine +
                                                " but no description for that machine is found on the JSON file");
                }
                if (machines[execution_machine].second >= 0) {
                    double core_ghz = (machines[execution_machine].second) / 1000.0;
                    double total_compute_power_used = core_ghz * (double) min_num_cores;
                    double actual_flop_rate = total_compute_power_used * 1000.0 * 1000.0 * 1000.0;
                    flop_amount = runtimeInSeconds * actual_flop_rate;
                } else {
                    flop_amount = (double) min_num_cores * runtimeInSeconds * flop_rate;// Assume a min-core execution
                }
            }

            // Deal with RAM, if any
            double ram_in_bytes = 0.0;
            if (task_exec.contains("memoryInBytes")) {
                ram_in_bytes = task_exec.at("memoryInBytes");
            }

            // Update the actual task data structure
            task->setFlops(flop_amount);
            task->setMinNumCores(min_num_cores);
            task->setMaxNumCores(max_num_cores);
            task->setMemoryRequirement(ram_in_bytes);

            // Deal with the priority, if any
            if (task_exec.contains("priority")) {
                long priority = task_exec.at("priority");
                task->setPriority(priority);
            }

            // Deal with written/read bytes, if any
            if (task_exec.contains("readBytes")) {
                unsigned long readBytes = task_exec.at("readBytes");
                task->setBytesRead(readBytes);
            }
            if (task_exec.contains("writtenBytes")) {
                unsigned long readBytes = task_exec.at("writtenBytes");
                task->setBytesWritten(readBytes);
            }
        }

        // Deal with task dependencies
        for (auto const &task_spec: task_specs) {
            auto task = workflow->getTaskByID(task_spec.at("id"));

            for (auto const &parent: task_spec.at("parents")) {
                try {
                    auto parent_task = workflow->getTaskByID(parent);
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

            for (auto const &child: task_spec.at("children")) {
                try {
                    auto child_task = workflow->getTaskByID(child);
                    workflow->addControlDependency(task, child_task, redundant_dependencies);
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

        // Update all top/bottom levels computations again
        workflow->enableTopBottomLevelDynamicUpdates(true);
        workflow->updateAllTopBottomLevels();

        return workflow;
    }


};// namespace wrench
