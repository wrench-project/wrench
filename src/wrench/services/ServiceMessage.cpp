/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/ServiceMessage.h>

namespace wrench {

    /**
     * @brief Constructor
     * @param name: the message name
     * @param payload: message size in bytes
     */
    ServiceMessage::ServiceMessage(std::string name, double payload) :
            SimulationMessage("ServiceMessage::" + name, payload) {}

    /**
     * @brief Constructor
     * @param ack_mailbox: mailbox to which the DaemonStoppedMessage ack will be sent. No ack will be sent if ack_mailbox=""
     * @param payload: message size in bytes
     *
     * @throw std::invalid_arguments
     */
    ServiceStopDaemonMessage::ServiceStopDaemonMessage(std::string ack_mailbox, double payload)
            : ServiceMessage("STOP_DAEMON", payload), ack_mailbox(std::move(ack_mailbox)) {}

    /**
     * @brief Constructor
     * @param payload: message size in bytes
     *
     * @throw std::invalid_arguments
     */
    ServiceDaemonStoppedMessage::ServiceDaemonStoppedMessage(double payload)
            : ServiceMessage("DAEMON_STOPPED", payload) {}


    /**
      * @brief Constructor
      * @param payload: message size in bytes
      *
      * @throw std::invalid_arguments
      */
    ServiceTTLExpiredMessage::ServiceTTLExpiredMessage(double payload)
            : ServiceMessage("TTL_EXPIRED", payload) {}


};
