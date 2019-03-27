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

#include <random>
#include "wrench/services/Service.h"
#include "wrench/services/network_proximity/NetworkProximityServiceProperty.h"
#include "wrench/services/network_proximity/NetworkProximityServiceMessagePayload.h"

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    class Simulation;

    /**
     * @brief A daemon used by a NetworkProximityService to run network measurements (proximity is computed between two such running daemons)
     */
    class NetworkProximityDaemon: public Service {
    public:

        NetworkProximityDaemon(Simulation *simulation, std::string hostname,
                               std::string network_proximity_service_mailbox,
                               double message_size,double measurement_period,
                               double noise, std::map<std::string, std::string> messagepayload_list);

    private:

        friend class Simulation;

        NetworkProximityDaemon(Simulation *simulation, std::string hostname,
                               std::string network_proximity_service_mailbox,
                               double message_size,double measurement_period,
                               double noise, std::map<std::string, std::string> messagepayload_list, std::string suffix);


        double message_size;
        double measurement_period;
        double max_noise;

        std::string suffix;
        std::string next_mailbox_to_send;
        NetworkProximityDaemon *next_daemon_to_send;
        std::string next_host_to_send;
        std::string network_proximity_service_mailbox;

        int main() override;
        void cleanup(bool has_returned_from_main, int return_value) override;


        double getTimeUntilNextMeasurement();

        bool processNextMessage(double timeout);
    };

    /***********************/
    /** \endcond           */
    /***********************/
}


#endif //WRENCH_NETWORKDAEMONS_H
