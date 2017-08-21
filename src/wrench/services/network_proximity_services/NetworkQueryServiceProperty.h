/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_NETWORKQUERYSERVICEPROPERTY_H
#define WRENCH_NETWORKQUERYSERVICEPROPERTY_H

#include <services/ServiceProperty.h>

namespace wrench {

    class NetworkQueryServiceProperty:public ServiceProperty {
    public:
        /** @brief The number of bytes in a request control message sent to the daemon to request a list of file locations **/
        DECLARE_PROPERTY_NAME(NETWORK_DB_LOOKUP_MESSAGE_PAYLOAD);

        /** @brief The number of bytes to send to the network daemon manager **/
        DECLARE_PROPERTY_NAME(NETWORK_DAEMON_CONTACT_REQUEST_PAYLOAD);

        /** @brief The number of bytes to send to reply to the network daemon from daemon manager **/
        DECLARE_PROPERTY_NAME(NETWORK_DAEMON_CONTACT_ANSWER_PAYLOAD);

        /** @brief The number of bytes to transfer to measure the proximity **/
        DECLARE_PROPERTY_NAME(NETWORK_PROXIMITY_TRANSFER_MESSAGE_PAYLOAD);

        /** @brief The number of bytes to transfer to measure the proximity **/
        DECLARE_PROPERTY_NAME(NETWORK_DAEMON_COMPUTE_ANSWER_PAYLOAD);

        /** @brief The overhead, in seconds, of looking up entries for a file **/
        DECLARE_PROPERTY_NAME(LOOKUP_OVERHEAD);
    };
}


#endif //WRENCH_NETWORKQUERYSERVICEPROPERTY_H
