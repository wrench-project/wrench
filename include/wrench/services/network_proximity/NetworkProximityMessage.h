/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_NETWORKPROXIMITYMESSAGE_H
#define WRENCH_NETWORKPROXIMITYMESSAGE_H

#include "wrench/services/network_proximity/NetworkProximitySenderDaemon.h"
#include "wrench/services/ServiceMessage.h"

namespace wrench {

    /***********************/
    /** \cond INTERNAL    **/
    /***********************/

    /**
     * @brief Top-level class for messages received/sent by a NetworkProximityService
     */
    class NetworkProximityMessage : public ServiceMessage {
    protected:
        NetworkProximityMessage(double payload);
    };


    /**
     * @brief A message sent to a NetworkProximityService to request a network proximity lookup
     */
    class NetworkProximityLookupRequestMessage : public NetworkProximityMessage {
    public:
        NetworkProximityLookupRequestMessage(S4U_CommPort *answer_commport, std::pair<std::string, std::string> hosts,
                                             double payload);

        /** @brief The commport_name to which the answer message should be sent */
        S4U_CommPort *answer_commport;
        /** @brief The hosts between which to calculate a proximity value */
        std::pair<std::string, std::string> hosts;
    };


    /**
     * @brief A message sent by a NetworkProximityService in answer to a network proximity lookup request
     */
    class NetworkProximityLookupAnswerMessage : public NetworkProximityMessage {
    public:
        NetworkProximityLookupAnswerMessage(std::pair<std::string, std::string> hosts, double proximity_value,
                                            double timestamp, double payload);

        /** @brief The hosts whose proximity values were calculated */
        std::pair<std::string, std::string> hosts;
        /** @brief The calculated proximity value */
        double proximity_value;
        /** @brief The timestamp of the oldest measurement data used to calculate the proximity value */
        double timestamp;
    };


    /**
     * @brief A message received by a NetworkProximityService that updates its database of proximity values
     */
    class NetworkProximityComputeAnswerMessage : public NetworkProximityMessage {
    public:
        NetworkProximityComputeAnswerMessage(std::pair<std::string, std::string> hosts, double proximity_value,
                                             double payload);

        /** @brief The hosts whose proximity values were calculated */
        std::pair<std::string, std::string> hosts;
        /** @brief The computed proximity value */
        double proximity_value;
    };

    /**
     * @brief A message sent between NetworkProximitySenderDaemon processes to perform network measurements
     */
    class NetworkProximityTransferMessage : public NetworkProximityMessage {
    public:
        NetworkProximityTransferMessage(double payload);
    };

    /**
     * @brief A message sent to a NetworkProximityService by a NetworkProximitySenderDaemon to ask which other NetworkProximityDaemons it should do measurements with next
     */
    class NextContactDaemonRequestMessage : public NetworkProximityMessage {
    public:
        NextContactDaemonRequestMessage(std::shared_ptr<NetworkProximitySenderDaemon> daemon, double payload);

        /** @brief The NetworkProximitySenderDaemon daemon to return the answer to */
        std::shared_ptr<NetworkProximitySenderDaemon> daemon;
    };

    /**
     * @brief A message sent by a NetworkProximityService to a NetworkProximitySenderDaemon to tell it which other NetworkProximityDaemons it should do measurements with next
     */
    class NextContactDaemonAnswerMessage : public NetworkProximityMessage {
    public:
        NextContactDaemonAnswerMessage(std::string next_host_to_send,
                                       std::shared_ptr<NetworkProximityReceiverDaemon> next_daemon_to_send,
                                       S4U_CommPort *next_commport_to_send, double payload);

        /** @brief The next host for the NetworkProximitySenderDaemon to contact */
        std::string next_host_to_send;

        /** @brief The next NetworkProximitySenderDaemon for the NetworkProximitySenderDaemon to contact */
        std::shared_ptr<NetworkProximityReceiverDaemon> next_daemon_to_send;

        /** @brief The next commport_name for the network daemon to contact */
        S4U_CommPort *next_commport_to_send;
    };

    /**
     * @brief A message sent to a NetworkProximityService to request a coordinate lookup
     */
    class CoordinateLookupRequestMessage : public NetworkProximityMessage {
    public:
        CoordinateLookupRequestMessage(S4U_CommPort *answer_commport, std::string requested_host, double payload);

        /** @brief The commport_name to which the answer should be sent back */
        S4U_CommPort *answer_commport;

        /** @brief The name of the host whose coordinates are being requested */
        std::string requested_host;
    };

    /**
     * @brief A message sent by a NetworkProximityService in answer to a coordinate lookup request
     */
    class CoordinateLookupAnswerMessage : public NetworkProximityMessage {
    public:
        CoordinateLookupAnswerMessage(std::string requested_host, bool success, std::pair<double, double> xy_coordinate,
                                      double timestamp, double payload);

        /** @brief The name of the host whose coordinates were requested  */
        std::string requested_host;

        /** @brief Whether the lookup was successful or not */
        bool success;

        /** @brief The current (x,y) coordinates of the requested host */
        std::pair<double, double> xy_coordinate;
        /** @brief The timestamp of the oldest measurement data used to calculate the proximity value */
        double timestamp;
    };

    /***********************/
    /** \endcond          **/
    /***********************/
}// namespace wrench


#endif//WRENCH_NETWORKPROXIMITYMESSAGE_H
