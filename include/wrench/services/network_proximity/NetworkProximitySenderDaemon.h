/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_NETWORKSENDERDAEMON_H
#define WRENCH_NETWORKSENDERDAEMON_H

#include <random>
#include "wrench/services/Service.h"
#include "wrench/services/network_proximity/NetworkProximityServiceProperty.h"
#include "wrench/services/network_proximity/NetworkProximityServiceMessagePayload.h"
#include "wrench/services/network_proximity/NetworkProximityReceiverDaemon.h"

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    class Simulation;

    /**
     * @brief A daemon used by a NetworkProximityService to run network measurements (proximity is computed between two such running daemons)
     */
    class NetworkProximitySenderDaemon : public Service {
    public:
        NetworkProximitySenderDaemon(Simulation *simulation, std::string hostname,
                                     S4U_CommPort *network_proximity_service_commport,
                                     double message_size, double measurement_period,
                                     double noise, int noise_seed, WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list);

    private:
        friend class Simulation;

        std::default_random_engine rng;

        //        NetworkProximitySenderDaemon(Simulation *simulation, std::string hostname,
        //                               S4U_CommPort *network_proximity_service_commport,
        //                               double message_size, double measurement_period,
        //                               double noise, int noise_seed, WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list, std::string suffix);


        double message_size;
        double measurement_period;
        double max_noise;

        std::string suffix;
        S4U_CommPort *next_commport_to_send;
        std::shared_ptr<NetworkProximityReceiverDaemon> next_daemon_to_send;
        std::string next_host_to_send;
        S4U_CommPort *network_proximity_service_commport;

        int main() override;
        void cleanup(bool has_returned_from_main, int return_value) override;


        double getTimeUntilNextMeasurement();

        bool processNextMessage(double timeout);
    };

    /***********************/
    /** \endcond           */
    /***********************/
}// namespace wrench


#endif//WRENCH_NETWORKSENDERDAEMON_H
