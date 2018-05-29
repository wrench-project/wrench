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

#include <wrench/services/network_proximity/NetworkProximityDaemon.h>
#include <wrench/services/ServiceMessage.h>

namespace wrench {

    /***********************/
    /** \cond INTERNAL    **/
    /***********************/

    /**
     * @brief Top-level class for messages received/sent by a NetworkProximityService
     */
    class NetworkProximityMessage : public ServiceMessage {
    protected:
        NetworkProximityMessage(std::string name, double payload);

    };


    /**
     * @brief A message sent to a NetworkProximityService to request a network proximity lookup
     */
    class NetworkProximityLookupRequestMessage : public NetworkProximityMessage {
    public:
        NetworkProximityLookupRequestMessage(std::string answer_mailbox, std::pair<std::string,std::string> hosts, double payload);

        /** @brief The mailbox to which the answer message should be sent */
        std::string answer_mailbox;
        /** @brief The hosts between which to calculate a proximity value */
        std::pair<std::string,std::string> hosts;
    };


    /**
     * @brief A message sent by a NetworkProximityService in answer to a network proximity lookup request
     */
    class NetworkProximityLookupAnswerMessage : public NetworkProximityMessage {
    public:
        NetworkProximityLookupAnswerMessage(std::pair<std::string,std::string> hosts, double proximityvalue, double payload);

        /** @brief The hosts whose proximity values were calculated */
        std::pair<std::string,std::string> hosts;
        /** @brief The calculated proximity value*/
        double proximityValue;
    };


    /**
     * @brief A message received by a NetworkProximityService that updates its database of proximity values
     */
    class NetworkProximityComputeAnswerMessage : public NetworkProximityMessage {
    public:
        NetworkProximityComputeAnswerMessage(std::pair<std::string,std::string> hosts,double proximityValue,double payload);

        /** @brief The hosts whose proximity values were calculated */
        std::pair<std::string,std::string> hosts;
        /** @brief The computed proximityValue */
        double proximityValue;
    };

    /**
     * @brief A message sent between NetworkProximityDaemon processes to perform network measurements
     */
    class NetworkProximityTransferMessage : public NetworkProximityMessage {
    public:
        NetworkProximityTransferMessage(double payload);

    };

    /**
     * @brief A message sent to a NetworkProximityService by a NetworkProximityDaemon to ask which other NetworkProximityDaemons it should do measurements with next
     */
    class NextContactDaemonRequestMessage : public NetworkProximityMessage {
    public:
        NextContactDaemonRequestMessage(NetworkProximityDaemon *daemon, double payload);

        /** @brief The NetworkProximityDaemon daemon to return the answer to */
        NetworkProximityDaemon *daemon;
    };

    /**
     * @brief A message sent by a NetworkProximityService to a NetworkProximityDaemon to tell it which other NetworkProximityDaemons it should do measurements with next
     */
    class NextContactDaemonAnswerMessage : public NetworkProximityMessage {
    public:
        NextContactDaemonAnswerMessage(std::string next_host_to_send,std::string next_mailbox_to_send,double payload);

        /** @brief The next host for the NetworkProximityDaemon to contact */
        std::string next_host_to_send;

        /** @brief The next mailbox for the network daemon to contact */
        std::string next_mailbox_to_send;
    };

    /**
     * @brief A message sent to a NetworkProximityService to request a coordinate lookup
     */
    class CoordinateLookupRequestMessage: public NetworkProximityMessage {
    public:
        CoordinateLookupRequestMessage(std::string answer_mailbox, std::string requested_host, double payload);

        /** @brief The mailbox to which the answer should be sent back */
        std::string answer_mailbox;

        /** @brief The name of the host whose coordinates are being requested */
        std::string requested_host;
    };

    /**
     * @brief A message sent by a NetworkProximityService in answer to a coordinate lookup request
     */
    class CoordinateLookupAnswerMessage: public NetworkProximityMessage {
    public:
        CoordinateLookupAnswerMessage(std::string requested_host, std::pair<double, double> xy_coordinate, double payload);

        /** @brief The name of the host whose coordinates were requested  */
        std::string requested_host;

        /** @brief The current (x,y) coordinates of the requested host */
        std::pair<double, double> xy_coordinate;
    };

    /***********************/
    /** \endcond          **/
    /***********************/
}


#endif //WRENCH_NETWORKPROXIMITYMESSAGE_H
