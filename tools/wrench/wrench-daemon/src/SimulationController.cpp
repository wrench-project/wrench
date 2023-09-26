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
            std::shared_ptr<Workflow> workflow,
            const std::string &hostname, int sleep_us) : ExecutionController(hostname, "SimulationController"), workflow(workflow), sleep_us(sleep_us) {}

    /**
     * @brief Simulation execution_controller's main method
     * 
     * @return exit code
     */
    int SimulationController::main() {
        // Initial setup
        wrench::TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_RED);

        S4U_Daemon::map_actor_to_recv_mailbox[simgrid::s4u::this_actor::get_pid()] = this->recv_mailbox;

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
                std::pair<std::tuple<unsigned long, double, WRENCH_PROPERTY_COLLECTION_TYPE, WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE>, std::shared_ptr<ComputeService>> spec_vm_to_create;
                std::pair<std::string, std::shared_ptr<ComputeService>> vm_id;
                std::pair<std::shared_ptr<DataFile>, std::shared_ptr<StorageService>> file_lookup_request;

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

                } else if (this->vm_to_create.tryPop(spec_vm_to_create)) {
                    auto cloud_cs = std::dynamic_pointer_cast<CloudComputeService>(spec_vm_to_create.second);
                    auto num_cores = std::get<0>(spec_vm_to_create.first);
                    auto ram_size = std::get<1>(spec_vm_to_create.first);
                    auto properties = std::get<2>(spec_vm_to_create.first);
                    auto payloads = std::get<3>(spec_vm_to_create.first);
                    std::string vm_name;
                    try {
                        vm_name = cloud_cs->createVM(num_cores, ram_size, properties, payloads);
                        this->vm_created.push(std::pair(true, vm_name));
                    } catch (ExecutionException &e) {
                        this->vm_created.push(std::pair(false, e.getCause()->toString()));
                    }

                } else if (this->vm_to_start.tryPop(vm_id)) {
                    auto cloud_cs = std::dynamic_pointer_cast<CloudComputeService>(vm_id.second);
                    auto vm_name = vm_id.first;
                    try {
                        if (not cloud_cs->isVMDown(vm_name)) {
                            throw std::invalid_argument("Cannot start VM because it's not down");
                        }
                        auto bm_cs = cloud_cs->startVM(vm_name);
                        this->compute_service_registry.insert(bm_cs->getName(), bm_cs);
                        this->vm_started.push(std::pair(true, bm_cs->getName()));
                    } catch (ExecutionException &e) {
                        this->vm_created.push(std::pair(false, e.getCause()->toString()));
                    } catch (std::invalid_argument &e) {
                        this->vm_started.push(std::pair(false, e.what()));
                    }

                } else if (this->vm_to_shutdown.tryPop(vm_id)) {

                    auto cloud_cs = std::dynamic_pointer_cast<CloudComputeService>(vm_id.second);
                    auto vm_name = vm_id.first;
                    try {
                        if (not cloud_cs->isVMRunning(vm_name)) {
                            throw std::invalid_argument("Cannot shutdown VM because it's not running");
                        }
                        auto bm_cs = cloud_cs->getVMComputeService(vm_name);

                        this->compute_service_registry.remove(bm_cs->getName());
                        cloud_cs->shutdownVM(vm_name);
                        this->vm_shutdown.push(std::pair(true, vm_name));
                    } catch (ExecutionException &e) {
                        this->vm_shutdown.push(std::pair(false, e.what()));
                    } catch (std::invalid_argument &e) {
                        this->vm_shutdown.push(std::pair(false, e.what()));
                    }

                } else if (this->vm_to_destroy.tryPop(vm_id)) {

                    auto cloud_cs = std::dynamic_pointer_cast<CloudComputeService>(vm_id.second);
                    auto vm_name = vm_id.first;
                    try {
                        if (not cloud_cs->isVMDown(vm_name)) {
                            throw std::invalid_argument("Cannot destroy VM because it's not down");
                        }
                        cloud_cs->destroyVM(vm_name);
                        this->vm_destroyed.push(std::pair(true, vm_name));
                    } catch (std::invalid_argument &e) {
                        this->vm_destroyed.push(std::pair(false, e.what()));
                    }

                } else if (this->file_to_lookup.tryPop(file_lookup_request)) {

                    auto file = file_lookup_request.first;
                    auto ss = file_lookup_request.second;
                    try {
                        bool result = ss->lookupFile(file);
                        this->file_looked_up.push(std::tuple(true, result, ""));
                    } catch (std::invalid_argument &e) {
                        this->file_looked_up.push(std::tuple(false, false, e.what()));
                    }

                } else if (this->vm_to_suspend.tryPop(vm_id)) {

                    auto cloud_cs = std::dynamic_pointer_cast<CloudComputeService>(vm_id.second);
                    auto vm_name = vm_id.first;
                    try {
                        cloud_cs->suspendVM(vm_name);
                        this->vm_suspended.push(std::pair(true, vm_name));
                    } catch (std::invalid_argument &e) {
                        this->vm_suspended.push(std::pair(false, e.what()));
                    }

                } else if (this->vm_to_resume.tryPop(vm_id)) {

                    auto cloud_cs = std::dynamic_pointer_cast<CloudComputeService>(vm_id.second);
                    auto vm_name = vm_id.first;
                    try {
                        cloud_cs->resumeVM(vm_name);
                        this->vm_resumed.push(std::pair(true, vm_name));
                    } catch (std::invalid_argument &e) {
                        this->vm_resumed.push(std::pair(false, e.what()));
                    }

                } else {
                    break;
                }
            }

            // Submit jobs that should be submitted
            while (true) {
                std::tuple<std::shared_ptr<StandardJob>, std::shared_ptr<ComputeService>, std::map<std::string, std::string>> submission_to_do;
                if (this->submissions_to_do.tryPop(submission_to_do)) {
                    WRENCH_INFO("Submitting a job...");
                    this->job_manager->submitJob(std::get<0>(submission_to_do), std::get<1>(submission_to_do), std::get<2>(submission_to_do));
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
            if (time_to_sleep > 0.0) {
                WRENCH_INFO("Sleeping %.2lf seconds", time_to_sleep);
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
     */
    json SimulationController::getStandardJobTasks(json data) {
        std::shared_ptr<StandardJob> job;
        std::string job_name = data["job_name"];
        if (not job_registry.lookup(job_name, job)) {
            throw std::runtime_error("Unknown job '" + job_name + "'");
        }
        json answer;
        std::vector<std::string> task_names;
        for (const auto &t: job->getTasks()) {
            task_names.push_back(t->getID());
        }
        answer["tasks"] = task_names;
        return answer;
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::addBareMetalComputeService(json data) {
        std::string head_host = data["head_host"];
        std::string resource = data["resources"];
        std::string scratch_space = data["scratch_space"];
        std::string property_list = data["property_list"];
        std::string message_payload_list = data["message_payload_list"];

        map<std::string, std::tuple<unsigned long, double>> resources;
        json jsonData = json::parse(resource);
        for (auto it = jsonData.cbegin(); it != jsonData.cend(); ++it) {
            resources.emplace(it.key(), it.value());
        }

        WRENCH_PROPERTY_COLLECTION_TYPE service_property_list;
        jsonData = json::parse(property_list);
        for (auto it = jsonData.cbegin(); it != jsonData.cend(); ++it) {
            auto property_key = ServiceProperty::translateString(it.key());
            service_property_list[property_key] = it.value();
        }

        WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE service_message_payload_list;
        jsonData = json::parse(message_payload_list);
        for (auto it = jsonData.cbegin(); it != jsonData.cend(); ++it) {
            auto message_payload_key = ServiceMessagePayload::translateString(it.key());
            service_message_payload_list[message_payload_key] = it.value();
        }

        // Create the new service
        auto new_service = new BareMetalComputeService(head_host, resources, scratch_space,
                                                       service_property_list, service_message_payload_list);
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
     */
    json SimulationController::addCloudComputeService(json data) {
        std::string hostname = data["head_host"];
        std::vector<std::string> resources = data["resources"];
        std::string scratch_space = data["scratch_space"];
        std::string property_list_string = data["property_list"];
        std::string message_payload_list_string = data["message_payload_list"];

        WRENCH_PROPERTY_COLLECTION_TYPE service_property_list;
        json jsonData = json::parse(property_list_string);
        for (auto it = jsonData.cbegin(); it != jsonData.cend(); ++it) {
            auto property_key = ServiceProperty::translateString(it.key());
            service_property_list[property_key] = it.value();
        }

        WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE service_message_payload_list;
        jsonData = json::parse(message_payload_list_string);
        for (auto it = jsonData.cbegin(); it != jsonData.cend(); ++it) {
            auto message_payload_key = ServiceMessagePayload::translateString(it.key());
            service_message_payload_list[message_payload_key] = it.value();
        }

        // Create the new service
        auto new_service = new CloudComputeService(hostname, resources, scratch_space,
                                                   service_property_list, service_message_payload_list);
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
     */
    json SimulationController::addBatchComputeService(json data) {
        std::string hostname = data["head_host"];
        std::vector<std::string> resources = data["resources"];
        std::string scratch_space = data["scratch_space"];
        std::string property_list_string = data["property_list"];
        std::string message_payload_list_string = data["message_payload_list"];

        WRENCH_PROPERTY_COLLECTION_TYPE service_property_list;
        json jsonData = json::parse(property_list_string);
        for (auto it = jsonData.cbegin(); it != jsonData.cend(); ++it) {
            auto property_key = ServiceProperty::translateString(it.key());
            service_property_list[property_key] = it.value();
        }

        WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE service_message_payload_list;
        jsonData = json::parse(message_payload_list_string);
        for (auto it = jsonData.cbegin(); it != jsonData.cend(); ++it) {
            auto message_payload_key = ServiceMessagePayload::translateString(it.key());
            service_message_payload_list[message_payload_key] = it.value();
        }

        // Create the new service
        auto new_service = new BatchComputeService(hostname, resources, scratch_space,
                                                   service_property_list, service_message_payload_list);
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
     */
    json SimulationController::createVM(json data) {

        std::string cs_name = data["service_name"];
        unsigned long num_cores = data["num_cores"];
        double ram_memory = data["ram_memory"];
        std::string property_list_string = data["property_list"];
        std::string message_payload_list_string = data["message_payload_list"];

        WRENCH_PROPERTY_COLLECTION_TYPE service_property_list;
        json jsonData = json::parse(property_list_string);
        for (auto it = jsonData.cbegin(); it != jsonData.cend(); ++it) {
            auto property_key = ServiceProperty::translateString(it.key());
            service_property_list[property_key] = it.value();
        }

        WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE service_message_payload_list;
        jsonData = json::parse(message_payload_list_string);
        for (auto it = jsonData.cbegin(); it != jsonData.cend(); ++it) {
            auto message_payload_key = ServiceMessagePayload::translateString(it.key());
            service_message_payload_list[message_payload_key] = it.value();
        }

        // Lookup the cloud compute service
        std::shared_ptr<ComputeService> cs;
        if (not this->compute_service_registry.lookup(cs_name, cs)) {
            throw std::runtime_error("Unknown compute service " + cs_name);
        }

        // Push the request into the blocking queue (will be a single one!)
        this->vm_to_create.push(
                std::pair(
                        std::tuple(num_cores,
                                   ram_memory,
                                   service_property_list,
                                   service_message_payload_list),
                        cs));

        // Pool from the shared queue (will be a single one!)
        std::pair<bool, std::string> reply;
        this->vm_created.waitAndPop(reply);
        bool success = std::get<0>(reply);
        if (not success) {
            std::string error_msg = std::get<1>(reply);
            throw std::runtime_error("Cannot create VM: " + error_msg);
        } else {
            json answer;
            answer["vm_name"] = std::get<1>(reply);
            return answer;
        }
    }

    /**
     * REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::startVM(json data) {
        std::string cs_name = data["service_name"];
        std::string vm_name = data["vm_name"];

        // Lookup the cloud compute service
        std::shared_ptr<ComputeService> cs;
        if (not this->compute_service_registry.lookup(cs_name, cs)) {
            throw std::runtime_error("Unknown compute service " + cs_name);
        }

        // Push the request into the blocking queue (will be a single one!)
        this->vm_to_start.push(std::pair(vm_name, cs));

        // Pool from the shared queue (will be a single one!)
        std::pair<bool, std::string> reply;
        this->vm_started.waitAndPop(reply);
        bool success = std::get<0>(reply);
        if (not success) {
            std::string error_msg = std::get<1>(reply);
            throw std::runtime_error("Cannot start VM: " + error_msg);
        } else {
            json answer;
            answer["service_name"] = std::get<1>(reply);
            return answer;
        }
    }

    /**
     * REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::shutdownVM(json data) {
        std::string cs_name = data["service_name"];
        std::string vm_name = data["vm_name"];

        // Lookup the cloud compute service
        std::shared_ptr<ComputeService> cs;
        if (not this->compute_service_registry.lookup(cs_name, cs)) {
            throw std::runtime_error("Unknown compute service " + cs_name);
        }

        // Push the request into the blocking queue (will be a single one!)
        this->vm_to_shutdown.push(std::pair(vm_name, cs));

        // Pool from the shared queue (will be a single one!)
        std::pair<bool, std::string> reply;
        this->vm_shutdown.waitAndPop(reply);
        bool success = std::get<0>(reply);
        if (not success) {
            std::string error_msg = std::get<1>(reply);
            throw std::runtime_error("Cannot shutdown VM: " + error_msg);
        } else {
            return {};
        }
    }

    /**
     * REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::destroyVM(json data) {
        std::string cs_name = data["service_name"];
        std::string vm_name = data["vm_name"];

        // Lookup the cloud compute service
        std::shared_ptr<ComputeService> cs;
        if (not this->compute_service_registry.lookup(cs_name, cs)) {
            throw std::runtime_error("Unknown compute service " + cs_name);
        }

        // Push the request into the blocking queue (will be a single one!)
        this->vm_to_destroy.push(std::pair(vm_name, cs));

        // Pool from the shared queue (will be a single one!)
        std::pair<bool, std::string> reply;
        this->vm_destroyed.waitAndPop(reply);
        bool success = std::get<0>(reply);
        if (not success) {
            std::string error_msg = std::get<1>(reply);
            throw std::runtime_error("Cannot destroy VM: " + error_msg);
        } else {
            return {};
        }
    }

    /**
     * REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::addSimpleStorageService(json data) {
        std::string head_host = data["head_host"];
        std::set<std::string> mount_points = data["mount_points"];

        // Create the new service
        auto new_service = SimpleStorageService::createSimpleStorageService(head_host, mount_points, {}, {});
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
            file = this->workflow->getFileByID(filename);
        } catch (std::invalid_argument &e) {
            throw std::runtime_error("Unknown file " + filename);
        }

        wrench::StorageService::createFileAtLocation(FileLocation::LOCATION(ss, file));

        // Return the expected answer
        return {};
    }

    /**
     * REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::lookupFileAtStorageService(json data) {
        std::string ss_name = data["storage_service_name"];
        std::string filename = data["filename"];

        std::shared_ptr<StorageService> ss;
        if (not this->storage_service_registry.lookup(ss_name, ss)) {
            throw std::runtime_error("Unknown storage service " + ss_name);
        }

        std::shared_ptr<DataFile> file;
        try {
            file = this->workflow->getFileByID(filename);
        } catch (std::invalid_argument &e) {
            throw std::runtime_error("Unknown file " + filename);
        }

        // Push the request into the blocking queue (will be a single one!)
        this->file_to_lookup.push(std::pair(file, ss));

        // Pool from the shared queue (will be a single one!)
        std::tuple<bool, bool, std::string> reply;
        this->file_looked_up.waitAndPop(reply);
        bool success = std::get<0>(reply);
        if (not success) {
            std::string error_msg = std::get<2>(reply);
            throw std::runtime_error("Cannot lookup file:" + error_msg);
        } else {
            json answer;
            answer["result"] = std::get<1>(reply);
            return answer;
        }
    }

    /**
     * REST API Handler
     * @param data JSON input
     * @return JSON output
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
     */
    json SimulationController::createStandardJob(json data) {
        std::vector<std::shared_ptr<WorkflowTask>> tasks;

        for (auto const &name: data["tasks"]) {
            tasks.push_back(this->workflow->getTaskByID(name));
        }

        std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> file_locations;
        for (auto it = data["file_locations"].begin(); it != data["file_locations"].end(); ++it) {
            auto file = this->workflow->getFileByID(it.key());
            std::shared_ptr<StorageService> storage_service;
            this->storage_service_registry.lookup(it.value(), storage_service);
            file_locations[file] = FileLocation::LOCATION(storage_service, file);
        }


        auto job = this->job_manager->createStandardJob(tasks, file_locations);
        this->job_registry.insert(job->getName(), job);
        json answer;
        answer["job_name"] = job->getName();
        return answer;
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::submitStandardJob(json data) {
        std::string job_name = data["job_name"];
        std::string cs_name = data["compute_service_name"];
        std::string service_specific_string = data["service_specific_args"];

        std::map<std::string, std::string> service_specific_args = {};
        json jsonData = json::parse(service_specific_string);
        for (auto it = jsonData.cbegin(); it != jsonData.cend(); ++it) {
            service_specific_args[it.key()] = it.value();
        }

        std::shared_ptr<StandardJob> job;
        if (not this->job_registry.lookup(job_name, job)) {
            throw std::runtime_error("Unknown job " + job_name);
        }

        std::shared_ptr<ComputeService> cs;
        if (not this->compute_service_registry.lookup(cs_name, cs)) {
            throw std::runtime_error("Unknown compute service " + cs_name);
        }

        this->submissions_to_do.push(std::tuple(job, cs, service_specific_args));
        return {};
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::createTask(json data) {

        auto t = this->workflow->addTask(data["name"],
                                         data["flops"],
                                         data["min_num_cores"],
                                         data["max_num_cores"],
                                         data["memory"]);
        return {};
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::getTaskFlops(json data) {
        json answer;
        answer["flops"] = this->workflow->getTaskByID(data["name"])->getFlops();
        return answer;
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::getTaskMinNumCores(json data) {
        json answer;
        answer["min_num_cores"] = this->workflow->getTaskByID(data["name"])->getMinNumCores();
        return answer;
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::getTaskMaxNumCores(json data) {
        json answer;
        answer["max_num_cores"] = this->workflow->getTaskByID(data["name"])->getMaxNumCores();
        return answer;
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::getTaskMemory(json data) {
        json answer;
        answer["memory"] = this->workflow->getTaskByID(data["name"])->getMemoryRequirement();
        return answer;
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output

     */
    json SimulationController::getTaskStartDate(json data) {
        json answer;
        answer["time"] = this->workflow->getTaskByID(data["name"])->getStartDate();
        return answer;
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::getTaskEndDate(json data) {
        json answer;
        answer["time"] = this->workflow->getTaskByID(data["name"])->getEndDate();
        return answer;
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::addFile(json data) {
        auto file = this->workflow->addFile(data["name"], data["size"]);
        return {};
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::getFileSize(json data) {
        auto file = this->workflow->getFileByID(data["file_id"]);
        json answer;
        answer["size"] = file->getSize();
        return answer;
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::addInputFile(json data) {
        auto task = this->workflow->getTaskByID(data["tid"]);
        auto file = this->workflow->getFileByID(data["file"]);
        task->addInputFile(file);
        return {};
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::addOutputFile(json data) {
        auto task = this->workflow->getTaskByID(data["tid"]);
        auto file = this->workflow->getFileByID(data["file"]);
        task->addOutputFile(file);
        return {};
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::getTaskInputFiles(json data) {

        auto task = this->workflow->getTaskByID(data["tid"]);
        auto files = task->getInputFiles();
        json answer;
        std::vector<std::string> file_names;
        for (const auto &f: files) {
            file_names.push_back(f->getID());
        }
        answer["files"] = file_names;
        return answer;
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::getTaskOutputFiles(json data) {

        auto task = this->workflow->getTaskByID(data["tid"]);
        auto files = task->getOutputFiles();
        json answer;
        std::vector<std::string> file_names;
        for (const auto &f: files) {
            file_names.push_back(f->getID());
        }
        answer["files"] = file_names;
        return answer;
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::getInputFiles(json data) {
        auto files = this->workflow->getInputFiles();
        json answer;
        std::vector<std::string> file_names;
        for (const auto &f: files) {
            file_names.push_back(f->getID());
        }
        answer["files"] = file_names;
        return answer;
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::stageInputFiles(json data) {
        std::shared_ptr<StorageService> storage_service;
        std::string service_name = data["storage"];
        if (not this->storage_service_registry.lookup(service_name, storage_service)) {
            throw std::runtime_error("Unknown storage service " + service_name);
        }

        for (auto const &f: this->workflow->getInputFiles()) {
            this->simulation->stageFile(f, storage_service);
        }
        return {};
    }

    /**
     * REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::supportsCompoundJobs(json data) {
        std::string cs_name = data["compute_service_name"];

        std::shared_ptr<ComputeService> cs;
        if (not this->compute_service_registry.lookup(cs_name, cs)) {
            throw std::runtime_error("Unknown compute service " + cs_name);
        }
        json answer;
        answer["result"] = cs->supportsCompoundJobs();
        return answer;
    }

    /**
     * REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::supportsPilotJobs(json data) {
        std::string cs_name = data["compute_service_name"];

        std::shared_ptr<ComputeService> cs;
        if (not this->compute_service_registry.lookup(cs_name, cs)) {
            throw std::runtime_error("Unknown compute service " + cs_name);
        }
        json answer;
        answer["result"] = cs->supportsPilotJobs();
        return answer;
    }

    /**
     * REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::supportsStandardJobs(json data) {
        std::string cs_name = data["compute_service_name"];

        std::shared_ptr<ComputeService> cs;
        if (not this->compute_service_registry.lookup(cs_name, cs)) {
            throw std::runtime_error("Unknown compute service " + cs_name);
        }
        json answer;
        answer["result"] = cs->supportsStandardJobs();
        return answer;
    }

    /**
     * REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::isVMRunning(json data) {
        std::string cs_name = data["compute_service_name"];
        std::string vm_name = data["vm_name"];

        std::shared_ptr<ComputeService> cs;
        if (not this->compute_service_registry.lookup(cs_name, cs)) {
            throw std::runtime_error("Unknown compute service " + cs_name);
        }

        auto cloud_cs = std::dynamic_pointer_cast<CloudComputeService>(cs);
        json answer;
        answer["result"] = cloud_cs->isVMRunning(vm_name);
        return answer;
    }

    /**
     * REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::isVMDown(json data) {
        std::string cs_name = data["compute_service_name"];
        std::string vm_name = data["vm_name"];

        std::shared_ptr<ComputeService> cs;
        if (not this->compute_service_registry.lookup(cs_name, cs)) {
            throw std::runtime_error("Unknown compute service " + cs_name);
        }

        auto cloud_cs = std::dynamic_pointer_cast<CloudComputeService>(cs);
        json answer;
        answer["result"] = cloud_cs->isVMDown(vm_name);
        return answer;
    }

    /**
     * REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::suspendVM(json data) {
        std::string cs_name = data["compute_service_name"];
        std::string vm_name = data["vm_name"];

        // Lookup the cloud compute service
        std::shared_ptr<ComputeService> cs;
        if (not this->compute_service_registry.lookup(cs_name, cs)) {
            throw std::runtime_error("Unknown compute service " + cs_name);
        }

        // Push the request into the blocking queue (will be a single one!)
        this->vm_to_suspend.push(std::pair(vm_name, cs));

        // Pool from the shared queue (will be a single one!)
        std::pair<bool, std::string> reply;
        this->vm_suspended.waitAndPop(reply);
        bool success = std::get<0>(reply);
        if (not success) {
            std::string error_msg = std::get<1>(reply);
            throw std::runtime_error("Cannot suspend VM: " + error_msg);
        } else {
            return {};
        }
    }

    /**
     * REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::isVMSuspended(json data) {
        std::string cs_name = data["compute_service_name"];
        std::string vm_name = data["vm_name"];

        std::shared_ptr<ComputeService> cs;
        if (not this->compute_service_registry.lookup(cs_name, cs)) {
            throw std::runtime_error("Unknown compute service " + cs_name);
        }

        auto cloud_cs = std::dynamic_pointer_cast<CloudComputeService>(cs);
        json answer;
        answer["result"] = cloud_cs->isVMSuspended(vm_name);

        return answer;
    }

    /**
     * REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::resumeVM(json data) {
        std::string cs_name = data["compute_service_name"];
        std::string vm_name = data["vm_name"];

        // Lookup the cloud compute service
        std::shared_ptr<ComputeService> cs;
        if (not this->compute_service_registry.lookup(cs_name, cs)) {
            throw std::runtime_error("Unknown compute service " + cs_name);
        }

        // Push the request into the blocking queue (will be a single one!)
        this->vm_to_resume.push(std::pair(vm_name, cs));

        // Pool from the shared queue (will be a single one!)
        std::pair<bool, std::string> reply;
        this->vm_resumed.waitAndPop(reply);
        bool success = std::get<0>(reply);
        if (not success) {
            std::string error_msg = std::get<1>(reply);
            throw std::runtime_error("Cannot suspend VM: " + error_msg);
        } else {
            return {};
        }
    }
    /**
     * REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::getExecutionHosts(json data) {
        std::string cs_name = data["compute_service_name"];
        std::shared_ptr<ComputeService> cs;
        if (not this->compute_service_registry.lookup(cs_name, cs)) {
            throw std::runtime_error("Unknown compute service " + cs_name);
        }
        auto cloud_cs = std::dynamic_pointer_cast<CloudComputeService>(cs);
        std::vector<std::string> execution_hosts_list = cloud_cs->getExecutionHosts();
        json answer{};
        answer["execution_hosts"] = execution_hosts_list;
        return answer;
    }

    /**
     * REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::getVMPhysicalHostname(json data) {
        std::string cs_name = data["compute_service_name"];
        std::string vm_name = data["vm_name"];
        std::shared_ptr<ComputeService> cs;
        if (not this->compute_service_registry.lookup(cs_name, cs)) {
            throw std::runtime_error("Unknown compute service " + cs_name);
        }

        auto cloud_cs = std::dynamic_pointer_cast<CloudComputeService>(cs);
        json answer;
        answer["physical_host"] = cloud_cs->getVMPhysicalHostname(vm_name);
        return answer;
    }
    /**
     * REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::getVMComputeService(json data) {
        std::string cs_name = data["compute_service_name"];
        std::string vm_name = data["vm_name"];
        std::shared_ptr<ComputeService> cs;
        if (not this->compute_service_registry.lookup(cs_name, cs)) {
            throw std::runtime_error("Unknown compute service " + cs_name);
        }

        auto cloud_cs = std::dynamic_pointer_cast<CloudComputeService>(cs);
        json answer;
        answer["vm_compute_service"] = cloud_cs->getVMComputeService(vm_name)->getName();
        return answer;
    }

}// namespace wrench
