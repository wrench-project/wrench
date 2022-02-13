/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_SERVICEMESSAGE_H
#define WRENCH_SERVICEMESSAGE_H


#include "wrench/simulation/SimulationMessage.h"
#include "wrench/services/compute/ComputeService.h"

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Top-level class for messages received/sent by a Service
     */
    class ServiceMessage : public SimulationMessage {
    protected:
        ServiceMessage(std::string name, double payload);

    };

    /**
     * @brief A message sent to a Service to request for it to terminate
     */
    class ServiceStopDaemonMessage : public ServiceMessage {
    public:
//        ~ServiceStopDaemonMessage(){};

        ServiceStopDaemonMessage(simgrid::s4u::Mailbox *ack_mailbox, bool send_failure_notifications, ComputeService::TerminationCause termination_cause, double payload);

        /** @brief the mailbox to which the "I stopped" ack should be sent */
        simgrid::s4u::Mailbox *ack_mailbox;
        /** @brief whether the service should send failure notifications before terminating **/
        bool send_failure_notifications;
        /** @brief The termination cause for the failure notifications, if any **/
        ComputeService::TerminationCause termination_cause;
    };

    /**
     * @brief A message sent by a Service to acknowledge a terminate request
     */
    class ServiceDaemonStoppedMessage : public ServiceMessage {
    public:
        ServiceDaemonStoppedMessage(double payload);
    };


    /**
    * @brief A message sent to a Service to notify it that its time-to-live has expired (which will
     *       cause the service to terminate)
    */
    class ServiceTTLExpiredMessage : public ServiceMessage {
    public:
        ServiceTTLExpiredMessage(double payload);
    };




    /***********************/
    /** \endcond           */
    /***********************/

};

#endif //WRENCH_SERVICEMESSAGE_H
