/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_NETWORKPROXIMITYSERVICEPROPERTY_H
#define WRENCH_NETWORKPROXIMITYSERVICEPROPERTY_H

#include "wrench/services/ServiceProperty.h"

namespace wrench {

    /**
     * @brief Configurable properties for a NetworkProximityService
     */

    class NetworkProximityServiceProperty:public ServiceProperty {
    public:

        /** @brief The overhead, in seconds, of looking up entries for a file (default: 0) **/
        DECLARE_PROPERTY_NAME(LOOKUP_OVERHEAD);

        /** @brief The type of network proximity implementation to be used:
         *   - ALLTOALL: a simple all-to-all algorithm (default)
         *   - VIVALDI: The Vivaldi network coordinate-based approach
         */
        DECLARE_PROPERTY_NAME(NETWORK_PROXIMITY_SERVICE_TYPE);

        /** @brief The message size (in bytes) to be used in RTT measurements (default: 1024) **/
        DECLARE_PROPERTY_NAME(NETWORK_PROXIMITY_MESSAGE_SIZE);

        /** @brief The inter-measurement period (in seconds) to be used (default: 60) **/
        DECLARE_PROPERTY_NAME(NETWORK_PROXIMITY_MEASUREMENT_PERIOD);

        /** @brief The maximum random uniformly distributed noise (in seconds) to be added to the measurement period (useful
         * to avoid idiosyncratic effects of perfect synchrony) (default: 20) **/
        DECLARE_PROPERTY_NAME(NETWORK_PROXIMITY_MEASUREMENT_PERIOD_MAX_NOISE);

        /** @brief The seed for the noise random number generator **/
        DECLARE_PROPERTY_NAME(NETWORK_PROXIMITY_MEASUREMENT_PERIOD_NOISE_SEED);

        /** @brief The percentage of other network proximity daemons that each network proximity daemon will conduct RTT measurements with (default: 1.0)**/
        DECLARE_PROPERTY_NAME(NETWORK_DAEMON_COMMUNICATION_COVERAGE);

        /** @brief The random (integer) number generator seed used by the service to pick RTT measurement peers (default: 1) **/
        DECLARE_PROPERTY_NAME(NETWORK_PROXIMITY_PEER_LOOKUP_SEED);
    };
}


#endif //WRENCH_NETWORKPROXIMITYSERVICEPROPERTY_H
