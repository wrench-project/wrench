/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_NETWORKPROXIMITYSERVICEMESSAGEPAYLOAD_H
#define WRENCH_NETWORKPROXIMITYSERVICEMESSAGEPAYLOAD_H

#include "wrench/services/ServiceMessagePayload.h"

namespace wrench {

    /**
     * @brief Configurable message payloads for a NetworkProximityService
     */

    class NetworkProximityServiceMessagePayload:public ServiceMessagePayload {
    public:
        /** @brief The number of bytes in the message sent to the service to request a proximity value lookup **/
        DECLARE_MESSAGEPAYLOAD_NAME(NETWORK_DB_LOOKUP_REQUEST_MESSAGE_PAYLOAD);

        /** @brief The number of bytes in the message sent by the service in answer to a request for a proximity value lookup **/
        DECLARE_MESSAGEPAYLOAD_NAME(NETWORK_DB_LOOKUP_ANSWER_MESSAGE_PAYLOAD);

        /** @brief The number of bytes in the message sent by a network proximity daemon to the network 
         * proximity service to request which other network proximity daemon it should run network proximity 
         * experiments with **/
        DECLARE_MESSAGEPAYLOAD_NAME(NETWORK_DAEMON_CONTACT_REQUEST_PAYLOAD);

        /** @brief The number of bytes in the message sent by the service to a network proximity daemon in 
         *  answer to a request for which other network proximity daemon to run network proximity 
         * experiments with **/
        DECLARE_MESSAGEPAYLOAD_NAME(NETWORK_DAEMON_CONTACT_ANSWER_PAYLOAD);

        /** @brief The number of bytes in the message sent by a network proximity daemon to the network proximity
         *  service to report on an  RTT measurement experiment  **/
        DECLARE_MESSAGEPAYLOAD_NAME(NETWORK_DAEMON_MEASUREMENT_REPORTING_PAYLOAD);

    };
}


#endif //WRENCH_NETWORKPROXIMITYSERVICEMESSAGEPAYLOAD_H
