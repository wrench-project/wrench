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
#include <boost/date_time/posix_time/posix_time.hpp>

WRENCH_LOG_CATEGORY(wfcommons_workflow_parser, "Log category for WfCommonsWorkflowParser");


namespace wrench {
    /**
     * Documentation in .h file
     */
    std::shared_ptr<Workflow> WfCommonsWorkflowParser::createWorkflowFromJSON(const std::string& filename,
                                                                              const std::string& reference_flop_rate,
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
        }
        catch (const std::ifstream::failure& e) {
            throw std::invalid_argument(
                "WfCommonsWorkflowParser::createWorkflowFromJson(): Invalid JSON file (" + std::string(e.what()) + ")");
        }
    }

    /**
    * Documentation in .h file
    */
    std::shared_ptr<Workflow> WfCommonsWorkflowParser::createWorkflowFromJSONString(const std::string& json_string,
        const std::string& reference_flop_rate,
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
        }
        catch (std::invalid_argument&) {
            throw;
        }

        nlohmann::json j = nlohmann::json::parse(json_string);

        // Check schema version
        nlohmann::json schema_version;
        try {
            schema_version = j.at("schemaVersion");
        } catch (std::out_of_range&) {
            throw std::invalid_argument(
                "WfCommonsWorkflowParser::createWorkflowFromJson(): Could not find a 'schema_version' key");
        }
        if (schema_version != "1.5") {
            throw std::invalid_argument(
                "WfCommonsWorkflowParser::createWorkflowFromJson(): Only handles WfFormat schema version 1.5 "
                "(use the script at https://github.com/wfcommons/WfFormat/tree/main/tools/ to update your workflow instances).");
        }

        nlohmann::json workflow_spec;
        try {
            workflow_spec = j.at("workflow");
        }
        catch (std::out_of_range&) {
            throw std::invalid_argument(
                "WfCommonsWorkflowParser::createWorkflowFromJson(): Could not find a 'workflow' key");
        }


        // Require the workflow/execution key
        if (not workflow_spec.contains("execution")) {
            throw std::invalid_argument(
                "WfCommonsWorkflowParser::createWorkflowFromJson(): The WfInstance doesn't contain a 'workflow/execution' key. "
                "Although this key isn't required in the WfInstances format, WRENCH requires it to determine task "
                "flop rates based on measured task execution times.");
        }

        // Gather machine information if any
        std::map<std::string, std::pair<unsigned long, double>> machines;
        if (workflow_spec.at("execution").contains("machines")) {
            auto machine_specs = workflow_spec.at("execution").at("machines");
            for (auto const& machine_spec : machine_specs) {
                std::string name = machine_spec.at("nodeName");
                nlohmann::json core_spec = machine_spec.at("cpu");
                unsigned long num_cores;
                try {
                    num_cores = core_spec.at("coreCount");
                }
                catch (nlohmann::detail::out_of_range&) {
                    num_cores = 1;
                } catch (nlohmann::detail::type_error& e) {
                    throw std::invalid_argument(
                        "WfCommonsWorkflowParser::createWorkflowFromJson(): Invalid coreCount value: " + std::string(
                            e.what()));
                }
                double mhz;
                try {
                    mhz = core_spec.at("speedInMHz");
                }
                catch (nlohmann::detail::out_of_range&) {
                    if (show_warnings) std::cerr << "[WARNING]: Machine " + name + " does not define a speed\n";
                    mhz = -1.0; // unknown
                } catch (nlohmann::detail::type_error& e) {
                    throw std::invalid_argument(
                        "WfCommonsWorkflowParser::createWorkflowFromJson(): Invalid speedInMhz value: " + std::string(
                            e.what()));
                }
                machines[name] = std::make_pair(num_cores, mhz);
            }
        }

        // Process the files, if any
        if (workflow_spec.at("specification").contains("files")) {
            auto file_specs = workflow_spec.at("specification").at("files");
            for (auto const& file_spec : file_specs) {
                std::string file_name = file_spec.at("id");
                std::shared_ptr<DataFile> data_file;
                try {
                    Simulation::getFileByID(file_name);
                }
                catch (const std::invalid_argument&) {
                    // making a new file
                    sg_size_t file_size = file_spec.at("sizeInBytes");
                    Simulation::addFile(file_name, file_size);
                }
            }
        }

        // Create the tasks with input/output files
        auto task_specs = workflow_spec.at("specification").at("tasks");
        for (const auto& task_spec : task_specs) {
            auto task_name = task_spec.at("name");
            auto task_id = task_spec.at("id");
            // Create a task with all kinds of default fields for now
            //            std::cerr << "CREATED TASK: " << task_id << "\n";
            auto task = workflow->addTask(task_id, 0.0, 1, 1, 0.0);

            if (task_spec.contains("inputFiles")) {
                for (auto const& f : task_spec.at("inputFiles")) {
                    auto file = Simulation::getFileByID(f);
                    task->addInputFile(file);
                    //                    std::cerr << "  ADDED INPUT FILE: " << file->getID() << "\n";
                }
            }
            if (task_spec.contains("outputFiles")) {
                for (auto const& f : task_spec.at("outputFiles")) {
                    auto file = Simulation::getFileByID(f);
                    task->addOutputFile(file);
                    //                    std::cerr << "  ADDED OUTPUT FILE: " << file->getID() << "\n";
                }
            }
        }

        // Fill in the task specifications based on the execution
        auto task_execs = workflow_spec.at("execution").at("tasks");
        for (const auto& task_exec : task_execs) {
            auto task = workflow->getTaskByID(task_exec.at("id"));

            // Deal with the runtime
            double avg_cpu = -1.0;
            try {
                avg_cpu = task_exec.at("avgCPU");
            } catch (nlohmann::json::out_of_range&) {
                // do nothing
            } catch (nlohmann::detail::type_error& e) {
                throw std::invalid_argument(
                    "WfCommonsWorkflowParser::createWorkflowFromJson(): Invalid avgCPU value: " + std::string(
                        e.what()));
            }

            unsigned long num_cores = 0;
            try {
                num_cores = task_exec.at("coreCount");
            } catch (nlohmann::json::out_of_range&) {
                // do nothing
            } catch (nlohmann::detail::type_error& e) {
                throw std::invalid_argument(
                    "WfCommonsWorkflowParser::createWorkflowFromJson(): Invalid coreCount value: " + std::string(
                        e.what()));
            }
            if (num_cores == 0) {
                if (show_warnings) std::cerr << "[WARNING]: Task " << task->getID() <<
                    " specifies an invalid number of cores (" + std::to_string(num_cores) +
                    "): Assuming 1 core instead.\n";
                num_cores = 1;
            }

            double runtimeInSeconds;
            try {
                runtimeInSeconds = task_exec.at("runtimeInSeconds");
            } catch (nlohmann::detail::type_error& e) {
                throw std::invalid_argument(
                    "WfCommonsWorkflowParser::createWorkflowFromJson(): Invalid runtimeInSeconds value: " + std::string(
                        e.what()));
            }

            // Scale runtime based on avgCPU unless disabled
            if (not ignore_avg_cpu) {
                if (avg_cpu < 0) {
                    if (show_warnings)
                        std::cerr << "[WARNING]: Task " << task->getID() <<
                            " does not specify an avgCPU: "
                            "Assuming avgCPU at 100%.\n";
                    avg_cpu = 100.0;
                } else if (avg_cpu > 100.0 * num_cores) {
                    if (show_warnings) {
                        std::cerr << "[WARNING]: Task " << task->getID() << " specifies " << static_cast<unsigned long>(
                                num_cores) << " cores and avgCPU " << avg_cpu << "%, "
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
                min_num_cores = num_cores;
                max_num_cores = num_cores;
            }

            // Deal with the flop amount
            double flop_amount;
            std::string execution_machine;
            if (task_exec.contains("machines") and !machines.empty()) {
                std::vector<std::string> task_machines = task_exec.at("machines");
                if (task_machines.size() > 1) {
                    throw std::invalid_argument(
                        "WfCommonsWorkflowParser::createWorkflowFromJSON(): Task " + task->getID() +
                        " was executed on multiple machines, which WRENCH currently does not support");
                }
                execution_machine = task_machines.at(0);
            }
            if (ignore_machine_specs or execution_machine.empty()) {
                flop_amount = runtimeInSeconds * flop_rate;
            }
            else {
                if (machines.find(execution_machine) == machines.end()) {
                    throw std::invalid_argument(
                        "WfCommonsWorkflowParser::createWorkflowFromJSON(): Task " + task->getID() +
                        " is said to have been executed on machine " + execution_machine +
                        " but no description for that machine is found on the JSON file");
                }
                if (machines[execution_machine].second >= 0) {
                    double core_ghz = (machines[execution_machine].second) / 1000.0;
                    double total_compute_power_used = core_ghz * static_cast<double>(min_num_cores);
                    double actual_flop_rate = total_compute_power_used * 1000.0 * 1000.0 * 1000.0;
                    flop_amount = runtimeInSeconds * actual_flop_rate;
                }
                else {
                    flop_amount = static_cast<double>(min_num_cores) * runtimeInSeconds * flop_rate;
                    // Assume a min-core execution
                }
            }

            // Deal with RAM, if any
            double ram_in_bytes = 0.0;
            if (task_exec.contains("memoryInBytes")) {
                try {
                    ram_in_bytes = task_exec.at("memoryInBytes");
                }
                catch (nlohmann::detail::type_error& e) {
                    throw std::invalid_argument(
                        "WfCommonsWorkflowParser::createWorkflowFromJson(): Invalid memoryInBytes value: " +
                        std::string(e.what()));
                }
            }

            // Update the actual task data structure
            task->setFlops(flop_amount);
            task->setMinNumCores(min_num_cores);
            task->setMaxNumCores(max_num_cores);
            task->setMemoryRequirement(static_cast<sg_size_t>(ram_in_bytes));

            // Deal with the priority, if any
            if (task_exec.contains("priority")) {
                long priority;
                try {
                    priority = task_exec.at("priority");
                }
                catch (nlohmann::detail::type_error& e) {
                    throw std::invalid_argument(
                        "WfCommonsWorkflowParser::createWorkflowFromJson(): Invalid priority value: " + std::string(
                            e.what()));
                }
                task->setPriority(priority);
            }

            // Deal with written/read bytes, if any
            if (task_exec.contains("readBytes")) {
                unsigned long readBytes;
                try {
                    readBytes = task_exec.at("readBytes");
                }
                catch (nlohmann::detail::type_error& e) {
                    throw std::invalid_argument(
                        "WfCommonsWorkflowParser::createWorkflowFromJson(): Invalid readBytes value: " + std::string(
                            e.what()));
                }
                task->setBytesRead(readBytes);
            }
            if (task_exec.contains("writtenBytes")) {
                unsigned long readBytes;
                try {
                    readBytes = task_exec.at("writtenBytes");
                }
                catch (nlohmann::detail::type_error& e) {
                    throw std::invalid_argument(
                        "WfCommonsWorkflowParser::createWorkflowFromJson(): Invalid writtenBytes value: " + std::string(
                            e.what()));
                }
                task->setBytesWritten(readBytes);
            }
        }

        // Deal with task dependencies
        for (auto const& task_spec : task_specs) {
            auto task = workflow->getTaskByID(task_spec.at("id"));

            for (auto const& parent : task_spec.at("parents")) {
                try {
                    auto parent_task = workflow->getTaskByID(parent);
                    workflow->addControlDependency(parent_task, task, redundant_dependencies);
                }
                catch (std::invalid_argument&) {
                    // do nothing
                } catch (std::runtime_error&) {
                    if (ignore_cycle_creating_dependencies) {
                        // nothing
                    }
                    else {
                        throw;
                    }
                }
            }

            for (auto const& child : task_spec.at("children")) {
                try {
                    auto child_task = workflow->getTaskByID(child);
                    workflow->addControlDependency(task, child_task, redundant_dependencies);
                }
                catch (std::invalid_argument&) {
                    // do nothing
                } catch (std::runtime_error&) {
                    if (ignore_cycle_creating_dependencies) {
                        // nothing
                    }
                    else {
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

    /**
     * @brief Helper method to build the workflow's JSON specification
     * @param workflow
     * @return
     */
    nlohmann::json build_workflow_specification_json(const std::shared_ptr<Workflow>& workflow) {
        nlohmann::json json_specification;

        // Tasks
        json_specification["tasks"] = nlohmann::json::array();
        for (const auto& task : workflow->getTasks()) {
            nlohmann::json json_task;
            json_task["name"] = task->getID();
            json_task["id"] = task->getID();
            nlohmann::json parent_tasks = nlohmann::json::array();
            for (const auto& parent : task->getParents()) {
                parent_tasks.push_back(parent->getID());
            }
            nlohmann::json child_tasks = nlohmann::json::array();
            for (const auto& child : task->getChildren()) {
                child_tasks.push_back(child->getID());
            }
            nlohmann::json input_files = nlohmann::json::array();
            for (const auto& file : task->getInputFiles()) {
                input_files.push_back(file->getID());
            }
            nlohmann::json output_files = nlohmann::json::array();
            for (const auto& file : task->getOutputFiles()) {
                output_files.push_back(file->getID());
            }
            json_task["parents"] = parent_tasks;
            json_task["children"] = child_tasks;
            json_task["inputFiles"] = input_files;
            json_task["outputFiles"] = output_files;

            json_specification["tasks"].push_back(json_task);
        }

        // Files
        json_specification["files"] = nlohmann::json::array();
        for (const auto& item : workflow->getFileMap()) {
            auto file = item.second;
            nlohmann::json json_file;
            json_file["id"] = file->getID();
            json_file["sizeInBytes"] = static_cast<long long>(file->getSize());
            json_specification["files"].push_back(json_file);
        }

        return json_specification;
    }

    /**
     * @brief Helper method to build the workflow's JSON execution
     * @param workflow
     * @return
     */
    nlohmann::json build_workflow_execution_json(const std::shared_ptr<Workflow>& workflow) {
        nlohmann::json json_execution;

        json_execution["makespanInSeconds"] = workflow->getCompletionDate() - workflow->getStartDate();
        boost::posix_time::ptime t = boost::posix_time::microsec_clock::universal_time();
        json_execution["executedAt"] = to_iso_extended_string(t);

        std::set<std::string> used_machines;

        // Tasks
        json_execution["tasks"] = nlohmann::json::array();
        for (const auto& task : workflow->getTasks()) {
            nlohmann::json json_task;
            json_task["id"] = task->getID();
            json_task["runtimeInSeconds"] = task->getEndDate() - task->getStartDate();
            json_task["coreCount"] = task->getNumCoresAllocated();
            nlohmann::json json_machines;
            auto execution_host = task->getPhysicalExecutionHost();
            json_execution["tasks"].push_back(json_task);
        }

        return json_execution;
    }


    /**
   * Documentation in .h file
   */
    std::string WfCommonsWorkflowParser::createJSONStringFromWorkflow(const std::shared_ptr<Workflow>& workflow) {
        nlohmann::json json_doc;
        json_doc["name"] = workflow->getName();
        json_doc["description"] = "Generated from a WRENCH simulator";
        boost::posix_time::ptime t = boost::posix_time::microsec_clock::universal_time();
        json_doc["createAt"] = to_iso_extended_string(t);
        json_doc["schemaVersion"] = "1.5";

        nlohmann::json json_workflow;

        json_workflow["specification"] = build_workflow_specification_json(workflow);

        if (workflow->isDone()) {
            json_workflow["execution"] = build_workflow_execution_json(workflow);
        }

        json_doc["workflow"] = json_workflow;
        return json_doc.dump();
    }
}; // namespace wrench
