/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/compute/hadoop/hadoop_subsystem/HdfsService.h"
#include "wrench/services/compute/hadoop/HadoopComputeService.h"
#include "wrench/services/ServiceMessage.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/workflow/failure_causes/NetworkError.h"
#include "../HadoopComputeServiceMessage.h"
#include "wrench/simulation/Simulation.h"

WRENCH_LOG_NEW_DEFAULT_CATEGORY(hdfs_servivce, "Log category for HDFS Actor");

namespace wrench {

    /**
     * @brief: HdfsService constructor
     *
     * @param hostname: the name of the host on which the service should be started
     * @param job: the MR job
     * @param compute_resources: a set of hostnames
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     */
    HdfsService::HdfsService(
            const std::string &hostname,
            MRJob *job,
            const std::set<std::string> compute_resources,
            std::map<std::string, std::string> property_list,
            std::map<std::string, double> messagepayload_list
    ) :
            Service(hostname,
                    "hdfs_service",
                    "hdfs_service"), job(job) {

        this->compute_resources = compute_resources;

        // Set default and specified properties
        this->setProperties(this->default_property_values, std::move(property_list));

        // Set default and specified message payloads
        this->setMessagePayloads(this->default_messagepayload_values, std::move(messagepayload_list));

    }

    /**
     * @brief Stop the compute service - must be called by the stop()
     *        method of derived classes
     */
    void HdfsService::stop() {
        Service::stop();
    }

    /**
     * @brief Main method of the HdfsMergeService daemon
     *
     * @return 0 on termination
     */
    int HdfsService::main() {
        this->state = Service::UP;

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_YELLOW);
        while (this->processNextMessage()) {

        }

        WRENCH_INFO("HdfsService on host %s terminating cleanly!", S4U_Simulation::getHostName().c_str());
        return 0;
    }

    /**
    * @brief Wait for and react to any incoming message
    *
    * @return false if the daemon should terminate, true otherwise
    *
    * @throw std::runtime_error
    */
    bool HdfsService::processNextMessage() {

        // TODO: DEFINE THE SET OF MESSAGES AN HDFSSERVICE CAN SEND AND RECEIVE

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

            // This is Synchronous
            try {
                S4U_Mailbox::putMessage(msg->ack_mailbox,
                                        new ServiceDaemonStoppedMessage(this->getMessagePayloadValue(
                                                HadoopComputeServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
            } catch (std::shared_ptr<NetworkError> &cause) {
                return false;
            }
            return false;

        } else if (auto msg = std::dynamic_pointer_cast<HdfsReadDataMessage>(message)) {
            WRENCH_INFO("HDFS reading data for a mapper...");
            simulation->readFromDisk(msg->data_size, this->hostname, "/");

            try {
                S4U_Mailbox::putMessage("mapper_service",
                                        new HdfsReadComplete(msg->data_size,
                                                this->getMessagePayloadValue(
                                                HadoopComputeServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
            } catch (std::shared_ptr<NetworkError> &cause) {
                return false;
            }

            return true;
        } else {
                throw std::runtime_error(
                        "HdfsService::processNextMessage(): Received an unexpected [" + message->getName() + "] message!");
        }
    }
}
