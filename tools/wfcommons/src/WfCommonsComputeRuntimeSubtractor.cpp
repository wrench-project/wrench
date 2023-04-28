/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench-dev.h>

#include <iostream>
#include <vector>
#include <fstream>
#include <nlohmann/json.hpp>


nlohmann::json read_from_file(char *path) {
    nlohmann::json json_object;
    try {
        std::ifstream file;
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        file.open(path);
        file >> json_object;
    } catch (const std::ifstream::failure &e) {
        std::cerr << "Error reading in workflow file at " + std::string(path) + ": " + std::string(e.what()) + "\n";
        exit(1);
    }
    return json_object;
}

std::map<std::string, double> get_task_runtimes(const nlohmann::json &workflow) {
    std::map<std::string, double> to_return;
    try {
        auto tasks = workflow.at("workflow").at("tasks");
        for (auto &task: tasks) {
            if (task.find("type") == task.end()) {
                std::cerr << "Error processing JSON: Missing 'type' for task " << task["name"] << "\n";
                exit(1);
            }
            if (task["type"] != "compute") {
                continue;
            }
            if (task.find("id") == task.end()) {
                std::cerr << "Error processing JSON: Missing 'id' for task " << task["name"] << "\n";
                exit(1);
            }
            if (task.find("runtimeInSeconds") == task.end()) {
                std::cerr << "Error processing JSON: Missing 'runtimeInSeconds' for task " << task["name"] << "\n";
                exit(1);
            }
            to_return[task["id"]] = task["runtimeInSeconds"];
        }
    } catch (std::exception &e) {
        std::cerr << "Error processing JSON: " << e.what() << "\n";
        exit(1);
    }
    return to_return;
}


int main(int argc, char **argv) {

    bool valid_args = (argc == 3) or ((argc == 4) and (std::string(argv[3]) == "--set-avgCPU-to-100"));
    if (not valid_args) {
        std::cerr << "Usage: " << argv[0] << " <path to JSON workflow #1> <path to JSON workflow #2> [--set-avgCPU-to-100]\n\n";
        std::cerr << "  This program takes as input two WfCommons workflow instance JSON files that correspond to the same "
                     "workflow (i.e. identical set of 'compute' tasks). It outputs on stdout the JSON for the first workflow "
                     "but where the runtimeInSeconds value of each 'compute' task has been reduced by the runtimeInSeconds value "
                     "for that task in the second workflow.";
        std::cerr << " This is useful because the runtimeInSeconds of WfCommons workflow instances includes both CPU and I/O. As a "
                     "result, before importing a WfCommons workflow instance into a WRENCH simulation, and if a 'zero-cpu-work' execution "
                     "of the workflow is available, this program makes it possible to correct task runtimeInSeconds values to discount "
                     "the I/O part of task executions (this is, in general, a coarse approximation since I/O and computation can be concurrent). "
                     "This program "
                     "is typically used for WfBench-generated instances, for which zero-cpu-work execution are easily obtained and for which "
                     "there is no concurrent I/O and computation.\n";
        exit(1);
    }

    bool set_avgCPU_to_100 = (argc == 4);

    // Read in the original workflow into a JSON object
    nlohmann::json original_workflow_json = read_from_file(argv[1]);
    nlohmann::json zero_cpu_workflow_json = read_from_file(argv[2]);

    // Get the task runtimes
    std::map<std::string, double> original_runtimeInSeconds = get_task_runtimes(original_workflow_json);
    std::map<std::string, double> zero_cpu_runtimeInSeconds = get_task_runtimes(zero_cpu_workflow_json);

    // Check that each task in the original workflow appears in the zero_cpu workflow
    for (const auto &task: original_runtimeInSeconds) {
        if (zero_cpu_runtimeInSeconds.find(task.first) == zero_cpu_runtimeInSeconds.end()) {
            std::cerr << "Error: Task with id='" + task.first + "' appears in the original workflow ('" +
                                 std::string(argv[1]) + "') but not in the zero-cpu-work workflow '(" +
                                 std::string(argv[1]) + "')\n";
            exit(1);
        }
    }
    // Update the original task runtimes in JSON
    auto tasks = original_workflow_json.at("workflow").at("tasks");
    nlohmann::json new_tasks;// new array of tasks
    for (auto &task: tasks) {
        if (task["type"] != "compute") {
            continue;
        }
        double updated_runtime = original_runtimeInSeconds[task["id"]] - zero_cpu_runtimeInSeconds[task["id"]];
        nlohmann::json update;
        update["runtimeInSeconds"] = updated_runtime;
        if (set_avgCPU_to_100) {
            double num_cores = 1;
            if (task.find("cores") != task.end()) {
                num_cores = task["cores"];
            }
            update["avgCPU"] = num_cores * 100.0;
        }
        task.update(update);
        new_tasks.push_back(task);
    }

    // Overwrite the initial array of task with the new array
    original_workflow_json["workflow"]["tasks"] = new_tasks;

    // Dump the modified workflow to stdout
    std::cout << original_workflow_json.dump(4) << "\n";
}