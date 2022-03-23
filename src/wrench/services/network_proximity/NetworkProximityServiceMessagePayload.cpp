/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/network_proximity/NetworkProximityServiceMessagePayload.h>


namespace wrench {

    SET_MESSAGEPAYLOAD_NAME(NetworkProximityServiceMessagePayload, NETWORK_DB_LOOKUP_REQUEST_MESSAGE_PAYLOAD);

    SET_MESSAGEPAYLOAD_NAME(NetworkProximityServiceMessagePayload, NETWORK_DB_LOOKUP_ANSWER_MESSAGE_PAYLOAD);

    SET_MESSAGEPAYLOAD_NAME(NetworkProximityServiceMessagePayload, NETWORK_DAEMON_CONTACT_REQUEST_PAYLOAD);

    SET_MESSAGEPAYLOAD_NAME(NetworkProximityServiceMessagePayload, NETWORK_DAEMON_CONTACT_ANSWER_PAYLOAD);

    SET_MESSAGEPAYLOAD_NAME(NetworkProximityServiceMessagePayload, NETWORK_DAEMON_MEASUREMENT_REPORTING_PAYLOAD);

};// namespace wrench