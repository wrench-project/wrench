/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/network_proximity/NetworkProximityServiceProperty.h"


namespace wrench {

    SET_PROPERTY_NAME(NetworkProximityServiceProperty, NETWORK_DB_LOOKUP_MESSAGE_PAYLOAD);

    SET_PROPERTY_NAME(NetworkProximityServiceProperty, NETWORK_DAEMON_CONTACT_REQUEST_PAYLOAD);

    SET_PROPERTY_NAME(NetworkProximityServiceProperty, NETWORK_DAEMON_CONTACT_ANSWER_PAYLOAD);

    SET_PROPERTY_NAME(NetworkProximityServiceProperty, NETWORK_DAEMON_COMPUTE_ANSWER_PAYLOAD);

    SET_PROPERTY_NAME(NetworkProximityServiceProperty, NETWORK_PROXIMITY_TRANSFER_MESSAGE_PAYLOAD);

    SET_PROPERTY_NAME(NetworkProximityServiceProperty, LOOKUP_OVERHEAD);

    // TODO: add set property name SERVICE_TYPE
};