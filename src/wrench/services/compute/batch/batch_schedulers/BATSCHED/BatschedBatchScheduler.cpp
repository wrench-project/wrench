/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifdef ENABLE_BATSCHED

#include <signal.h>
#include <zmq.hpp>
#include <zmq.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <iostream>
#include <fstream>

#include <nlohmann/json.hpp>

#endif


#include "BatschedBatchScheduler.h"
#include "wrench/services/compute/batch/BatchComputeService.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "wrench/exceptions/WorkflowExecutionException.h"
#include "wrench/services/compute/batch/BatchComputeServiceMessage.h"


namespace wrench {


    void BatschedBatchScheduler::init() {
#ifdef ENABLE_BATSCHED

        // Launch the Batsched process

        // The "mod by 1500" below is totally ad-hoc, but not modding seemed
        // to lead to weird zmq "address already in use" errors...
        this->cs->batsched_port = 28000 + (getpid() % 1500) +
                              S4U_Mailbox::generateUniqueSequenceNumber();
        this->cs->pid = getpid();

        int top_pid = fork();

        if (top_pid == 0) { // Child process that will exec batsched

            std::string algorithm = this->cs->getPropertyValueAsString(BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM);
            bool is_supported = this->cs->scheduling_algorithms.find(algorithm) != this->cs->scheduling_algorithms.end();
            if (not is_supported) {
                exit(1);
            }

            std::string queue_ordering = this->cs->getPropertyValueAsString(
                    BatchComputeServiceProperty::BATCH_QUEUE_ORDERING_ALGORITHM);
            bool is_queue_ordering_available =
                    this->cs->queue_ordering_options.find(queue_ordering) != this->cs->queue_ordering_options.end();
            if (not is_queue_ordering_available) {
                exit(2);
            }

            std::string rjms_delay = this->cs->getPropertyValueAsString(BatchComputeServiceProperty::BATCH_RJMS_DELAY);
            std::string socket_endpoint = "tcp://*:" + std::to_string(this->cs->batsched_port);

            char **args = NULL;
            unsigned int num_args = 0;

            (args = (char **)realloc(args, (num_args+1)*sizeof(char*)))[num_args] = strdup("batsched"); num_args++;
            (args = (char **)realloc(args, (num_args+1)*sizeof(char*)))[num_args] = strdup("-v"); num_args++;
            (args = (char **)realloc(args, (num_args+1)*sizeof(char*)))[num_args] = strdup(algorithm.c_str()); num_args++;
            (args = (char **)realloc(args, (num_args+1)*sizeof(char*)))[num_args] = strdup("-o"); num_args++;
            (args = (char **)realloc(args, (num_args+1)*sizeof(char*)))[num_args] = strdup(queue_ordering.c_str()); num_args++;
            (args = (char **)realloc(args, (num_args+1)*sizeof(char*)))[num_args] = strdup("-s"); num_args++;
            (args = (char **)realloc(args, (num_args+1)*sizeof(char*)))[num_args] = strdup(socket_endpoint.c_str()); num_args++;
            (args = (char **)realloc(args, (num_args+1)*sizeof(char*)))[num_args] = strdup("--rjms_delay"); num_args++;
            (args = (char **)realloc(args, (num_args+1)*sizeof(char*)))[num_args] = strdup(rjms_delay.c_str()); num_args++;

            if (this->cs->getPropertyValueAsBoolean(BatchComputeServiceProperty::BATSCHED_LOGGING_MUTED)) {
                (args = (char **) realloc(args, (num_args + 1) * sizeof(char *)))[num_args] =
                        strdup("--verbosity=silent"); num_args++;
            }
            if (this->cs->getPropertyValueAsBoolean(BatchComputeServiceProperty::BATSCHED_CONTIGUOUS_ALLOCATION)) {
                (args = (char **) realloc(args, (num_args + 1) * sizeof(char *)))[num_args] =
                        strdup("--policy=contiguous"); num_args++;
            }
            (args = (char **)realloc(args, (num_args+1)*sizeof(char*)))[num_args] = nullptr;

            if (execvp(args[0], args) == -1) {
                exit(3);
            }


        } else if (top_pid > 0) {
            // parent process
            sleep(1); // Wait one second to let batsched the time to start
            // (this is pretty ugly)
            int exit_code = 0;
            int status = waitpid(top_pid, &exit_code, WNOHANG);
            if (status == 0) {
                exit_code = 0;
            } else {
                exit_code = WIFEXITED(exit_code);
            }
            switch (exit_code) {
                case 0: {
                    // Establish a tether so that if the main process dies, then batsched is brutally killed
                    int tether[2]; // this is a local variable, only defined in this scope
                    if (pipe(tether) != 0) {  // the pipe however is opened during the whole duration of both processes
                        kill(top_pid, SIGKILL); //kill the other child (that has fork-exec'd batsched)
                        throw std::runtime_error("startBatsched(): tether pipe creation failed!");
                    }
                    //now fork a process that sleeps until its parent is dead
                    int nested_pid = fork();

                    if (nested_pid > 0) {
                        //I am the parent, whose child has fork-exec'd batsched
                    } else if (nested_pid == 0) {
                        char foo;
                        close(tether[1]); // closing write end
                        read(tether[0], &foo, 1); // blocking read which returns when the parent dies
                        //check if the child that forked batsched is still running
                        if (getpgid(top_pid)) {
                            kill(top_pid, SIGKILL); //kill the other child that has fork-exec'd batsched
                        }
                        //my parent has died so, I will kill myself instead of exiting and becoming a zombie
                        kill(getpid(), SIGKILL);
                        //exit(is_sent); //if exit myself and become a zombie :D

                    }
                    break;
                }
                case 1:
                    throw std::invalid_argument(
                            "startBatsched(): Scheduling algorithm " +
                            this->cs->getPropertyValueAsString(BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM) +
                            " not supported by the batch service");
                case 2:
                    throw std::invalid_argument(
                            "startBatsched(): Queuing option " +
                            this->cs->getPropertyValueAsString(BatchComputeServiceProperty::BATCH_QUEUE_ORDERING_ALGORITHM) +
                            "not supported by the batch service");
                case 3:
                    throw std::runtime_error(
                            "startBatsched(): Cannot start the batsched process");
                default:
                    throw std::runtime_error(
                            "startBatsched(): Unknown fatal error");
            }

            // Create the output CSV file if needed
            std::string output_csv_file = this->cs->getPropertyValueAsString(BatchComputeServiceProperty::OUTPUT_CSV_JOB_LOG);
            if (not output_csv_file.empty()) {
                std::ofstream file;
                file.open(output_csv_file, std::ios_base::out);
                if (not file) {
                    throw std::invalid_argument("BatchComputeService(): Unable to create CSV output file " +
                                                output_csv_file + " (as specified by the BatchComputeServiceProperty::OUTPUT_CSV_JOB_LOG property)");
                }
            }

        } else {
            // fork failed
            throw std::runtime_error(
                    "Error while fork-exec of batsched"
            );
        }

#else
        throw std::runtime_error("BatschedBatchScheduler::init(): BATSCHED_ENABLE should be set to 'on'");
#endif
    }

    void BatschedBatchScheduler::launch() {
#ifdef ENABLE_BATSCHED
        // Start the Batsched Network Listener
        try {
            this->cs->startBatschedNetworkListener();
        } catch (std::runtime_error &e) {
            throw;
        }
#else
        throw std::runtime_error("BatschedBatchScheduler::launch(): BATSCHED_ENABLE should be set to 'on'");
#endif
    }


    std::map<std::string, double> BatschedBatchScheduler::getStartTimeEstimates(
            std::set<std::tuple<std::string, unsigned int, unsigned int, double>> set_of_jobs) {

#ifdef ENABLE_BATSCHED

        std::set<std::string> supported_algorithms = {"conservative_bf", "fast_conservative_bf", "fcfs_fast"};
        if (supported_algorithms.find(this->cs->getPropertyValueAsString(BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM)) == supported_algorithms.end()) {
            throw std::runtime_error("BatschedBatchScheduler::getStartTimeEstimates(): Algorithm does not support start time estimates");
        }
        nlohmann::json batch_submission_data;
        batch_submission_data["now"] = S4U_Simulation::getClock();

        // IMPORTANT: THIS IGNORES THE NUMBER OF CORES (THIS IS A LIMITATION OF BATSCHED!)

        int idx = 0;
        batch_submission_data["events"] = nlohmann::json::array();
        for (auto job : set_of_jobs) {
            batch_submission_data["events"][idx]["timestamp"] = S4U_Simulation::getClock();
            batch_submission_data["events"][idx]["type"] = "QUERY";
            batch_submission_data["events"][idx]["data"]["requests"]["estimate_waiting_time"]["job_id"] = std::get<0>(job);
            batch_submission_data["events"][idx]["data"]["requests"]["estimate_waiting_time"]["job"]["id"] = std::get<0>(
                    job);
            batch_submission_data["events"][idx]["data"]["requests"]["estimate_waiting_time"]["job"]["res"] = std::get<1>(
                    job);
            batch_submission_data["events"][idx++]["data"]["requests"]["estimate_waiting_time"]["job"]["walltime"] = std::get<3>(
                    job);
        }

        std::string data = batch_submission_data.dump();

        std::string batchsched_query_mailbox = S4U_Mailbox::generateUniqueMailboxName("batchsched_query_mailbox");

        std::shared_ptr<BatschedNetworkListener> network_listener =
                std::shared_ptr<BatschedNetworkListener>(
                        new BatschedNetworkListener(this->cs->hostname, this->cs->getSharedPtr<BatchComputeService>(), batchsched_query_mailbox,
                                                    std::to_string(this->cs->batsched_port),
                                                    data));
        network_listener->simulation = this->cs->simulation;
        network_listener->start(network_listener, true, false); // Daemonized, no auto-restart
        network_listener = nullptr; // detached mode
//      this->cs->network_listeners.insert(std::move(network_listener));


        std::map<std::string, double> job_estimated_start_times = {};
        for (auto job : set_of_jobs) {
            // Get the answer
            std::shared_ptr<SimulationMessage> message = nullptr;
            try {
                message = S4U_Mailbox::getMessage(batchsched_query_mailbox);
            } catch (std::shared_ptr<NetworkError> &cause) {
                throw WorkflowExecutionException(cause);
            }

            if (auto msg = std::dynamic_pointer_cast<BatchQueryAnswerMessage>(message)) {
                job_estimated_start_times[std::get<0>(job)] = msg->estimated_start_time;
            } else {
                throw std::runtime_error(
                        "BatschedBatchScheduler::getStartTimeEstimates: Received an unexpected [" + message->getName() +
                        "] message.\nThis likely means that the scheduling algorithm that Batsched was configured to use (" +
                        this->cs->getPropertyValueAsString(BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM) +
                        ") does not support queue waiting time predictions!");
            }
        }
        return job_estimated_start_times;
#else
        throw std::runtime_error("BatschedBatchScheduler::init(): BATSCHED_ENABLE should be set to 'on'");
#endif
    }


    // TODO
    BatchJob *BatschedBatchScheduler::pickNextJobToSchedule() {
        return nullptr;
    }

    // TODO
    std::map<std::string, std::tuple<unsigned long, double>>
    BatschedBatchScheduler::scheduleOnHosts(unsigned long, unsigned long, double) {
        return std::map<std::string, std::tuple<unsigned long, double>>();
    }

    void BatschedBatchScheduler::shutdown() {

#ifdef ENABLE_BATSCHED

        // Stop Batsched
        zmq::context_t context(1);
        zmq::socket_t socket(context, ZMQ_REQ);
        socket.connect("tcp://localhost:" + std::to_string(this->cs->batsched_port));

        nlohmann::json simulation_ends_msg;
        simulation_ends_msg["now"] = S4U_Simulation::getClock();
        simulation_ends_msg["events"][0]["timestamp"] = S4U_Simulation::getClock();
        simulation_ends_msg["events"][0]["type"] = "SIMULATION_ENDS";
        simulation_ends_msg["events"][0]["data"] = {};
        std::string data_to_send = simulation_ends_msg.dump();

        zmq::message_t request(strlen(data_to_send.c_str()));
        memcpy(request.data(), data_to_send.c_str(), strlen(data_to_send.c_str()));
        socket.send(request);

        //  Get the reply.
        zmq::message_t reply;
        socket.recv(&reply);

        // Process the reply
        std::string reply_data;
        reply_data = std::string(static_cast<char *>(reply.data()), reply.size());

        nlohmann::json reply_decisions;
        nlohmann::json decision_events;
        reply_decisions = nlohmann::json::parse(reply_data);
        decision_events = reply_decisions["events"];
        if (decision_events.size() > 0) {
            throw std::runtime_error(
                    "BatschedBatchScheduler::shutdown(): Upon termination BATSCHED returned a non-empty event set, which is unexpected");
        }

#else
        throw std::runtime_error("BatschedBatchScheduler::shutdown(): BATSCHED_ENABLE should be set to 'on'");
#endif

    }



}


