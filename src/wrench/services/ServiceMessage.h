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


#include <simulation/SimulationMessage.h>

namespace wrench {


    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Top-level SimuationMessage class
     */
    class ServiceMessage : public SimulationMessage {
    protected:
        ServiceMessage(std::string name, double payload);

    };

    /**
     * @brief "STOP_DAEMON" SimulationMessage class
     */
    class ServiceStopDaemonMessage : public ServiceMessage {
    public:
        ServiceStopDaemonMessage(std::string ack_mailbox, double payload);

        std::string ack_mailbox;
    };

    /**
     * @brief "DAEMON_STOPPED" SimulationMessage class
     */
    class ServiceDaemonStoppedMessage : public ServiceMessage {
    public:
        ServiceDaemonStoppedMessage(double payload);
    };

    /***********************/
    /** \endcond           */
    /***********************/

};

#endif //WRENCH_SERVICEMESSAGE_H
