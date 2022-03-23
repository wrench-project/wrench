/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/ServiceMessage.h>
#include <wrench/failure_causes/FailureCause.h>

namespace wrench {

    /**
     * @brief Constructor
     * @param payload: message size in bytes
     */
    ServiceMessage::ServiceMessage(double payload) : SimulationMessage(payload) {}

    /**
     * @brief Constructor
     * @param ack_mailbox: mailbox to which the DaemonStoppedMessage ack will be sent. No ack will be sent if ack_mailbox=""
     * @param send_failure_notifications: whether the service should send failure notifications before terminating
     * @param termination_cause: the termination cause (if failure notifications are sent)
     * @param payload: message size in bytes
     *
     * @throw std::invalid_arguments
     */
    ServiceStopDaemonMessage::ServiceStopDaemonMessage(simgrid::s4u::Mailbox *ack_mailbox, bool send_failure_notifications,
                                                       ComputeService::TerminationCause termination_cause,
                                                       double payload)
        : ServiceMessage(payload), ack_mailbox(ack_mailbox), send_failure_notifications(send_failure_notifications), termination_cause(termination_cause) {}

    /**
     * @brief Constructor
     * @param payload: message size in bytes
     *
     * @throw std::invalid_arguments
     */
    ServiceDaemonStoppedMessage::ServiceDaemonStoppedMessage(double payload)
        : ServiceMessage(payload) {}


    /**
      * @brief Constructor
      * @param payload: message size in bytes
      *
      * @throw std::invalid_arguments
      */
    ServiceTTLExpiredMessage::ServiceTTLExpiredMessage(double payload)
        : ServiceMessage(payload) {}


};// namespace wrench
