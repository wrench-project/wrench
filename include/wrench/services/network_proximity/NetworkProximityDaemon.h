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
    class NetworkProximityDaemon : public Service {
    public:
        NetworkProximityDaemon(Simulation *simulation, std::string hostname,
                               simgrid::s4u::Mailbox *network_proximity_service_mailbox,
                               double message_size, double measurement_period,
                               double noise, int noise_seed, WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list);

    private:
        friend class Simulation;

        std::default_random_engine rng;

        NetworkProximityDaemon(Simulation *simulation, std::string hostname,
                               simgrid::s4u::Mailbox *network_proximity_service_mailbox,
                               double message_size, double measurement_period,
                               double noise, int noise_seed, WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list, std::string suffix);


        double message_size;
        double measurement_period;
        double max_noise;

        std::string suffix;
        simgrid::s4u::Mailbox *next_mailbox_to_send;
        std::shared_ptr<NetworkProximityDaemon> next_daemon_to_send;
        std::string next_host_to_send;
        simgrid::s4u::Mailbox *network_proximity_service_mailbox;

        int main() override;
        void cleanup(bool has_returned_from_main, int return_value) override;


        double getTimeUntilNextMeasurement();

        bool processNextMessage(double timeout);
    };

    /***********************/
    /** \endcond           */
    /***********************/
}// namespace wrench


#endif//WRENCH_NETWORKDAEMONS_H
