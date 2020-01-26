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

#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "wrench/services/compute/ComputeService.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/services/ServiceMessage.h"
#include "wrench/workflow/execution_events/FailureCause.h"


WRENCH_LOG_NEW_DEFAULT_CATEGORY(hadoop_compute_servivce, "Log category for Mapper Actor");

namespace wrench {

    MapperService::MapperService(const std::string &hostname,
            MRJob &MRJob, double map_function_cost
    ) : ComputeService(hostname, "hadooop", "hadoop", ""), job(MRJob) {
        this->setMapFunctionCost(map_function_cost);
    }

    MapperService::~MapperService() = default;

    std::pair<double, double> MapperService::calculateMapperCost() {
        /**
         * TODO: pass the HDFS reaed and write costs as Mapper
         * parameters that a Job instance can specify.
         */
        auto *spill = new Spill(this->job);
        auto *merge = new MapSideMerge(this->job);

        double total_map_cpu_cost = this->getMapFunctionCost();
        double total_map_io_cost = 0.0;

        if (this->job.getJobType() == std::string("deterministic")) {
            total_map_io_cost += (spill->deterministicSpillIOCost() +
                                  merge->deterministicMapSideMergeIOCost());
            total_map_cpu_cost += (spill->deterministicSpillCPUCost() +
                                  merge->deterministicMapSideMergCPUCost());
        }
        return std::make_pair(total_map_cpu_cost, total_map_io_cost);
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

        } else {
            throw std::runtime_error(
                    "MapperService::processNextMessage(): Received an unexpected [" + message->getName() + "] message!");
        }
    }

    int MapperService::main() {
        this->state = Service::UP;

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_YELLOW);
        while (this->processNextMessage()) {

        }

        WRENCH_INFO("MapperService on host %s terminating cleanly!", S4U_Simulation::getHostName().c_str());
        return 0;
    }
}