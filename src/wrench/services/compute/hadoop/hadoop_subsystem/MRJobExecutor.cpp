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

WRENCH_LOG_NEW_DEFAULT_CATEGORY(mr_job_executor_servivce, "Log category for MR Job Executor Service");

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
                    "mr_job_executor") {

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

    void MRJobExecutor::setup_workers(std::vector<std::shared_ptr<Service>> workers) {

        // TODO: Right now these are all running on a single host
        // But we should change this so that workers are spun up
        // on the appropriate nodes.  As well as think about what
        // to do when resorues do not match user specs.
        workers.push_back(std::shared_ptr<HdfsService>(
                new HdfsService(this->hostname, this->job, this->compute_resources,
                                {},
                                {})));

        workers.push_back(std::shared_ptr<ShuffleService>(
                new ShuffleService(this->hostname, this->job, this->compute_resources,
                                   {},
                                   {})));

        for (int i = 0; i < this->job->calculateNumMappers(); i++) {
            workers.push_back(std::shared_ptr<MapperService>(
                    new MapperService(this->hostname, this->job, this->compute_resources,
                                      {},
                                      {})));
        }

        for (int i = 0; i < this->job->getNumReducers(); i++) {
            workers.push_back(std::shared_ptr<ReducerService>(
                    new ReducerService(this->hostname, this->job, this->compute_resources,
                                       {},
                                       {})));
        }
    }

    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int MRJobExecutor::main() {
        this->state = Service::UP;

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);

        WRENCH_INFO("New MRJobExecutor starting (%s) on %ld hosts",
                    this->mailbox_name.c_str(), this->compute_resources.size());

        this->success = true;

        std::vector<std::shared_ptr<Service>> workers;
        setup_workers(workers);
        for (auto worker: workers) {
            worker->simulation = this->simulation;
            worker->start(worker, true, false);
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

            // Right now, assume the job fails.
            this->success = false;
            // This is Synchronous
            try {
                S4U_Mailbox::putMessage(msg->ack_mailbox,
                                        new ServiceDaemonStoppedMessage(this->getMessagePayloadValue(
                                                HadoopComputeServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
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
