/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/compute/hadoop/hadoop_subsystem/MapperService.h"
#include "wrench/services/compute/hadoop/HadoopComputeService.h"
#include "wrench/services/ServiceMessage.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "wrench/simulation/Simulation.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/workflow/failure_causes/NetworkError.h"
#include "../HadoopComputeServiceMessage.h"


WRENCH_LOG_CATEGORY(mapper_service, "Log category for Mapper Actor");

namespace wrench {
    /**
     * @brief: MapperService constructor
     *
     * @param hostname: the name of the host on which the service should be started
     * @param job: the job to execute
     * @param compute_resources: a set of hostnames
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     */
    MapperService::MapperService(const std::string &hostname,
                                 MRJob *job, const std::set<std::string> &compute_resources,
                                 std::map<std::string, std::string> property_list,
                                 std::map<std::string, double> messagepayload_list
    ) : Service(hostname, "mapper_service",
                "mapper_service"), job(job) {
        this->compute_resources = compute_resources;
        this->setProperties(this->default_property_values, std::move(property_list));
        this->setMessagePayloads(this->default_messagepayload_values, std::move(messagepayload_list));
    }

    /**
     * @brief Stop the compute service - must be called by the stop()
     *        method of derived classes
     */
    void MapperService::stop() {
        Service::stop();
    }

    /**
     * @brief Main method of the MapperService daemon
     *
     * @return 0 on termination
     */
    int MapperService::main() {
        this->state = Service::UP;
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_YELLOW);

        /** Request data to start the job */
        try {
            S4U_Mailbox::putMessage(this->job->getHdfsMailboxName(),
                                    new HdfsReadDataMessage(this->job->getDataSize(),
                                                            this->mailbox_name,
                                                            this->getMessagePayloadValue(
                                                                    MRJobExecutorMessagePayload::MAP_SIDE_HDFS_DATA_REQUEST_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            return false;
        }

        /** Main loop */
        while (this->processNextMessage()) {
        }

        WRENCH_INFO("MapperService on host %s terminating cleanly!", S4U_Simulation::getHostName().c_str());
        return 0;
    }

    /**
     * @brief Wait for and react to any incoming message
     *
     * @return false if the daemon should terminate, true otherwise
     *
     * @throw std::runtime_error
     */
    bool MapperService::processNextMessage() {
        S4U_Simulation::computeZeroFlop();

        // Wait for a message
        std::shared_ptr<SimulationMessage> message;
        try {
            message = S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<NetworkError> &error) { WRENCH_INFO(
                    "Got a network error while getting some message... ignoring");
            return true;
        }

        WRENCH_INFO("MapperService::MapperService() Got a [%s] message", message->getName().c_str());
        if (auto msg = std::dynamic_pointer_cast<ServiceStopDaemonMessage>(message)) {
            // This is Synchronous
            try {
                S4U_Mailbox::putMessage(msg->ack_mailbox,
                                        new ServiceDaemonStoppedMessage(this->getMessagePayloadValue(
                                                MRJobExecutorMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
            } catch (std::shared_ptr<NetworkError> &cause) {
                return false;
            }
            return false;
        } else if (auto msg = std::dynamic_pointer_cast<HdfsReadCompleteMessage>(message)) {
            // TODO: Use the correct values.
            // The mapper computes the user map function, writes spill files locally,
            // and then merges all spill files back into a single map file.

            // The materilaized output is found via:
            // https://github.com/wrench-project/understanding_hadoop/blob/00a1c5653ae025e9db723d4c3e2137f2bb2b5472/hadoop_mr_tests/map_output_materialized_bytes/run_test.py#L109
            this->materialized_ouput_bytes = 1000.0;

            // For the spill phase refer to:
            // https://github.com/wrench-project/understanding_hadoop/blob/master/resources/top_level_overview.markdown#point-3--4-mapping--handling-intermediate-map-values

            // TODO: Consider how to handle a map task failure.
            bool success = true;

            // Compute map function
            Simulation::compute(this->job->getMapperFlops());

            // Write spill file(s)
            simulation->writeToDisk(this->job->getDataSize(), "WMSHost", "/");

            // Merge all spilled files to single output
            Simulation::compute(this->job->getMapperFlops());

            // Notify the MRJobExecutor that we have finished
            try {
                S4U_Mailbox::putMessage(this->job->getExecutorMailbox(),
                                        new MapTaskCompleteMessage(success, this->mailbox_name,
                                                                   this->getMessagePayloadValue(
                                                                           MRJobExecutorMessagePayload::MAP_SIDE_SHUFFLE_REQUEST_PAYLOAD)));
            } catch (std::shared_ptr<NetworkError> &cause) {
                return false;
            }

            return true;
        } else if (auto msg = std::dynamic_pointer_cast<RequestMapperMaterializedOutputMessage>(message)) {
            S4U_Mailbox::putMessage(msg->return_mailbox,
                                    new SendMaterializedOutputMessage(this->materialized_ouput_bytes,
                                                                      this->getMessagePayloadValue(
                                                                                    MRJobExecutorMessagePayload::MAP_OUTPUT_MATERIALIZED_BYTES_PAYLOAD)));
            return true;
        } else {
            throw std::runtime_error(
                    "MapperService::processNextMessage(): Received an unexpected [" + message->getName() +
                    "] message!");
        }
    }
}
