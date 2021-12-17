/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/network_proximity/NetworkProximityServiceProperty.h>


namespace wrench {


    SET_PROPERTY_NAME(NetworkProximityServiceProperty, LOOKUP_OVERHEAD);

    SET_PROPERTY_NAME(NetworkProximityServiceProperty, NETWORK_PROXIMITY_SERVICE_TYPE);

    SET_PROPERTY_NAME(NetworkProximityServiceProperty, NETWORK_PROXIMITY_MESSAGE_SIZE);

    SET_PROPERTY_NAME(NetworkProximityServiceProperty, NETWORK_PROXIMITY_MEASUREMENT_PERIOD);

    SET_PROPERTY_NAME(NetworkProximityServiceProperty, NETWORK_PROXIMITY_MEASUREMENT_PERIOD_MAX_NOISE);

    SET_PROPERTY_NAME(NetworkProximityServiceProperty, NETWORK_PROXIMITY_MEASUREMENT_PERIOD_NOISE_SEED);

    SET_PROPERTY_NAME(NetworkProximityServiceProperty, NETWORK_DAEMON_COMMUNICATION_COVERAGE);

    SET_PROPERTY_NAME(NetworkProximityServiceProperty, NETWORK_PROXIMITY_PEER_LOOKUP_SEED);
};