/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <wrench/logging/TerminalOutput.h>
#include <wrench/services/network_proximity/NetworkProximityReceiverDaemon.h>
#include <wrench/simgrid_S4U_util/S4U_Simulation.h>
#include <wrench/simulation/SimulationMessage.h>
#include <wrench/simgrid_S4U_util/S4U_CommPort.h>
#include "wrench/services/network_proximity/NetworkProximityMessage.h"
#include "wrench/exceptions/ExecutionException.h"


WRENCH_LOG_CATEGORY(wrench_core_network_receiver_daemon_service, "Log category for Network Proximity Receiver Daemon Service");

namespace wrench {

    /**
     * @brief Constructor
     * @param simulation: a pointer to the simulation object
     * @param hostname: the hostname on which to start the service
     * @param messagepayload_list: the message payload list
     */

    NetworkProximityReceiverDaemon::NetworkProximityReceiverDaemon(
            Simulation *simulation,
            const std::string& hostname, const WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE& messagepayload_list) : Service(hostname, "network_proximity_receiver_daemon") {

        setMessagePayloads(messagepayload_list, {});
        this->simulation_ = simulation;
    }

    /**
     * @brief Cleanup method
     *
     * @param has_returned_from_main: whether main() returned
     * @param return_value: the return value (if main() returned)
     */
    void NetworkProximityReceiverDaemon::cleanup(bool has_returned_from_main, int return_value) {
        // Do nothing. It's fine to die, and we'll just autorestart with our previous state
    }


    int NetworkProximityReceiverDaemon::main() {
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_MAGENTA);

        WRENCH_INFO("Network Daemon Receiver Daemon starting on host %s!", S4U_Simulation::getHostName().c_str());

        // In case of restart
        this->commport->reset();
        this->recv_commport->reset();

        /** Main loop **/
        while (true) {
            S4U_Simulation::computeZeroFlop();
            std::shared_ptr<SimulationMessage> message;
            try {
                message = this->commport->getMessage();
            } catch (ExecutionException &e) {
                continue;
            }
            if (auto msg = std::dynamic_pointer_cast<ServiceStopDaemonMessage>(message)) {
                // This is Synchronous
                try {
                    msg->ack_commport->putMessage(
                            new ServiceDaemonStoppedMessage(this->getMessagePayloadValue(
                                    NetworkProximityServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
                } catch (ExecutionException &e) {
                }
                break;
            }
        }
        return 0;
    }

}// namespace wrench
