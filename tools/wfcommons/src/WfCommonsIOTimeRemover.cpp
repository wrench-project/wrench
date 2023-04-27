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

std::map<std::string, double> get_task_runtimes(const nlohmann::json& workflow) {
    std::map<std::string, double> to_return;
    try {
        auto tasks = workflow.at("workflow").at("tasks");
        for (auto &task: tasks) {
            if (task["type"] != "compute") {
                continue;
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

    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <original workflow path> <zero-cpu-work workflow path>\n";
        exit(1);
    }

    // Read in the original workflow into a JSON object
    nlohmann::json original_workflow_json = read_from_file(argv[1]);
    nlohmann::json zero_cpu_workflow_json = read_from_file(argv[2]);

    // Get the task runtimes
    std::map<std::string, double> original_runtimeInSeconds = get_task_runtimes(original_workflow_json);
    std::map<std::string, double> zero_cpu_runtimeInSeconds = get_task_runtimes(zero_cpu_workflow_json);

    // Update the original task runtimes in JSON
    auto tasks = original_workflow_json.at("workflow").at("tasks");
    nlohmann::json new_tasks; // new array of tasks
    for (auto &task: tasks) {
        if (task["type"] != "compute") {
            continue;
        }
        double updated_runtime = original_runtimeInSeconds[task["id"]] - zero_cpu_runtimeInSeconds[task["id"]];
        nlohmann::json update;
        update["runtimeInSeconds"] = updated_runtime;
        task.update(update);
        new_tasks.push_back(task);
    }

    // Overwrite the initial array of task with the new array
    original_workflow_json["workflow"]["tasks"] = new_tasks;

    // Dump the modified workflow to stdout
    std::cout << original_workflow_json.dump(4) << "\n";


}