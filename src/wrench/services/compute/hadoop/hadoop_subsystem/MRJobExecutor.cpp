/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/compute/hadoop/MRJob.h>
#include <wrench/services/compute/hadoop/hadoop_subsystem/MapperService.h>
#include <wrench/services/compute/hadoop/hadoop_subsystem/ReducerService.h>
#include <wrench/services/compute/hadoop/hadoop_subsystem/ShuffleService.h>
#include "wrench/services/compute/hadoop/HadoopComputeService.h"
#include "../HadoopComputeServiceMessage.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "wrench/services/compute/ComputeService.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"

WRENCH_LOG_CATEGORY(mr_job_executor_service, "Log category for MR Job Executor Service");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host on which the service should be started
     * @param job: the job to execute
     * @param compute_resources: a set of hostnames
     * @param notify_mailbox: mailbox to which a "job done" or "job failed" message should be sent
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     */
    MRJobExecutor::MRJobExecutor(
            const std::string &hostname,
            MRJob *job,
            const std::set<std::string> compute_resources,
            std::string notify_mailbox,
            std::map<std::string, std::string> property_list,
            std::map<std::string, double> messagepayload_list
    ) :
            Service(hostname,
                    "mr_job_executor",
                    "mr_job_executor"), job(job), success(false) {
        this->compute_resources = compute_resources;
        this->notify_mailbox = std::move(notify_mailbox);

        // Set default and specified properties
        this->setProperties(this->default_property_values, std::move(property_list));

        // Set default and specified message payloads
        this->setMessagePayloads(this->default_messagepayload_values, std::move(messagepayload_list));

    }

    /**
     * @brief Stop the compute service - must be called by the stop()
     *        method of derived classes
     */
    void MRJobExecutor::stop() {
        Service::stop();
    }

    void MRJobExecutor::setup_workers(std::vector<std::shared_ptr<Service>> &mappers,
                                      std::vector<std::shared_ptr<Service>> &reducers,
                                      std::shared_ptr<Service> &hdfs,
                                      std::shared_ptr<Service> &shuffle) {
        // TODO: Right now these are all running on a single host
        // But we should change this so that workers are spun up
        // on the appropriate nodes.  As well as think about what
        // to do when resources do not match user specs.
        hdfs = std::shared_ptr<HdfsService>(
                new HdfsService(
                        this->hostname,
                        this->job,
                        this->compute_resources,
                        {},
                        {}));
        hdfs->simulation = this->simulation;

        shuffle = std::shared_ptr<ShuffleService>(
                new ShuffleService(
                        this->hostname,
                        this->job,
                        this->compute_resources,
                        {},
                        {}));
        shuffle->simulation = this->simulation;
        this->job->setShuffleMailbox(shuffle->mailbox_name);

        for (int i = 0; i < this->job->getNumMappers(); i++) {
            mappers.push_back(std::shared_ptr<MapperService>(
                    new MapperService(
                            this->hostname,
                            this->job,
                            this->compute_resources,
                            {},
                            {})));
            mappers.back()->simulation = this->simulation;
        }

        for (int i = 0; i < this->job->getNumReducers(); i++) {
            reducers.push_back(std::shared_ptr<ReducerService>(
                    new ReducerService(
                            this->hostname,
                            this->job,
                            this->compute_resources,
                            {},
                            {})));
            reducers.back()->simulation = this->simulation;
        }
    }

    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int MRJobExecutor::main() {
        this->state = Service::UP;
        this->job->setExecutorMailbox(this->mailbox_name);

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_CYAN);WRENCH_INFO(
                "New MRJobExecutor starting (%s) on %ld hosts",
                this->mailbox_name.c_str(), this->compute_resources.size());

        // TODO:  This is updated when the reducer(s) send a success message.
        this->success = false;

        setup_workers(this->mappers, this->reducers, this->hdfs, this->shuffle);

        // Start the HDFS and Shuffle services first
        WRENCH_INFO("MRJobExecutor::main(): Starting up the MRJob's Services.")
        hdfs->start(hdfs, true, false);
        shuffle->start(shuffle, true, false);

        // Now start up our mappers and reducers.
        for (auto mapper : mappers) {
            mapper->start(mapper, true, false);
        }
        for (auto reducer : reducers) {
            reducer->start(reducer, true, false);
        }

        /** Main loop **/
        while (this->processNextMessage()) {
        }

        // Reply to my creator with job status
        try {
            S4U_Mailbox::putMessage(this->notify_mailbox,
                                    new MRJobExecutorNotificationMessage(this->success, this->job,
                                                                         this->getMessagePayloadValue(
                                                                                 MRJobExecutorMessagePayload::NOTIFY_EXECUTOR_STATUS_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) { WRENCH_INFO(
                    "Failed to notify the MRJobExecutor's creator...");
            // TODO: deal with this failure mode.
        }

        WRENCH_INFO("MRJobExecutor on host %s terminating cleanly!", S4U_Simulation::getHostName().c_str());
        return 0;
    }


    /**
     * @brief Wait for and react to any incoming message
     *
     * @return false if the daemon should terminate, true otherwise
     *
     * @throw std::runtime_error
     */
    bool MRJobExecutor::processNextMessage() {
        S4U_Simulation::computeZeroFlop();

        // Wait for a message
        std::shared_ptr<SimulationMessage> message;
        try {
            message = S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<NetworkError> &error) { WRENCH_INFO(
                    "Got a network error while getting some message... ignoring");
            return true;
        }

        WRENCH_INFO("Got a [%s] message", message->getName().c_str());
        if (auto msg = std::dynamic_pointer_cast<ServiceStopDaemonMessage>(message)) {
            // TODO: Kill all the subordinate services (mappers, reducers, whatever)
            // TODO: Or reject the stop request because a job  is running?
            // TODO: Or terminate brutally a running job?

            // This is a total lie of a statement.
            this->success = true;

            // This is Synchronous
            try {
                S4U_Mailbox::putMessage(msg->ack_mailbox,
                                        new ServiceDaemonStoppedMessage(this->getMessagePayloadValue(
                                                HadoopComputeServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD)));
            } catch (std::shared_ptr<NetworkError> &cause) {
                return false;
            }
            return false;

        } else {
            throw std::runtime_error(
                    "MRJobExecutor::processNextMessage(): Received an unexpected [" + message->getName() +
                    "] message!");
        }
    }
};
