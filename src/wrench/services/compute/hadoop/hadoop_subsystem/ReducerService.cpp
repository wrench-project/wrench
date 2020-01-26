/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/compute/hadoop/hadoop_subsystem/ReducerService.h"
#include "wrench/services/compute/hadoop/HadoopComputeService.h"


#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "wrench/services/compute/ComputeService.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/services/ServiceMessage.h"
#include "wrench/workflow/execution_events/FailureCause.h"

WRENCH_LOG_NEW_DEFAULT_CATEGORY(hadoop_compute_servivce, "Log category for Reducer Actor");

namespace wrench {

    ReducerService::ReducerService(const std::string &hostname,
            MRJob &MRJob
    ) : ComputeService(hostname, "hadooop", "hadoop", ""), job(MRJob) {

    }

    bool ReducerService::processNextMessage() {

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
                    "ReducerService::processNextMessage(): Received an unexpected [" + message->getName() + "] message!");
        }
    }

    int ReducerService::main() {
        this->state = Service::UP;

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_YELLOW);
        while (this->processNextMessage()) {

        }

        WRENCH_INFO("ReducerService on host %s terminating cleanly!", S4U_Simulation::getHostName().c_str());
        return 0;
    }
}