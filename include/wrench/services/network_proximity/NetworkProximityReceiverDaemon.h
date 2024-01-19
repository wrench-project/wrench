/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_NETWORKRECEIVERDAEMON_H
#define WRENCH_NETWORKRECEIVERDAEMON_H

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
     * @brief A daemon used by a NetworkProximityService to run network measurements
     *        (proximity is computed between two such running daemons)
     */
    class NetworkProximityReceiverDaemon : public Service {
    public:
        NetworkProximityReceiverDaemon(Simulation *simulation, std::string hostname, WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list);

    private:
        friend class Simulation;

        int main() override;
        void cleanup(bool has_returned_from_main, int return_value) override;

    };

    /***********************/
    /** \endcond           */
    /***********************/
}// namespace wrench


#endif//WRENCH_NETWORKRECEIVERDAEMON_H
