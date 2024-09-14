/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/failure_causes/FailureCause.h>
#include <wrench/services/ServiceMessage.h>

namespace wrench {

/**
 * @brief Constructor
 * @param payload: message size in bytes
 */
ServiceMessage::ServiceMessage(double payload) : SimulationMessage(payload) {}

/**
 * @brief Constructor
 * @param ack_commport: commport to which the DaemonStoppedMessage ack will be
 * sent. No ack will be sent if ack_commport=""
 * @param send_failure_notifications: whether the service should send failure
 * notifications before terminating
 * @param termination_cause: the termination cause (if failure notifications are
 * sent)
 * @param payload: message size in bytes
 *
 * @throw std::invalid_arguments
 */
ServiceStopDaemonMessage::ServiceStopDaemonMessage(
    S4U_CommPort *ack_commport, bool send_failure_notifications,
    ComputeService::TerminationCause termination_cause, double payload)
    : ServiceMessage(payload), ack_commport(ack_commport),
      send_failure_notifications(send_failure_notifications),
      termination_cause(termination_cause) {}

/**
 * @brief Constructor
 * @param payload: message size in bytes
 *
 * @throw std::invalid_arguments
 */
ServiceDaemonStoppedMessage::ServiceDaemonStoppedMessage(double payload)
    : ServiceMessage(payload) {}

} // namespace wrench
