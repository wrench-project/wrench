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
     * @brief Top-level NetworkProximityMessage class
     */
    class NetworkProximityMessage : public ServiceMessage {
    protected:
        NetworkProximityMessage(std::string name, double payload);

    };


    /**
     * @brief NetworkProximityLookupRequestMessage class
     */
    class NetworkProximityLookupRequestMessage : public NetworkProximityMessage {
    public:
        NetworkProximityLookupRequestMessage(std::string answer_mailbox, std::pair<std::string,std::string> hosts, double payload);

        /** @brief The mailbox to which the answer message should be sent */
        std::string answer_mailbox;
        /** @brief The hosts between whom to calculate proximity value */
        std::pair<std::string,std::string> hosts;
    };


    /**
     * @brief NetworkProximityLookupAnswerMessage class
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
     * @brief NetworkProximityComputeSendMessage class
     */
    class NetworkProximityComputeSendMessage : public NetworkProximityMessage {
    public:
        NetworkProximityComputeSendMessage(std::string send_to_mailbox, std::string message_to_send, std::string reply_to_mailbox);

        /** @brief The mailbox where message should be sent */
        std::string send_to_mailbox;
        /** @brief The message to be sent*/
        std::string message_to_send;
        /** @brief The mailbox to reply the computed proximity value*/
        std::string reply_to_mailbox;
    };

    /**
     * @brief NetworkProximityComputeAnswerMessage class
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
     * @brief NetworkProximityComputeAnswerMessage class
     */
    class NetworkProximityTransferMessage : public NetworkProximityMessage {
    public:
        NetworkProximityTransferMessage(double payload);

    };

    /**
     * @brief NextContactDaemonRequestMessage class
     */
    class NextContactDaemonRequestMessage : public NetworkProximityMessage {
    public:
        NextContactDaemonRequestMessage(NetworkProximityDaemon *daemon, double payload);

        /** @brief The network proximity daemon to return the answer to */
        NetworkProximityDaemon *daemon;
    };

    /**
     * @brief NextContactDaemonAnswerMessage class
     */
    class NextContactDaemonAnswerMessage : public NetworkProximityMessage {
    public:
        NextContactDaemonAnswerMessage(std::string next_host_to_send,std::string next_mailbox_to_send,double payload);

        /** @brief The next host for the network daemon to contact */
        std::string next_host_to_send;

        /** @brief The next mailbox for the network daemon to contact */
        std::string next_mailbox_to_send;
    };

    /**
     * @brief CoordinateLookupRequestMessage class
     */
    class CoordinateLookupRequestMessage: public NetworkProximityMessage {
    public:
        CoordinateLookupRequestMessage(std::string answer_mailbox, std::string requested_host, double payload);

        /** @brief The mailbox to return the answer to */
        std::string answer_mailbox;

        /** @brief The host who's coordinates is being requested */
        std::string requested_host;
    };

    /**
     * @brief CoordinateLookupAnswerMessage class
     */
    class CoordinateLookupAnswerMessage: public NetworkProximityMessage {
    public:
        CoordinateLookupAnswerMessage(std::string requested_host, std::pair<double, double> xy_coordinate, double payload);

        /** @brief The host whose current coordinates were requested for */
        std::string requested_host;

        /** @brief The current (x,y) coordinate corresponding to the requested_host */
        std::pair<double, double> xy_coordinate;
    };
}


#endif //WRENCH_NETWORKPROXIMITYMESSAGE_H
