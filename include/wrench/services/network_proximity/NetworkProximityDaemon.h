/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_NETWORKDAEMONS_H
#define WRENCH_NETWORKDAEMONS_H

#include "wrench/services/Service.h"
#include "wrench/services/network_proximity/NetworkProximityServiceProperty.h"

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    class Simulation;

    /**
     * @brief A network daemon running (proximity is computed between two such running daemons)
     */
    class NetworkProximityDaemon: public Service {
    public:

        NetworkProximityDaemon(Simulation *simulation, std::string hostname,
                       std::string network_proximity_service_mailbox,
        double message_size,double measurement_period,
        double noise);

    private:
        std::map<std::string, std::string> default_property_values =
                {{NetworkProximityServiceProperty::STOP_DAEMON_MESSAGE_PAYLOAD,          "1024"},
                 {NetworkProximityServiceProperty::DAEMON_STOPPED_MESSAGE_PAYLOAD,       "1024"},
                 {NetworkProximityServiceProperty::NETWORK_DAEMON_CONTACT_REQUEST_PAYLOAD,    "1024"},
                 {NetworkProximityServiceProperty::NETWORK_DAEMON_CONTACT_ANSWER_PAYLOAD,    "1024"},
                 {NetworkProximityServiceProperty::NETWORK_PROXIMITY_TRANSFER_MESSAGE_PAYLOAD,    "1024"},
                 {NetworkProximityServiceProperty::NETWORK_DAEMON_COMPUTE_ANSWER_PAYLOAD,    "1024"},
                 {NetworkProximityServiceProperty::LOOKUP_OVERHEAD,                      "0.0"},
                };



    private:

        friend class Simulation;

        NetworkProximityDaemon(Simulation *simulation, std::string hostname,
                       std::string network_proximity_service_mailbox,
                       double message_size,double measurement_period,
                       double noise, std::string suffix);


        double message_size;
        double measurement_period;
        double max_noise;
        std::string suffix;
        std::string next_mailbox_to_send;
        std::string next_host_to_send;
        std::string network_proximity_service_mailbox;

        int main();

        double getTimeUntilNextMeasurement();

          bool processNextMessage(double timeout);
    };

    /***********************/
    /** \endcond           */
    /***********************/
}


#endif //WRENCH_NETWORKDAEMONS_H
