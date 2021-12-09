/**
 * Copyright (c) 2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "SimulationController.h"

#include <random>
#include <iostream>
#include <utility>
#include <unistd.h>

// The timeout use when the SimulationController receives a message
// from the job manager. Can't be zero, but can be very small.
#define JOB_MANAGER_COMMUNICATION_TIMEOUT_VALUE 0.00000001

WRENCH_LOG_CATEGORY(simulation_controller, "Log category for SimulationController");

namespace wrench {

    /**
     * @brief Construct a new SimulationController object
     * 
     * @param hostname string containing the name of the host on which this service runs
     */
    SimulationController::SimulationController(
            const std::string &hostname, int sleep_us) :
            WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "SimulationController"), sleep_us(sleep_us) {}

    /**
     * @brief Simulation execution_controller's main method
     * 
     * @return exit code
     */
    int SimulationController::main() {
        // Initial setup
        wrench::TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_RED);

        WRENCH_INFO("Starting");
        this->job_manager = this->createJobManager();
        this->data_movement_manager = this->createDataMovementManager();

        // Main control loop
        while (keep_going) {
            // Starting compute and storage services that should be started, if any
            while (true) {
                wrench::ComputeService *new_compute_service = nullptr;
                wrench::StorageService *new_storage_service = nullptr;
                wrench::FileRegistryService *new_file_service = nullptr;

                if (this->compute_services_to_start.tryPop(new_compute_service)) {
                    // starting compute service
                    WRENCH_INFO("Starting a new compute service...");
                    auto new_service_shared_ptr = this->simulation->startNewService(new_compute_service);
                    // Add the new service to the registry of existing services, so that later we can look it up by name
                    this->compute_service_registry.insert(new_service_shared_ptr->getName(), new_service_shared_ptr);

                } else if (this->storage_services_to_start.tryPop(new_storage_service)) {
                    // starting storage service
                    WRENCH_INFO("Starting a new storage service...");
                    auto new_service_shared_ptr = this->simulation->startNewService(new_storage_service);
                    // Add the new service to the registry of existing services, so that later we can look it up by name
                    this->storage_service_registry.insert(new_service_shared_ptr->getName(), new_service_shared_ptr);
                
                } else if (this->file_service_to_start.tryPop(new_file_service)) {
                    // start file registry service
                    WRENCH_INFO("Starting a new file registry service...");
                    auto new_service_shared_ptr = this->simulation->startNewService(new_file_service);
                    // Add the new service to the registry of existing services, so that later we can look it up by name
                    this->file_service_registry.insert(new_service_shared_ptr->getName(), new_service_shared_ptr);

                } else {
                    break;
                }
            }

            // Submit jobs that should be submitted
            while (true) {
                std::pair<std::shared_ptr<StandardJob>, std::shared_ptr<ComputeService>> submission_to_do;
                if (this->submissions_to_do.tryPop(submission_to_do)) {
                    WRENCH_INFO("Submitting a job...");
                    this->job_manager->submitJob(submission_to_do.first, submission_to_do.second, {});
                } else {
                    break;
                }
            }

            // If the server thread is waiting for the next event to occur, just do that
            if (time_horizon_to_reach < 0) {
                time_horizon_to_reach = Simulation::getCurrentSimulatedDate();
                auto event = this->waitForNextEvent();
                this->event_queue.push(std::make_pair(Simulation::getCurrentSimulatedDate(), event));
            }

            // Moves time forward if needed (because the client has done a sleep),
            // And then add all events that occurred to the event queue
            double time_to_sleep = std::max<double>(0, time_horizon_to_reach -
                                                       wrench::Simulation::getCurrentSimulatedDate());
            if (time_to_sleep > 0.0) { WRENCH_INFO("Sleeping %.2lf seconds", time_to_sleep);
                S4U_Simulation::sleep(time_to_sleep);
                while (auto event = this->waitForNextEvent(10 * JOB_MANAGER_COMMUNICATION_TIMEOUT_VALUE)) {
                    // Add job onto the event queue
                    event_queue.push(std::make_pair(Simulation::getCurrentSimulatedDate(), event));
                }
            }

            // Sleep since no matter what we're in locked step with client time and don't want
            // to burn CPU cycles like crazy. Could probably sleep 1s...
            usleep(sleep_us);
        }
        return 0;
    }

    /**
     * @brief Sets the flag to stop this service
     */
    void SimulationController::stopSimulation() {
        keep_going = false;
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     * BEGIN_REST_API_DOCUMENTATION
     * {
     *   "REST_func": "advanceTime",
     *   "documentation":
     *     {
     *       "purpose": "Advances current simulated time by some number of seconds",
     *       "json_input": {
     *         "increment": ["double", "increment in seconds"]
     *       },
     *       "json_output": {
     *       }
     *     }
     * }
     * END_REST_API_DOCUMENTATION
     */
    json SimulationController::advanceTime(json data) {
        // Simply set the time_horizon_to_reach variable so that
        // the Controller will catch up to that time
        double increment_in_seconds = data["increment"];
        this->time_horizon_to_reach = Simulation::getCurrentSimulatedDate() + increment_in_seconds;
        return {};
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     * BEGIN_REST_API_DOCUMENTATION
     * {
     *   "REST_func": "getTime",
     *   "documentation":
     *     {
     *       "purpose": "Retrieve the current simulated time",
     *       "json_input": {
     *       },
     *       "json_output": {
     *         "time": ["double", "simulation time in seconds"]
     *       }
     *     }
     * }
     * END_REST_API_DOCUMENTATION
     */
    json SimulationController::getSimulationTime(json data) {
        // This is not called by the simulation thread, but getting the
        // simulation time is fine as it doesn't change the state of the simulation

        json answer;
        answer["time"] = Simulation::getCurrentSimulatedDate();
        return answer;
    }

    /**
     * @brief Construct a json description of a workflow execution event
     * @param event workflow execution event
     * @return json description of the event
     */
    json SimulationController::eventToJSON(double date, const std::shared_ptr<wrench::ExecutionEvent> &event) {
        // Construct the json event description
        std::shared_ptr<wrench::StandardJob> job;
        json event_desc;

        event_desc["event_date"] = date;
        // Deal with the different event types
        if (auto failed = std::dynamic_pointer_cast<wrench::StandardJobFailedEvent>(event)) {
            event_desc["event_type"] = "job_failure";
            event_desc["failure_cause"] = failed->failure_cause->toString();
            job = failed->standard_job;
        } else if (auto complete = std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
            event_desc["event_type"] = "job_completion";
            job = complete->standard_job;
        }

        event_desc["compute_service_name"] = job->getParentComputeService()->getName();
        event_desc["job_name"] = job->getName();
        event_desc["submit_date"] = job->getSubmitDate();
        event_desc["end_date"] = job->getEndDate();

        return event_desc;
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     * BEGIN_REST_API_DOCUMENTATION
     * {
     *   "REST_func": "waitForNextSimulationEvent",
     *   "documentation":
     *     {
     *       "purpose": "Retrieve the next simulation event",
     *       "json_input": {
     *       },
     *       "json_output": {
     *         "event": ["json", "JSON event description"]
     *       }
     *     }
     * }
     * END_REST_API_DOCUMENTATION
     */
    json SimulationController::waitForNextSimulationEvent(json data) {
        // Set the time horizon to -1, to signify the "wait for next event" to the execution_controller
        time_horizon_to_reach = -1.0;
        // Wait for and grab the next event
        std::pair<double, std::shared_ptr<wrench::ExecutionEvent>> event;
        this->event_queue.waitAndPop(event);

        // Construct the json event description
        std::shared_ptr<wrench::StandardJob> job;
        json event_desc = eventToJSON(event.first, event.second);

        // Construct the json answer
        json answer;
        answer["event"] = event_desc;

        return answer;
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     * BEGIN_REST_API_DOCUMENTATION
     * {
     *   "REST_func": "getSimulationEvents",
     *   "documentation":
     *     {
     *       "purpose": "Retrieve all simulation events since last time we checked",
     *       "json_input": {
     *       },
     *       "json_output": {
     *         "events": ["list<json>", "List of JSON event descriptions"]
     *       }
     *     }
     * }
     * END_REST_API_DOCUMENTATION
     */
    json SimulationController::getSimulationEvents(json data) {
        // Deal with all events
        std::pair<double, std::shared_ptr<wrench::ExecutionEvent>> event;

        std::vector<json> json_events;

        while (this->event_queue.tryPop(event)) {
            json event_desc = eventToJSON(event.first, event.second);
            json_events.push_back(event_desc);
        }

        json answer;
        answer["events"] = json_events;
        return answer;
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     * BEGIN_REST_API_DOCUMENTATION
     * {
     *   "REST_func": "getAllHostnames",
     *   "documentation":
     *     {
     *       "purpose": "Retrieve the names of all hosts in the simulated platform",
     *       "json_input": {
     *       },
     *       "json_output": {
     *         "hostnames": ["list<string>", "List of host names"]
     *       }
     *     }
     * }
     * END_REST_API_DOCUMENTATION
     */
    json SimulationController::getAllHostnames(json data) {
        std::vector<std::string> hostname_list = Simulation::getHostnameList();
        json answer = {};
        answer["hostnames"] = hostname_list;
        return answer;
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     * BEGIN_REST_API_DOCUMENTATION
     * {
     *   "REST_func": "standardJobGetTasks",
     *   "documentation":
     *     {
     *       "purpose": "Retrieve the tasks in a standard job",
     *       "json_input": {
     *         "job_name": ["string", "The job's name"]
     *       },
     *       "json_output": {
     *         "tasks": ["list<string>", "A list of task names"]
     *       }
     *     }
     * }
     * END_REST_API_DOCUMENTATION
     */
    json SimulationController::getStandardJobTasks(json data) {
        std::shared_ptr<StandardJob> job;
        std::string job_name = data["job_name"];
        if (not job_registry.lookup(job_name, job)) {
            throw std::runtime_error("Unknown job '" + job_name + "'");
        }
        json answer;
        std::vector<std::string> task_names;
        for (const auto &t : job->getTasks()) {
            task_names.push_back(t->getID());
        }
        answer["tasks"] = task_names;
        return answer;
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     * BEGIN_REST_API_DOCUMENTATION
     * {
     *   "REST_func": "addBareMetalComputeService",
     *   "documentation":
     *     {
     *       "purpose": "Create and start a bare-metal compute service",
     *       "json_input": {
     *         "head_host": ["string", "The service's head host"]
     *       },
     *       "json_output": {
     *         "service_name": ["string", "The new service's name"]
     *       }
     *     }
     * }
     * END_REST_API_DOCUMENTATION
     */
    json SimulationController::addBareMetalComputeService(json data) {
        std::string head_host = data["head_host"];

        // Create the new service
        auto new_service = new BareMetalComputeService(head_host, {head_host}, "", {}, {});
        // Put in the list of services to start (this is because this method is called
        // by the server thread, and therefore, it will segfault horribly if it calls any
        // SimGrid simulation methods, e.g., to start a service)
        this->compute_services_to_start.push(new_service);

        // Return the expected answer
        json answer;
        answer["service_name"] = new_service->getName();
        return answer;
    }

    /**
     * REST API Handler
     * @param data JSON input
     * @return JSON output
     * BEGIN_REST_API_DOCUMENTATION
     * {
     *   "REST_func": "addSimpleStorageService",
     *   "documentation":
     *     {
     *       "purpose": "Create and start a simple storage service",
     *       "json_input": {
     *         "head_host": ["string", "The service's head host"]
     *       },
     *       "json_output": {
     *         "service_name": ["string", "The new service's name"]
     *       }
     *     }
     * }
     * END_REST_API_DOCUMENTATION
     */
    json SimulationController::addSimpleStorageService(json data) {
        std::string head_host = data["head_host"];

        // Create the new service
        auto new_service = new SimpleStorageService(head_host, {"/"}, {}, {});
        // Put in the list of services to start (this is because this method is called
        // by the server thread, and therefore, it will segfault horribly if it calls any
        // SimGrid simulation methods, e.g., to start a service)
        this->storage_services_to_start.push(new_service);

        // Return the expected answer
        json answer;
        answer["service_name"] = new_service->getName();
        return answer;
    }

    /**
     * REST API Handler
     * @param data JSON input
     * @return JSON output
     * BEGIN_REST_API_DOCUMENTATION
     * {
     *   "REST_func": "createFileCopyAtStorageService",
     *   "documentation":
     *     {
     *       "purpose": "Create, ex nihilo, a copy of a file copy at a storage service",
     *       "json_input": {
     *         "storage_service_name": ["string", "The storage service's head host"],
     *         "filename": ["string", "The file name"]
     *       },
     *       "json_output": {
     *       }
     *     }
     * }
     * END_REST_API_DOCUMENTATION
     */
    json SimulationController::createFileCopyAtStorageService(json data) {
        std::string ss_name = data["storage_service_name"];
        std::string filename = data["filename"];

        std::shared_ptr<StorageService> ss;
        if (not this->storage_service_registry.lookup(ss_name, ss)) {
            throw std::runtime_error("Unknown storage service " + ss_name);
        }

        std::shared_ptr<DataFile> file;
        try {
            file = this->getWorkflow()->getFileByID(filename);
        } catch (std::invalid_argument &e) {
            throw std::runtime_error("Unknown file " + filename);
        }

        ss->createFile(file, FileLocation::LOCATION(ss));

        // Return the expected answer
        return {};
    }

    /**
     * REST API Handler
     * @param data JSON input
     * @return JSON output
     * BEGIN_REST_API_DOCUMENTATION
     * {
     *   "REST_func": "addFileRegistryService",
     *   "documentation":
     *     {
     *       "purpose": "Create and start a file registery service",
     *       "json_input": {
     *         "head_host": ["string", "The service's head host"]
     *       },
     *       "json_output": {
     *         "service_name": ["string", "The new service's name"]
     *       }
     *     }
     * }
     * END_REST_API_DOCUMENTATION
     */
    json SimulationController::addFileRegistryService(json data) {
        std::string head_host = data["head_host"];

        // Create the new service
        auto new_service = new FileRegistryService(head_host, {}, {});
        // Put in the list of services to start (this is because this method is called
        // by the server thread, and therefore, it will segfault horribly if it calls any
        // SimGrid simulation methods, e.g., to start a service)
        this->file_service_to_start.push(new_service);

        // Return the expected answer
        json answer;
        answer["service_name"] = new_service->getName();
        return answer;
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     * BEGIN_REST_API_DOCUMENTATION
     * {
     *   "REST_func": "createStandardJob",
     *   "documentation":
     *     {
     *       "purpose": "Create a new standard job",
     *       "json_input": {
     *         "tasks": ["list<string>", "List of task names"]
     *       },
     *       "json_output": {
     *         "job_name": ["string", "The new job's name"]
     *       }
     *     }
     * }
     * END_REST_API_DOCUMENTATION
     */
    json SimulationController::createStandardJob(json data) {
        std::vector<std::shared_ptr<WorkflowTask>> tasks;

        for (auto const &name : data["tasks"]) {
            tasks.push_back(this->getWorkflow()->getTaskByID(name));
        }

        auto job = this->job_manager->createStandardJob(tasks);
        this->job_registry.insert(job->getName(), job);
        json answer;
        answer["job_name"] = job->getName();
        return answer;
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     * BEGIN_REST_API_DOCUMENTATION
     * {
     *   "REST_func": "submitStandardJob",
     *   "documentation":
     *     {
     *       "purpose": "Submit a standard job for execution to a compute service",
     *       "json_input": {
     *         "job_name": ["string", "The job's name"],
     *         "compute_service_name": ["string", "The compute service's name"]
     *       },
     *       "json_output": {
     *       }
     *     }
     * }
     * END_REST_API_DOCUMENTATION
     */
    json SimulationController::submitStandardJob(json data) {
        std::string job_name = data["job_name"];
        std::string cs_name = data["compute_service_name"];

        std::shared_ptr<StandardJob> job;
        if (not this->job_registry.lookup(job_name, job)) {
            throw std::runtime_error("Unknown job " + job_name);
        }

        std::shared_ptr<ComputeService> cs;
        if (not this->compute_service_registry.lookup(cs_name, cs)) {
            throw std::runtime_error("Unknown compute service " + cs_name);
        }

        this->submissions_to_do.push(std::make_pair(job, cs));
        return {};
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     * BEGIN_REST_API_DOCUMENTATION
     * {
     *   "REST_func": "createTask",
     *   "documentation":
     *     {
     *       "purpose": "Create a new task",
     *       "json_input": {
     *         "name": ["string", "The task's name"],
     *         "flops": ["double", "The task's flops"],
     *         "min_num_cores": ["double", "The task's minimum number of cores"],
     *         "max_num_cores": ["double", "The task's maximum number of cores"],
     *         "memory": ["double", "The task's memory requirement"]
     *       },
     *       "json_output": {
     *       }
     *     }
     * }
     * END_REST_API_DOCUMENTATION
     */
    json SimulationController::createTask(json data) {

        this->getWorkflow()->addTask(data["name"],
                                     data["flops"],
                                     data["min_num_cores"],
                                     data["max_num_cores"],
                                     data["memory"]);
        return json({});
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     * BEGIN_REST_API_DOCUMENTATION
     * {
     *   "REST_func": "taskGetFlops",
     *   "documentation":
     *     {
     *       "purpose": "Get a task's flops",
     *       "json_input": {
     *         "name": ["string", "The task's name"]
     *       },
     *       "json_output": {
     *         "flops": ["double", "The task's flops"]
     *       }
     *     }
     * }
     * END_REST_API_DOCUMENTATION
     */
    json SimulationController::getTaskFlops(json data) {
        json answer;
        answer["flops"] = this->getWorkflow()->getTaskByID(data["name"])->getFlops();
        return answer;
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     * BEGIN_REST_API_DOCUMENTATION
     * {
     *   "REST_func": "taskGetMinNumCores",
     *   "documentation":
     *     {
     *       "purpose": "Get a task's minimum number of cores",
     *       "json_input": {
     *         "name": ["string", "The task's name"]
     *       },
     *       "json_output": {
     *         "min_num_cores": ["double", "The task's minimum number of cores"]
     *       }
     *     }
     * }
     * END_REST_API_DOCUMENTATION
     */
    json SimulationController::getTaskMinNumCores(json data) {
        json answer;
        answer["min_num_cores"] = this->getWorkflow()->getTaskByID(data["name"])->getMinNumCores();
        return answer;
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     * BEGIN_REST_API_DOCUMENTATION
     * {
     *   "REST_func": "taskGetMaxNumCores",
     *   "documentation":
     *     {
     *       "purpose": "Get a task's maximum number of cores",
     *       "json_input": {
     *         "name": ["string", "The task's name"]
     *       },
     *       "json_output": {
     *         "max_num_cores": ["double", "The task's maximum number of cores"]
     *       }
     *     }
     * }
     * END_REST_API_DOCUMENTATION
     */
    json SimulationController::getTaskMaxNumCores(json data) {
        json answer;
        answer["max_num_cores"] = this->getWorkflow()->getTaskByID(data["name"])->getMaxNumCores();
        return answer;
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     * BEGIN_REST_API_DOCUMENTATION
     * {
     *   "REST_func": "taskGetMemory",
     *   "documentation":
     *     {
     *       "purpose": "Get a task's memory requirement",
     *       "json_input": {
     *         "name": ["string", "The task's name"]
     *       },
     *       "json_output": {
     *         "memory": ["double", "The task's memory requirement in bytes"]
     *       }
     *     }
     * }
     * END_REST_API_DOCUMENTATION
     */
    json SimulationController::getTaskMemory(json data) {
        json answer;
        answer["memory"] = this->getWorkflow()->getTaskByID(data["name"])->getMemoryRequirement();
        return answer;
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     * BEGIN_REST_API_DOCUMENTATION
     * {
     *   "REST_func": "addFile",
     *   "documentation":
     *     {
     *       "purpose": "Add a file to the workflow",
     *       "json_input": {
     *         "name": ["string", "The file's name"],
     *         "size": ["int", "The file's size in bytes"]
     *       },
     *       "json_output": {
     *       }
     *     }
     * }
     * END_REST_API_DOCUMENTATION
     */
    json SimulationController::addFile(json data) {
        auto file = this->getWorkflow()->addFile(data["name"], data["size"]);
        return {};
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     * BEGIN_REST_API_DOCUMENTATION
     * {
     *   "REST_func": "fileGetSize",
     *   "documentation":
     *     {
     *       "purpose": "Get a file size",
     *       "json_input": {
     *         "name": ["string", "The file's name"]
     *       },
     *       "json_output": {
     *         "size": ["int", "The file's size in bytes"]
     *       }
     *     }
     * }
     * END_REST_API_DOCUMENTATION
     */
    json SimulationController::getFileSize(json data) {
        auto file = this->getWorkflow()->getFileByID(data["name"]);
        json answer;
        answer["size"] = file->getSize();
        return answer;
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     * BEGIN_REST_API_DOCUMENTATION
     * {
     *   "REST_func": "addInputFile",
     *   "documentation":
     *     {
     *       "purpose": "Add an input file to a task",
     *       "json_input": {
     *         "task": ["string", "The task's ID"],
     *         "file": ["string", "The input file's ID"]
     *       },
     *       "json_output": {
     *       }
     *     }
     * }
     * END_REST_API_DOCUMENTATION
     */
    json SimulationController::addInputFile(json data) {
        auto task = this->getWorkflow()->getTaskByID(data["task"]);
        auto file = this->getWorkflow()->getFileByID(data["file"]);
        task->addInputFile(file);
        return {};
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     * BEGIN_REST_API_DOCUMENTATION
     * {
     *   "REST_func": "addOutputFile",
     *   "documentation":
     *     {
     *       "purpose": "Add an output file to a task",
     *       "json_input": {
     *         "task": ["string", "The task's ID"],
     *         "file": ["string", "The output file's ID"]
     *       },
     *       "json_output": {
     *       }
     *     }
     * }
     * END_REST_API_DOCUMENTATION
     */
    json SimulationController::addOutputFile(json data) {
        auto task = this->getWorkflow()->getTaskByID(data["task"]);
        auto file = this->getWorkflow()->getFileByID(data["file"]);
        task->addOutputFile(file);
        return {};
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     * BEGIN_REST_API_DOCUMENTATION
     * {
     *   "REST_func": "getTaskInputFiles",
     *   "documentation":
     *     {
     *       "purpose": "Return the list of input files for a given task",
     *       "json_input": {
     *         "task": ["string", "The task's ID"]
     *       },
     *       "json_output": {
     *         "files": ["list<string>", "A list of input files"]
     *       }
     *     }
     * }
     * END_REST_API_DOCUMENTATION
     */
    json SimulationController::getTaskInputFiles(json data) {
        auto task = this->getWorkflow()->getTaskByID(data["task"]);
        auto files = task->getInputFiles();
        json answer;
        std::vector<std::string> file_names;
        for (const auto &f : files) {
            file_names.push_back(f->getID());
        }
        answer["files"] = file_names;
        return answer;
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     * BEGIN_REST_API_DOCUMENTATION
     * {
     *   "REST_func": "getInputFiles",
     *   "documentation":
     *     {
     *       "purpose": "Return the list of input files of the workflow",
     *       "json_input": {
     *       },
     *       "json_output": {
     *         "files": ["list<string>", "A list of input files"]
     *       }
     *     }
     * }
     * END_REST_API_DOCUMENTATION
     */
    json SimulationController::getInputFiles(json data) {
        auto files = this->getWorkflow()->getInputFiles();
        json answer;
        std::vector<std::string> file_names;
        for (const auto &f : files) {
            file_names.push_back(f->getID());
        }
        answer["files"] = file_names;
        return answer;
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     * BEGIN_REST_API_DOCUMENTATION
     * {
     *   "REST_func": "stageInputFiles",
     *   "documentation":
     *     {
     *       "purpose": "Stage all input files of a workflow on a given storage service",
     *       "json_input": {
     *         "storage": ["string", "Storage service on which stage the file"]
     *       },
     *       "json_output": {
     *       }
     *     }
     * }
     * END_REST_API_DOCUMENTATION
     */
    json SimulationController::stageInputFiles(json data) {
        std::shared_ptr<StorageService> storage_service;
        std::string service_name = data["storage"];
        if (not this->storage_service_registry.lookup(service_name, storage_service)) {
            throw std::runtime_error("Unknown storage service " + service_name);
        }

        for (auto const &f : this->getWorkflow()->getInputFiles()) {
            this->simulation->stageFile(f, storage_service);
        }
        return {};
    }

}
