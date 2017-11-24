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
     * @brief Top-level SimulationMessage class
     */
    class ServiceMessage : public SimulationMessage {
    public:
      virtual ~ServiceMessage(){};
    protected:
        ServiceMessage(std::string name, double payload);

    };

    /**
     * @brief ServiceStopDaemonMessage class
     */
    class ServiceStopDaemonMessage : public ServiceMessage {
    public:
        ~ServiceStopDaemonMessage(){};

        ServiceStopDaemonMessage(std::string ack_mailbox, double payload);

        /** @brief the mailbox to which the "I stopped" ack should be sent */
        std::string ack_mailbox;
    };

    /**
     * @brief ServiceDaemonStoppedMessage class
     */
    class ServiceDaemonStoppedMessage : public ServiceMessage {
    public:
        ServiceDaemonStoppedMessage(double payload);
    };


    /**
    * @brief ServiceTTLExpiredMessage class
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
