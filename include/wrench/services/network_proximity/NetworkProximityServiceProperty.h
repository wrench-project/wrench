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

#include <wrench/services/ServiceProperty.h>

namespace wrench {

    /**
     * @brief Configurable properties for a NetworkProximityService
     */

    class NetworkProximityServiceProperty:public ServiceProperty {
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

        /** @brief The type of network proximity service to be used **/
        DECLARE_PROPERTY_NAME(NETWORK_PROXIMITY_SERVICE_TYPE);

        /** @brief The message size (in bytes) to be used **/
        DECLARE_PROPERTY_NAME(NETWORK_PROXIMITY_MESSAGE_SIZE);

        /** @brief The measurement period (in seconds) to be used **/
        DECLARE_PROPERTY_NAME(NETWORK_PROXIMITY_MEASUREMENT_PERIOD);

        /** @brief The maximum random uniformly distributed noise (in seconds) to be added to the measurement period **/
        DECLARE_PROPERTY_NAME(NETWORK_PROXIMITY_MEASUREMENT_PERIOD_MAX_NOISE);

        /** @brief The percentage of other network daemons that each network daemon will communicate with **/
        DECLARE_PROPERTY_NAME(NETWORK_DAEMON_COMMUNICATION_COVERAGE);

        /** @brief The random number generator seed (int) used by the service to decide a sequence of communication peers **/
        DECLARE_PROPERTY_NAME(NETWORK_PROXIMITY_PEER_LOOKUP_SEED);
    };
}


#endif //WRENCH_NETWORKPROXIMITYSERVICEPROPERTY_H
