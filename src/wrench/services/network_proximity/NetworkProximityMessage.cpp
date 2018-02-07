/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "NetworkProximityMessage.h"

namespace wrench {
    /**
     * @brief Constructor
     * @param name: the message name
     * @param payload: the message size in bytes
     */
    NetworkProximityMessage::NetworkProximityMessage(std::string name, double payload) :
            ServiceMessage("NetworkProximity::" + name, payload) {
    }


    /**
     * @brief NetworkProximityLookupRequestMessage class
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param hosts: the pair of hosts to look for
     * @param payload: the message size in bytes
     */
    NetworkProximityLookupRequestMessage::NetworkProximityLookupRequestMessage(std::string answer_mailbox, std::pair<std::string,std::string> hosts, double payload) :
            NetworkProximityMessage("PROXIMITY_LOOKUP_REQUEST", payload) {

        if ((answer_mailbox == "") || (std::get<0>(hosts)=="") || (std::get<1>(hosts)=="")) {
            throw std::invalid_argument("NetworkProximityLookupRequestMessage::NetworkProximityLookupRequestMessage(): Invalid argument");
        }
        this->answer_mailbox = answer_mailbox;
        this->hosts = hosts;
    }


    /**
     * @brief NetworkProximityLookupAnswerMessage class
     * @param hosts: the pair of hosts that were looked up
     * @param proximityvalue: the proximity value between the pair of hosts
     * @param payload: the message size in bytes
     */
    NetworkProximityLookupAnswerMessage::NetworkProximityLookupAnswerMessage(std::pair<std::string,std::string> hosts, double proximityvalue, double payload) :
            NetworkProximityMessage("PROXIMITY_LOOKUP_ANSWER", payload) {
        if ((std::get<0>(hosts)=="") || (std::get<1>(hosts)=="")) {
            throw std::invalid_argument("NetworkProximityLookupAnswerMessage::NetworkProximityLookupAnswerMessage(): Invalid argument");
        }
        this->hosts = hosts;
        this->proximityValue = proximityvalue;
    }

    /**
     * @brief NetworkProximityComputeAnswerMessage class
     * @param hosts: the pair of hosts that were looked up
     * @param proximityvalue: the proximity value between the pair of hosts
     * @param payload: the message size in bytes
     */
    NetworkProximityComputeAnswerMessage::NetworkProximityComputeAnswerMessage(std::pair<std::string,std::string> hosts,double proximityvalue,double payload) :
            NetworkProximityMessage("PROXIMITY_COMPUTE_ANSWER", payload) {
        if ((std::get<0>(hosts)=="") || (std::get<1>(hosts)=="")) {
            throw std::invalid_argument("NetworkProximityComputeAnswerMessage::NetworkProximityComputeAnswerMessage(): Invalid argument");
        }
        this->hosts = hosts;
        this->proximityValue = proximityvalue;
    }


    /**
     * @brief NextContactDaemonRequestMessage class
     * @param answer_mailbox: the mailbox to return the request to
     * @param payload: the message size in bytes
     */
    NextContactDaemonRequestMessage::NextContactDaemonRequestMessage(std::string answer_mailbox, double payload) :
            NetworkProximityMessage("NEXT_CONTACT_DAEMON_REQUEST", payload) {
        this->answer_mailbox = answer_mailbox;
    }

    /**
     * @brief NextContactDaemonAnswerMessage class
     * @param next_host_to_send: the next host to contact
     * @param next_mailbox_to_send: the next mailbox to contact
     * @param payload: the message size in bytes
     */
    NextContactDaemonAnswerMessage::NextContactDaemonAnswerMessage(std::string next_host_to_send,std::string next_mailbox_to_send,double payload) :
            NetworkProximityMessage("NEXT_CONTACT_DAEMON_ANSWER", payload) {
        this->next_host_to_send = next_host_to_send;
        this->next_mailbox_to_send = next_mailbox_to_send;
    }


    /**
     * @brief NetworkProximityTransferMessage class
     * @param message_to_transfer: the message to transfer to measure proximity
     * @param payload: the message size in bytes
     */
    NetworkProximityTransferMessage::NetworkProximityTransferMessage(std::string message_to_transfer,double payload) :
            NetworkProximityMessage("NETWORK_PROXIMITY_TRANSFER", payload) {
        this->message_to_transfer = message_to_transfer;
    }

    /**
     * @brief CoordinateLookupRequestMessage class
     * @param requested_host: the host whose coordinates are being requested
     * @param payload: the message size in bytes
     */
    CoordinateLookupRequestMessage::CoordinateLookupRequestMessage(std::string requested_host, double payload) :
            NetworkProximityMessage("COORDINATE_LOOKUP_REQUEST", payload) {
        this->requested_host = requested_host;
    }

    /**
     * @brief CoordinateLookupAnswerMessage class
     * @param requested_host: the host whose coordinates are being requested
     * @param xy_coordinate: the (x,y) coordinate corresponding to the requested_host
     * @param payload: the message size in bytes
     */
    CoordinateLookupAnswerMessage::CoordinateLookupAnswerMessage(std::string requested_host,
                                                                 std::pair<double, double> xy_coordinate,
                                                                 double payload) :
            NetworkProximityMessage("COORDINATE_LOOKUP_ANSWER", payload) {
        this->requested_host = requested_host;
        this->xy_coordinate = xy_coordinate;
    }
}