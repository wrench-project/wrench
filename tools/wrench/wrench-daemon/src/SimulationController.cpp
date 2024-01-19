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

#define PARSE_SERVICE_PROPERTY_LIST()                                       \
    WRENCH_PROPERTY_COLLECTION_TYPE service_property_list;                  \
    {                                                                       \
        json jsonData = json::parse(property_list_string);                  \
        for (auto it = jsonData.cbegin(); it != jsonData.cend(); ++it) {    \
            auto property_key = ServiceProperty::translateString(it.key()); \
            service_property_list[property_key] = it.value();               \
        }                                                                   \
    }

#define PARSE_MESSAGE_PAYLOAD_LIST()                                                     \
    WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE service_message_payload_list;                  \
    {                                                                                    \
        json jsonData = json::parse(message_payload_list_string);                        \
        for (auto it = jsonData.cbegin(); it != jsonData.cend(); ++it) {                 \
            auto message_payload_key = ServiceMessagePayload::translateString(it.key()); \
            service_message_payload_list[message_payload_key] = it.value();              \
        }                                                                                \
    }

namespace wrench {

    /**
     * @brief Construct a new SimulationController object
     * 
     * @param hostname string containing the name of the host on which this service runs
     */
    SimulationController::SimulationController(const std::string &hostname, int sleep_us) : ExecutionController(hostname, "SimulationController"), sleep_us(sleep_us) {}


    template<class T>
    json SimulationController::startService(T *s) {
        BlockingQueue<std::pair<bool, std::string>> s_created;

        this->things_to_do.push([this, s, &s_created]() {
            try {
                auto new_service_shared_ptr = this->simulation->startNewService(s);
                if (auto cs = std::dynamic_pointer_cast<wrench::ComputeService>(new_service_shared_ptr)) {
                    WRENCH_INFO("Started a new compute service");
                    this->compute_service_registry.insert(new_service_shared_ptr->getName(), cs);
                } else if (auto ss = std::dynamic_pointer_cast<wrench::StorageService>(new_service_shared_ptr)) {
                    WRENCH_INFO("Started a new storage service");
                    this->storage_service_registry.insert(new_service_shared_ptr->getName(), ss);
                } else if (auto fs = std::dynamic_pointer_cast<wrench::FileRegistryService>(new_service_shared_ptr)) {
                    WRENCH_INFO("Started a new storage service");
                    this->file_service_registry.insert(new_service_shared_ptr->getName(), fs);
                } else {
                    throw std::runtime_error("SimulationController::startNewService(): Unknown service type");
                }
                s_created.push(std::pair(true, ""));
            } catch (ExecutionException &e) {
                s_created.push(std::pair(false, e.getCause()->toString()));
            }
        });

        // Poll from the shared queue (will be a single one!)
        std::pair<bool, std::string> reply;
        s_created.waitAndPop(reply);
        bool success = std::get<0>(reply);
        if (not success) {
            std::string error_msg = std::get<1>(reply);
            throw std::runtime_error("Cannot start Service: " + error_msg);
        } else {
            json answer;
            answer["service_name"] = s->getName();
            return answer;
        }
    }

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
                std::function<void()> thing_to_do;

                if (this->things_to_do.tryPop(thing_to_do)) {
                    thing_to_do();
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
        std::string property_list_string = data["property_list"];
        std::string message_payload_list_string = data["message_payload_list"];

        PARSE_SERVICE_PROPERTY_LIST()

        PARSE_MESSAGE_PAYLOAD_LIST()

        map<std::string, std::tuple<unsigned long, double>> resources;
        json jsonData = json::parse(resource);
        for (auto it = jsonData.cbegin(); it != jsonData.cend(); ++it) {
            resources.emplace(it.key(), it.value());
        }

        // Create the new service
        auto new_service = new BareMetalComputeService(head_host, resources, scratch_space,
                                                       service_property_list, service_message_payload_list);

        return this->startService<wrench::ComputeService>(new_service);
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

        PARSE_SERVICE_PROPERTY_LIST()

        PARSE_MESSAGE_PAYLOAD_LIST()

        // Create the new service
        auto new_service = new CloudComputeService(hostname, resources, scratch_space,
                                                   service_property_list, service_message_payload_list);
        return this->startService<wrench::ComputeService>(new_service);
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

        PARSE_SERVICE_PROPERTY_LIST()

        PARSE_MESSAGE_PAYLOAD_LIST()

        // Create the new service
        auto new_service = new BatchComputeService(hostname, resources, scratch_space,
                                                   service_property_list, service_message_payload_list);
        return this->startService<wrench::ComputeService>(new_service);
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

        PARSE_SERVICE_PROPERTY_LIST()

        PARSE_MESSAGE_PAYLOAD_LIST()

        // Lookup the cloud compute service
        std::shared_ptr<ComputeService> cs;
        if (not this->compute_service_registry.lookup(cs_name, cs)) {
            throw std::runtime_error("Unknown compute service " + cs_name);
        }

        BlockingQueue<std::pair<bool, std::string>> vm_created;

        // Push the request into the blocking queue (will be a single one!)
        this->things_to_do.push([num_cores, ram_memory, service_property_list, service_message_payload_list, cs, &vm_created]() {
            auto cloud_cs = std::dynamic_pointer_cast<CloudComputeService>(cs);
            std::string vm_name;
            try {
                vm_name = cloud_cs->createVM(num_cores, ram_memory, service_property_list, service_message_payload_list);
                vm_created.push(std::pair(true, vm_name));
            } catch (ExecutionException &e) {
                vm_created.push(std::pair(false, e.getCause()->toString()));
            }
        });

        // Poll from the shared queue (will be a single one!)
        std::pair<bool, std::string> reply;
        vm_created.waitAndPop(reply);
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

        BlockingQueue<std::pair<bool, std::string>> vm_started;
        // Push the request into the blocking queue (will be a single one!)
        this->things_to_do.push([this, vm_name, cs, &vm_started]() {
            auto cloud_cs = std::dynamic_pointer_cast<CloudComputeService>(cs);
            try {
                if (not cloud_cs->isVMDown(vm_name)) {
                    throw std::invalid_argument("Cannot start VM because it's not down");
                }
                auto bm_cs = cloud_cs->startVM(vm_name);
                this->compute_service_registry.insert(bm_cs->getName(), bm_cs);
                vm_started.push(std::pair(true, bm_cs->getName()));
            } catch (ExecutionException &e) {
                vm_started.push(std::pair(false, e.getCause()->toString()));
            } catch (std::invalid_argument &e) {
                vm_started.push(std::pair(false, e.what()));
            }
        });

        // Poll from the shared queue (will be a single one!)
        std::pair<bool, std::string> reply;
        vm_started.waitAndPop(reply);
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

        BlockingQueue<std::pair<bool, std::string>> vm_shutdown;

        // Push the request into the blocking queue (will be a single one!)
        //this->vm_to_shutdown.push(std::pair(vm_name, cs));
        this->things_to_do.push([this, vm_name, cs, &vm_shutdown]() {
            auto cloud_cs = std::dynamic_pointer_cast<CloudComputeService>(cs);
            try {
                if (not cloud_cs->isVMRunning(vm_name)) {
                    throw std::invalid_argument("Cannot shutdown VM because it's not running");
                }
                auto bm_cs = cloud_cs->getVMComputeService(vm_name);

                this->compute_service_registry.remove(bm_cs->getName());
                cloud_cs->shutdownVM(vm_name);
                vm_shutdown.push(std::pair(true, vm_name));
            } catch (ExecutionException &e) {
                vm_shutdown.push(std::pair(false, e.what()));
            } catch (std::invalid_argument &e) {
                vm_shutdown.push(std::pair(false, e.what()));
            }
        });

        // Poll from the shared queue (will be a single one!)
        std::pair<bool, std::string> reply;
        vm_shutdown.waitAndPop(reply);
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

        BlockingQueue<std::pair<bool, std::string>> vm_destroyed;

        // Push the request into the blocking queue (will be a single one!)
        this->things_to_do.push([vm_name, cs, &vm_destroyed]() {
            auto cloud_cs = std::dynamic_pointer_cast<CloudComputeService>(cs);
            try {
                if (not cloud_cs->isVMDown(vm_name)) {
                    throw std::invalid_argument("Cannot destroy VM because it's not down");
                }
                cloud_cs->destroyVM(vm_name);
                vm_destroyed.push(std::pair(true, vm_name));
            } catch (std::invalid_argument &e) {
                vm_destroyed.push(std::pair(false, e.what()));
            }
        });

        // Poll from the shared queue (will be a single one!)
        std::pair<bool, std::string> reply;
        vm_destroyed.waitAndPop(reply);
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
        return this->startService<wrench::StorageService>(new_service);
    }

    /**
     * REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::createFileCopyAtStorageService(json data) {
        std::cerr << "DATA " << data << "\n";
        std::string ss_name = data["storage_service_name"];
        std::string filename = data["filename"];

        std::shared_ptr<StorageService> ss;
        if (not this->storage_service_registry.lookup(ss_name, ss)) {
            throw std::runtime_error("Unknown storage service " + ss_name);
        }

        std::shared_ptr<DataFile> file;
        //        std::string workflow_name = data["workflow_name"];
        //        std::shared_ptr<Workflow> workflow;
        //        if (not this-> workflow_registry.lookup(workflow_name, workflow)) {
        //            throw std::runtime_error("Unknown workflow " + workflow_name);
        //        }
        try {
            file = Simulation::getFileByID(filename);
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
        //        std::string workflow_name = data["workflow_name"];
        //        std::shared_ptr<Workflow> workflow;
        //
        //        if (not this->workflow_registry.lookup(workflow_name, workflow)) {
        //            throw std::runtime_error("Unknown workflow " + workflow_name);
        //        }
        try {
            file = Simulation::getFileByID(filename);
        } catch (std::invalid_argument &e) {
            throw std::runtime_error("Unknown file " + filename);
        }

        BlockingQueue<std::tuple<bool, bool, std::string>> file_looked_up;

        // Push the request into the blocking queue (will be a single one!)
        this->things_to_do.push([file, ss, &file_looked_up]() {
            try {
                bool result = ss->lookupFile(file);
                file_looked_up.push(std::tuple(true, result, ""));
            } catch (std::invalid_argument &e) {
                file_looked_up.push(std::tuple(false, false, e.what()));
            }
        });

        // Poll from the shared queue (will be a single one!)
        std::tuple<bool, bool, std::string> reply;
        file_looked_up.waitAndPop(reply);
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

        return this->startService<wrench::FileRegistryService>(new_service);
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::createStandardJob(json data) {

        std::vector<std::shared_ptr<WorkflowTask>> tasks;
        std::string workflow_name = data["workflow_name"];
        std::shared_ptr<Workflow> workflow;

        if (not this->workflow_registry.lookup(workflow_name, workflow)) {
            throw std::runtime_error("Unknown workflow " + workflow_name);
        }
        for (auto const &name: data["tasks"]) {
            tasks.push_back(workflow->getTaskByID(name));
        }

        std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> file_locations;
        for (auto it = data["file_locations"].begin(); it != data["file_locations"].end(); ++it) {
            auto file = Simulation::getFileByID(it.key());
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

        BlockingQueue<std::pair<bool, std::string>> job_submitted;
        this->things_to_do.push([this, job, cs, service_specific_args, &job_submitted]() {
            try {
                WRENCH_INFO("Submitting a job...");
                this->job_manager->submitJob(job, cs, service_specific_args);
                job_submitted.push(std::make_pair(true, ""));
            } catch (std::exception &e) {
                job_submitted.push(std::make_pair(false, e.what()));
            }
        });
        // Poll from the shared queue (will be a single one!)
        std::pair<bool, std::string> reply;
        job_submitted.waitAndPop(reply);
        bool success = std::get<0>(reply);
        if (not success) {
            std::string error_msg = std::get<1>(reply);
            throw std::runtime_error("Cannot submit job: " + error_msg);
        } else {
            return {};
        }
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::createTask(json data) {

        std::string workflow_name = data["workflow_name"];
        std::shared_ptr<Workflow> workflow;
        if (not this->workflow_registry.lookup(workflow_name, workflow)) {
            throw std::runtime_error("Unknown workflow  " + workflow_name);
        }
        auto t = workflow->addTask(data["name"],
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
        std::string workflow_name = data["workflow_name"];
        std::shared_ptr<Workflow> workflow;
        json answer;
        if (not this->workflow_registry.lookup(workflow_name, workflow)) {
            throw std::runtime_error("Unknown workflow  " + workflow_name);
        }
        answer["flops"] = workflow->getTaskByID(data["name"])->getFlops();
        return answer;
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::getTaskMinNumCores(json data) {
        std::string workflow_name = data["workflow_name"];
        std::shared_ptr<Workflow> workflow;
        json answer;
        if (not this->workflow_registry.lookup(workflow_name, workflow)) {
            throw std::runtime_error("Unknown workflow  " + workflow_name);
        }
        answer["min_num_cores"] = workflow->getTaskByID(data["name"])->getMinNumCores();
        return answer;
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::getTaskMaxNumCores(json data) {
        std::string workflow_name = data["workflow_name"];
        std::shared_ptr<Workflow> workflow;
        json answer;
        if (not this->workflow_registry.lookup(workflow_name, workflow)) {
            throw std::runtime_error("Unknown workflow " + workflow_name);
        }
        answer["max_num_cores"] = workflow->getTaskByID(data["name"])->getMaxNumCores();
        return answer;
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::getTaskMemory(json data) {
        std::string workflow_name = data["workflow_name"];
        std::shared_ptr<Workflow> workflow;
        json answer;
        if (not this->workflow_registry.lookup(workflow_name, workflow)) {
            throw std::runtime_error("Unknown workflow " + workflow_name);
        }
        answer["memory"] = workflow->getTaskByID(data["name"])->getMemoryRequirement();
        return answer;
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output

     */
    json SimulationController::getTaskStartDate(json data) {
        std::string workflow_name = data["workflow_name"];
        std::shared_ptr<Workflow> workflow;
        json answer;
        if (not this->workflow_registry.lookup(workflow_name, workflow)) {
            throw std::runtime_error("Unknown workflow " + workflow_name);
        }
        answer["time"] = workflow->getTaskByID(data["name"])->getStartDate();
        return answer;
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::getTaskEndDate(json data) {
        std::string workflow_name = data["workflow_name"];
        std::shared_ptr<Workflow> workflow;
        json answer;
        if (not this->workflow_registry.lookup(workflow_name, workflow)) {
            throw std::runtime_error("Unknown workflow " + workflow_name);
        }
        answer["time"] = workflow->getTaskByID(data["name"])->getEndDate();
        return answer;
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::addFile(json data) {
        //        std::string workflow_name = data["workflow_name"];
        //        std::shared_ptr<Workflow> workflow;
        //        if (not this->workflow_registry.lookup(workflow_name, workflow)) {
        //            throw std::runtime_error("Unknown workflow  " + workflow_name);
        //        }
        auto file = Simulation::addFile(data["name"], data["size"]);
        return {};
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::getFileSize(json data) {
        //        std::string workflow_name = data["workflow_name"];
        //        std::shared_ptr<Workflow> workflow;
        //        if (not this->workflow_registry.lookup(workflow_name, workflow)) {
        //            throw std::runtime_error("Unknown workflow " + workflow_name);
        //        }
        auto file = Simulation::getFileByID(data["file_id"]);
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
        std::string workflow_name = data["workflow_name"];
        std::shared_ptr<Workflow> workflow;
        if (not this->workflow_registry.lookup(workflow_name, workflow)) {
            throw std::runtime_error("Unknown workflow " + workflow_name);
        }
        auto task = workflow->getTaskByID(data["tid"]);
        auto file = Simulation::getFileByID(data["file"]);
        task->addInputFile(file);
        return {};
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::addOutputFile(json data) {
        std::string workflow_name = data["workflow_name"];
        std::shared_ptr<Workflow> workflow;
        if (not this->workflow_registry.lookup(workflow_name, workflow)) {
            throw std::runtime_error("Unknown workflow " + workflow_name);
        }
        auto task = workflow->getTaskByID(data["tid"]);
        auto file = Simulation::getFileByID(data["file"]);
        task->addOutputFile(file);
        return {};
    }

    /**
     * @brief REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::getTaskInputFiles(json data) {
        std::string workflow_name = data["workflow_name"];
        std::shared_ptr<Workflow> workflow;
        if (not this->workflow_registry.lookup(workflow_name, workflow)) {
            throw std::runtime_error("Unknown workflow " + workflow_name);
        }
        auto task = workflow->getTaskByID(data["tid"]);
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
        std::string workflow_name = data["workflow_name"];
        std::shared_ptr<Workflow> workflow;
        if (not this->workflow_registry.lookup(workflow_name, workflow)) {
            throw std::runtime_error("Unknown workflow " + workflow_name);
        }
        auto task = workflow->getTaskByID(data["tid"]);
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
        std::string workflow_name = data["workflow_name"];
        std::shared_ptr<Workflow> workflow;
        if (not this->workflow_registry.lookup(workflow_name, workflow)) {
            throw std::runtime_error("Unknown workflow " + workflow_name);
        }
        auto files = workflow->getInputFiles();
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
        std::string workflow_name = data["workflow_name"];
        std::shared_ptr<Workflow> workflow;
        if (not this->storage_service_registry.lookup(service_name, storage_service)) {
            throw std::runtime_error("Unknown storage service " + service_name);
        }
        if (not this->workflow_registry.lookup(workflow_name, workflow)) {
            throw std::runtime_error("Unknown workflow " + workflow_name);
        }

        for (auto const &f: workflow->getInputFiles()) {
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
        //this->vm_to_suspend.push(std::pair(vm_name, cs));
        BlockingQueue<std::pair<bool, std::string>> vm_suspended;
        this->things_to_do.push([vm_name, cs, &vm_suspended]() {
            auto cloud_cs = std::dynamic_pointer_cast<CloudComputeService>(cs);
            try {
                cloud_cs->suspendVM(vm_name);
                vm_suspended.push(std::pair(true, vm_name));
            } catch (std::invalid_argument &e) {
                vm_suspended.push(std::pair(false, e.what()));
            }
        });

        // Poll from the shared queue (will be a single one!)
        std::pair<bool, std::string> reply;
        vm_suspended.waitAndPop(reply);
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

        BlockingQueue<std::pair<bool, std::string>> vm_resumed;

        // Push the request into the blocking queue (will be a single one!)
        this->things_to_do.push([vm_name, cs, &vm_resumed]() {
            auto cloud_cs = std::dynamic_pointer_cast<CloudComputeService>(cs);
            try {
                cloud_cs->resumeVM(vm_name);
                vm_resumed.push(std::pair(true, vm_name));
            } catch (std::invalid_argument &e) {
                vm_resumed.push(std::pair(false, e.what()));
            }
        });

        // Poll from the shared queue (will be a single one!)
        std::pair<bool, std::string> reply;
        vm_resumed.waitAndPop(reply);
        bool success = std::get<0>(reply);
        if (not success) {
            std::string error_msg = std::get<1>(reply);
            throw std::runtime_error("Cannot resume VM: " + error_msg);
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
        std::vector<std::string> execution_hosts_list = cloud_cs->getHosts();
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

    /**
     * REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::createWorkflow(json data) {

        auto wf = wrench::Workflow::createWorkflow();
        json answer;
        answer["workflow_name"] = wf->getName();
        this->workflow_registry.insert(wf->getName(), wf);
        return answer;
    }

    /**
     * REST API Handler
     * @param data JSON input
     * @return JSON output
     */
    json SimulationController::createWorkflowFromJSON(json data) {
        std::string json_string = data["json_string"];
        std::string reference_flop_rate = data["reference_flop_rate"];
        bool ignore_machine_specs = data["ignore_machine_specs"];
        bool redundant_dependencies = data["redundant_dependencies"];
        bool ignore_cycle_creating_dependencies = data["ignore_cycle_creating_dependencies"];
        unsigned long min_cores_per_task = data["min_cores_per_task"];
        unsigned long max_cores_per_task = data["max_cores_per_task"];
        bool enforce_num_cores = data["enforce_num_cores"];
        bool ignore_avg_cpu = data["ignore_avg_cpu"];
        bool show_warnings = data["show_warnings"];

        json answer;
        try {
            auto wf = WfCommonsWorkflowParser::createWorkflowFromJSONString(json_string, reference_flop_rate, ignore_machine_specs,
                                                                            redundant_dependencies, ignore_cycle_creating_dependencies,
                                                                            min_cores_per_task, max_cores_per_task, enforce_num_cores,
                                                                            ignore_avg_cpu, show_warnings);
            answer["workflow_name"] = wf->getName();
            std::vector<std::string> task_names;
            for (const auto &t: wf->getTasks()) {
                task_names.push_back(t->getID());
            }
            answer["tasks"] = task_names;
            this->workflow_registry.insert(wf->getName(), wf);
            return answer;
        } catch (std::exception &e) {
            throw std::runtime_error("Error while importing workflow from JSON: " + std::string(e.what()));
        }
    }

}// namespace wrench
