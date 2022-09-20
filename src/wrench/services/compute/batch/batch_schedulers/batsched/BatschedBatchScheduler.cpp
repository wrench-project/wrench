/**
 * Copyright (c) 2017-2021. The WRENCH Team.
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

#include <wrench/logging/TerminalOutput.h>
#include "wrench/services/compute/batch/batch_schedulers/batsched/BatschedBatchScheduler.h"
#include <wrench/services/compute/batch/BatchComputeService.h>
#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <wrench/simgrid_S4U_util/S4U_Simulation.h>
#include <wrench/exceptions/ExecutionException.h>
#include <wrench/services/compute/batch/BatchComputeServiceMessage.h>
#include <wrench/failure_causes/NetworkError.h>

WRENCH_LOG_CATEGORY(wrench_core_batsched_batch_scheduler, "Log category for BatschedBatchScheduler");


namespace wrench {


    /**
     * @brief Initialization method
     */
    void BatschedBatchScheduler::init() {
#ifdef ENABLE_BATSCHED

        // Launch the Batsched process

        // Determine q "good" port number, if possible
        // This process below is implemented because of some srange
        // "address already in use" errors with ZMQ
        while (1) {
            // The "mod by 1500" below is totally ad-hoc, but not modding seemed
            // to lead to even more weird ZMQ "address already in use" errors...
            this->batsched_port = 28000 + (getpid() % 1500) +
                                  S4U_Mailbox::generateUniqueSequenceNumber();
            std::cerr << "Thinking of starting Batsched on port " << this->batsched_port << "...\n";
            this->pid = getpid();
            zmq::context_t context(1);
            zmq::socket_t socket(context, ZMQ_REQ);
            std::string address = "tcp://*:" + std::to_string(this->batsched_port);
            int rc = zmq_bind(socket, address.c_str());
            if (rc != 0) {
                std::cerr << "Couldn't bind socket to port (errno=" << errno << " - EADDRINUSE = " << EADDRINUSE << ". Retrying...\n";
                continue;
            } else {
                std::cerr << "Was able to bind socket to port, unbinding and proceedings\n";
                zmq_unbind(socket, address.c_str());
                break;
            }
        }

        int top_pid = fork();

        if (top_pid == 0) {// Child process that will exec batsched
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

            std::string rjms_delay = this->cs->getPropertyValueAsString(BatchComputeServiceProperty::BATCH_RJMS_PADDING_DELAY);
            std::string socket_endpoint = "tcp://*:" + std::to_string(this->batsched_port);

            char **args = NULL;
            unsigned int num_args = 0;

            (args = (char **) realloc(args, (num_args + 1) * sizeof(char *)))[num_args] = strdup("batsched");
            num_args++;
            (args = (char **) realloc(args, (num_args + 1) * sizeof(char *)))[num_args] = strdup("-v");
            num_args++;
            (args = (char **) realloc(args, (num_args + 1) * sizeof(char *)))[num_args] = strdup(algorithm.c_str());
            num_args++;
            (args = (char **) realloc(args, (num_args + 1) * sizeof(char *)))[num_args] = strdup("-o");
            num_args++;
            (args = (char **) realloc(args, (num_args + 1) * sizeof(char *)))[num_args] = strdup(queue_ordering.c_str());
            num_args++;
            (args = (char **) realloc(args, (num_args + 1) * sizeof(char *)))[num_args] = strdup("-s");
            num_args++;
            (args = (char **) realloc(args, (num_args + 1) * sizeof(char *)))[num_args] = strdup(socket_endpoint.c_str());
            num_args++;
            (args = (char **) realloc(args, (num_args + 1) * sizeof(char *)))[num_args] = strdup("--rjms_delay");
            num_args++;
            (args = (char **) realloc(args, (num_args + 1) * sizeof(char *)))[num_args] = strdup(rjms_delay.c_str());
            num_args++;

            if (this->cs->getPropertyValueAsBoolean(BatchComputeServiceProperty::BATSCHED_LOGGING_MUTED)) {
                (args = (char **) realloc(args, (num_args + 1) * sizeof(char *)))[num_args] =
                        strdup("--verbosity=silent");
                num_args++;
            }
            if (this->cs->getPropertyValueAsBoolean(BatchComputeServiceProperty::BATSCHED_CONTIGUOUS_ALLOCATION)) {
                (args = (char **) realloc(args, (num_args + 1) * sizeof(char *)))[num_args] =
                        strdup("--policy=contiguous");
                num_args++;
            }
            (args = (char **) realloc(args, (num_args + 1) * sizeof(char *)))[num_args] = nullptr;

            if (execvp(args[0], args) == -1) {
                exit(3);
            }


        } else if (top_pid > 0) {
            // parent process
            sleep(1);// Wait one second to let batsched the time to start
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
                    int tether[2];             // this is a local variable, only defined in this scope
                    if (pipe(tether) != 0) {   // the pipe however is opened during the whole duration of both processes
                        kill(top_pid, SIGKILL);//kill the other child (that has fork-exec'd batsched)
                        throw std::runtime_error("startBatsched(): tether pipe creation failed!");
                    }
                    //now fork a process that sleeps until its parent is dead
                    int nested_pid = fork();

                    if (nested_pid > 0) {
                        //I am the parent, whose child has fork-exec'd batsched
                    } else if (nested_pid == 0) {
                        char foo;
                        close(tether[1]);                              // closing write end
                        auto num_bytes_read = read(tether[0], &foo, 1);// blocking read which returns when the parent dies
                        //check if the child that forked batsched is still running
                        if (getpgid(top_pid)) {
                            kill(top_pid, SIGKILL);//kill the other child that has fork-exec'd batsched
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
                            " not supported by the BatchComputeService service");
                case 2:
                    throw std::invalid_argument(
                            "startBatsched(): Queuing option " +
                            this->cs->getPropertyValueAsString(BatchComputeServiceProperty::BATCH_QUEUE_ORDERING_ALGORITHM) +
                            "not supported by the BatchComputeService service");
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
                    "Error while fork-exec of batsched");
        }

#else
        throw std::runtime_error("BatschedBatchScheduler::init(): BATSCHED_ENABLE should be set to 'on'");
#endif
    }

    /**
     * @brief Method to launch Batsched
     */
    void BatschedBatchScheduler::launch() {
#ifdef ENABLE_BATSCHED
        // Start the Batsched Network Listener
        try {
            this->startBatschedNetworkListener();
        } catch (std::runtime_error &e) {
            throw;
        }
#else
        throw std::runtime_error("BatschedBatchScheduler::launch(): BATSCHED_ENABLE should be set to 'on'");
#endif
    }


    std::map<std::string, double> BatschedBatchScheduler::getStartTimeEstimates(
            std::set<std::tuple<std::string, unsigned long, unsigned long, double>> set_of_jobs) {
#ifdef ENABLE_BATSCHED

        std::set<std::string> supported_algorithms = {"conservative_bf", "fast_conservative_bf", "fcfs_fast"};
        if (supported_algorithms.find(this->cs->getPropertyValueAsString(BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM)) == supported_algorithms.end()) {
            throw std::runtime_error("BatschedBatchScheduler::getStartTimeEstimates(): Algorithm does not support start time estimates");
        }
        nlohmann::json batch_submission_data;
        batch_submission_data["now"] = S4U_Simulation::getClock();

        // IMPORTANT: THIS IGNORES THE NUMBER OF CORES (THIS IS A LIMITATION OF batsched!)

        int idx = 0;
        batch_submission_data["events"] = nlohmann::json::array();
        for (auto job: set_of_jobs) {
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

        simgrid::s4u::Mailbox *batchsched_query_mailbox = S4U_Mailbox::generateUniqueMailbox("batchsched_query_mailbox");

        std::shared_ptr<BatschedNetworkListener> network_listener =
                std::shared_ptr<BatschedNetworkListener>(
                        new BatschedNetworkListener(this->cs->hostname, this->cs->getSharedPtr<BatchComputeService>(), batchsched_query_mailbox,
                                                    std::to_string(this->batsched_port),
                                                    data));
        network_listener->setSimulation(this->cs->getSimulation());
        network_listener->start(network_listener, true, false);// Daemonized, no auto-restart
        network_listener = nullptr;                            // detached mode

        std::map<std::string, double> job_estimated_start_times = {};
        for (auto job: set_of_jobs) {
            // Get the answer
            std::unique_ptr<SimulationMessage> message = nullptr;
            try {
                message = S4U_Mailbox::getMessage(batchsched_query_mailbox);
            } catch (std::shared_ptr<NetworkError> &cause) {
                throw ExecutionException(cause);
            }

            if (auto msg = dynamic_cast<BatchQueryAnswerMessage *>(message.get())) {
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


    /**
     * @brief Method to shutdown Batsched
     */
    void BatschedBatchScheduler::shutdown() {
#ifdef ENABLE_BATSCHED

        // Stop Batsched
        zmq::context_t context(1);
        zmq::socket_t socket(context, ZMQ_REQ);
        socket.connect("tcp://localhost:" + std::to_string(this->batsched_port));

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
                    "BatschedBatchScheduler::shutdown(): Upon termination batsched returned a non-empty event set, which is unexpected");
        }

#else
        throw std::runtime_error("BatschedBatchScheduler::shutdown(): BATSCHED_ENABLE should be set to 'on'");
#endif
    }


    void BatschedBatchScheduler::processQueuedJobs() {
#if ENABLE_BATSCHED
        if (this->cs->batch_queue.empty()) {
            return;
        }

        // IMPORTANT: We always ask for more time, so that when the alarm goes
        // of at the right time, we can respond to it before the Batsched
        // time slice has expired!
        double BATSCHED_JOB_EXTRA_TIME = 1.0;

        // Send ALL Queued jobs to batsched, and move them all to the WAITING queue
        // The WAITING queue is: those jobs that I need to hear from Batsched about

        nlohmann::json batch_submission_data;
        batch_submission_data["now"] = S4U_Simulation::getClock();
        batch_submission_data["events"] = nlohmann::json::array();
        size_t i;
        std::deque<std::shared_ptr<BatchJob>>::iterator it;
        for (i = 0, it = this->cs->batch_queue.begin(); i < this->cs->batch_queue.size(); i++, it++) {
            auto batch_job = *it;

            /* Get the nodes and cores per nodes asked for */
            unsigned long cores_per_node_asked_for = batch_job->getRequestedCoresPerNode();
            unsigned long num_nodes_asked_for = batch_job->getRequestedNumNodes();
            unsigned long allocated_time = batch_job->getRequestedTime();

            batch_submission_data["events"][i]["timestamp"] = batch_job->getArrivalTimestamp();
            batch_submission_data["events"][i]["type"] = "JOB_SUBMITTED";
            batch_submission_data["events"][i]["data"]["job_id"] = std::to_string(batch_job->getJobID());
            batch_submission_data["events"][i]["data"]["job"]["id"] = std::to_string(batch_job->getJobID());
            batch_submission_data["events"][i]["data"]["job"]["res"] = num_nodes_asked_for;
            batch_submission_data["events"][i]["data"]["job"]["core"] = cores_per_node_asked_for;
            batch_submission_data["events"][i]["data"]["job"]["walltime"] = allocated_time + BATSCHED_JOB_EXTRA_TIME;

            this->cs->batch_queue.erase(it);
            this->cs->waiting_jobs.insert(batch_job);
        }
        std::string data = batch_submission_data.dump();
        std::shared_ptr<BatschedNetworkListener> network_listener =
                std::shared_ptr<BatschedNetworkListener>(
                        new BatschedNetworkListener(this->cs->hostname, this->cs->getSharedPtr<BatchComputeService>(), this->cs->mailbox,
                                                    std::to_string(this->batsched_port),
                                                    data));
        network_listener->setSimulation(this->cs->getSimulation());
        network_listener->start(network_listener, true, false);// Daemonized, no auto-restart
        network_listener = nullptr;                            // detached mode
#else
        throw std::runtime_error("BatschedBatchScheduler::processQueuesJobs(): BATSCHED_ENABLE should be set to 'on'");
#endif
    }


    void BatschedBatchScheduler::processJobFailure(std::shared_ptr<BatchJob> batch_job) {
#ifdef ENABLE_BATSCHED
        this->notifyJobEventsToBatSched(std::to_string(batch_job->getJobID()), "TIMEOUT", "COMPLETED_FAILED", "", "JOB_COMPLETED");

        this->appendJobInfoToCSVOutputFile(batch_job.get(), "FAILED");
#else
        throw std::runtime_error("BatschedBatchScheduler::processQueuesJobs(): BATSCHED_ENABLE should be set to 'on'");
#endif
    }


    void BatschedBatchScheduler::processJobCompletion(std::shared_ptr<BatchJob> batch_job) {
#ifdef ENABLE_BATSCHED
        this->notifyJobEventsToBatSched(std::to_string(batch_job->getJobID()), "SUCCESS", "COMPLETED_SUCCESSFULLY", "", "JOB_COMPLETED");
        this->appendJobInfoToCSVOutputFile(batch_job.get(), "success");
#else
        throw std::runtime_error("BatschedBatchScheduler::processQueuesJobs(): BATSCHED_ENABLE should be set to 'on'");
#endif
    }


    void BatschedBatchScheduler::processJobTermination(std::shared_ptr<BatchJob> batch_job) {
#ifdef ENABLE_BATSCHED
        // Fake it as a success
        this->notifyJobEventsToBatSched(std::to_string(batch_job->getJobID()), "SUCCESS", "COMPLETED_KILLED", "terminated by users", "JOB_COMPLETED");
        this->appendJobInfoToCSVOutputFile(batch_job.get(), "TERMINATED");
#else
        throw std::runtime_error("BatschedBatchScheduler::processQueuesJobs(): BATSCHED_ENABLE should be set to 'on'");
#endif
    }


    void BatschedBatchScheduler::processUnknownJobTermination(std::string job_id) {
#ifdef ENABLE_BATSCHED
        // Fake it as a success
        this->notifyJobEventsToBatSched(job_id, "SUCCESS", "COMPLETED_SUCCESSFULLY", "", "JOB_COMPLETED");
//        this->appendJobInfoToCSVOutputFile(job_id, "TERMINATED");
#else
        throw std::runtime_error("BatschedBatchScheduler::processUnknownJobTermination(): BATSCHED_ENABLE should be set to 'on'");
#endif
    }

    void BatschedBatchScheduler::processJobSubmission(std::shared_ptr<BatchJob> batch_job) {
        // Do nothing
    }


    /**
     * HELPER METHODS
     */

#ifdef ENABLE_BATSCHED

    /**
    * @brief Notify a job even to batsched (batsched ONLY)
    * @param job_id the id of the job to be processed
    * @param status the status of the job
    * @param job_state current state of the job
    * @param kill_reason the reason to be killed ("" if not being killed)
    * @param event_type the type of event (JOB_COMPLETED/JOB_KILLED..)
    */
    void BatschedBatchScheduler::notifyJobEventsToBatSched(std::string job_id, std::string status, std::string job_state,
                                                           std::string kill_reason, std::string event_type) {

        nlohmann::json batch_submission_data;
        batch_submission_data["now"] = S4U_Simulation::getClock();
        batch_submission_data["events"][0]["timestamp"] = S4U_Simulation::getClock();
        batch_submission_data["events"][0]["type"] = event_type;
        batch_submission_data["events"][0]["data"]["job_id"] = job_id;
        batch_submission_data["events"][0]["data"]["status"] = status;
        batch_submission_data["events"][0]["data"]["job_state"] = job_state;
        batch_submission_data["events"][0]["data"]["kill_reason"] = kill_reason;

        std::string data = batch_submission_data.dump();
        std::shared_ptr<BatschedNetworkListener> network_listener =
                std::shared_ptr<BatschedNetworkListener>(
                        new BatschedNetworkListener(this->cs->hostname, this->cs->getSharedPtr<BatchComputeService>(),
                                                    this->cs->mailbox,
                                                    std::to_string(this->batsched_port),
                                                    data));
        network_listener->setSimulation(this->cs->getSimulation());
        network_listener->start(network_listener, true, false);// Daemonized, no auto-restart
        network_listener = nullptr;                            // detached mode
    }


    /**
     * @brief Appends a job to the CSV Output file
     *
     * @param batch_job: the BatchComputeService job
     * @param status: "COMPLETED", "TERMINATED" (by user), "FAILED" (timeout)
     */
    void BatschedBatchScheduler::appendJobInfoToCSVOutputFile(BatchJob *batch_job, std::string status) {
        std::string csv_file_path;
        static unsigned long job_id = 0;

        csv_file_path = this->cs->getPropertyValueAsString(BatchComputeServiceProperty::OUTPUT_CSV_JOB_LOG);
        if (csv_file_path.empty()) {
            return;
        }
        std::ofstream file;
        file.open(csv_file_path, std::ios_base::app);
        if (not file) {
            throw std::runtime_error("BatchComputeService::appendJobInfoToCSVOutputFile(): Unable to append to CSV output file " +
                                     csv_file_path + " (as specified by the BatchComputeServiceProperty::OUTPUT_CSV_JOB_LOG property)");
        }
        std::string csv_line = "";

        // Allocated Processors
        csv_line += batch_job->csv_allocated_processors + ",";
        // Consumed Energy
        csv_line += "0,";
        // Execution Time
        double execution_time = batch_job->getEndingTimestamp() - batch_job->getBeginTimestamp();
        csv_line += std::to_string(execution_time) + ",";
        // Finish time
        double finish_time = batch_job->getEndingTimestamp();
        csv_line += std::to_string(finish_time) + ",";
        // Job ID
        csv_line += std::to_string(job_id++) + ",";
        // MetaData
        csv_line += std::string("\"") + batch_job->csv_metadata + "\",";
        // Requested number of processors
        csv_line += std::to_string(batch_job->getRequestedNumNodes()) + ",";
        // Requested time
        csv_line += std::to_string(batch_job->getRequestedTime()) + ",";
        // Starting time
        double starting_time = batch_job->getBeginTimestamp();
        csv_line += std::to_string(starting_time) + ",";
        // Stretch
        double stretch = (batch_job->getEndingTimestamp() - batch_job->getArrivalTimestamp()) /
                         (batch_job->getEndingTimestamp() - batch_job->getBeginTimestamp());
        csv_line += std::to_string(stretch) + ",";
        // Submission time
        double submission_time = batch_job->getArrivalTimestamp();
        csv_line += std::to_string(submission_time) + ",";
        // Success
        unsigned char success = 1;
        if (status == "COMPLETED") {
            success = 1;
        } else if (status == "FAILED") {
            success = 0;
        } else if (status == "TERMINATED") {
            success = 2;
        }
        csv_line += std::to_string(success) + ",";
        // Turnaround time
        double turnaround_time = (batch_job->getEndingTimestamp() - batch_job->getArrivalTimestamp());
        csv_line += std::to_string(turnaround_time) + ",";
        // Waiting time
        double waiting_time = (batch_job->getBeginTimestamp() - batch_job->getArrivalTimestamp());
        csv_line += std::to_string(waiting_time) + ",";
        // Workload name
        csv_line += "wrench";

        file << csv_line << std::endl;
        file.close();
    }


    /**
    * @brief Start a network listener process (for batsched only)
    */
    void BatschedBatchScheduler::startBatschedNetworkListener() {
        nlohmann::json compute_resources_map;
        compute_resources_map["now"] = S4U_Simulation::getClock();
        compute_resources_map["events"][0]["timestamp"] = S4U_Simulation::getClock();
        compute_resources_map["events"][0]["type"] = "SIMULATION_BEGINS";
        compute_resources_map["events"][0]["data"]["nb_resources"] = this->cs->nodes_to_cores_map.size();
        compute_resources_map["events"][0]["data"]["allow_time_sharing"] = false;
        //    This was the "old" batsched up until commit 39a30d83
        //      compute_resources_map["events"][0]["data"]["config"]["redis"]["enabled"] = false;
        compute_resources_map["events"][0]["data"]["config"]["redis-enabled"] = false;
        std::map<std::string, unsigned long>::iterator it;
        int count = 0;
        for (it = this->cs->nodes_to_cores_map.begin(); it != this->cs->nodes_to_cores_map.end(); it++) {
            compute_resources_map["events"][0]["data"]["resources_data"][count]["id"] = std::to_string(count);
            compute_resources_map["events"][0]["data"]["resources_data"][count]["name"] = it->first;
            compute_resources_map["events"][0]["data"]["resources_data"][count]["core"] = it->second;
            compute_resources_map["events"][0]["data"]["resources_data"][count++]["state"] = "idle";
        }
        std::string data = compute_resources_map.dump();


        try {
            std::shared_ptr<BatschedNetworkListener> network_listener =
                    std::shared_ptr<BatschedNetworkListener>(
                            new BatschedNetworkListener(this->cs->hostname, this->cs->getSharedPtr<BatchComputeService>(), this->cs->mailbox,
                                                        std::to_string(this->batsched_port),
                                                        data));
            network_listener->setSimulation(this->cs->getSimulation());
            network_listener->start(network_listener, true, false);// Daemonized, no auto-restart
            network_listener = nullptr;                            // detached mode
        } catch (std::runtime_error &e) {
            throw;
        }
    }
#endif

}// namespace wrench
