/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/network_proximity/NetworkProximityMessage.h"

namespace wrench {
    /**
     * @brief Constructor
     * @param payload: the message size in bytes
     */
    NetworkProximityMessage::NetworkProximityMessage(double payload) :
            ServiceMessage(payload) {
    }


    /**
     * @brief Constructor
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param hosts: the pair of hosts to look up
     * @param payload: the message size in bytes
     */
    NetworkProximityLookupRequestMessage::NetworkProximityLookupRequestMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                                                               std::pair<std::string, std::string> hosts,
                                                                               double payload) :
            NetworkProximityMessage( payload), answer_mailbox(answer_mailbox), hosts(hosts) {

        if ((answer_mailbox == nullptr) || (std::get<0>(hosts) == "") || (std::get<1>(hosts) == "")) {
            throw std::invalid_argument(
                    "NetworkProximityLookupRequestMessage::NetworkProximityLookupRequestMessage(): Invalid argument");
        }
    }


    /**
     * @brief Constructor
     * @param hosts: the pair of hosts that were looked up
     * @param proximity_value: the proximity value between the pair of hosts
     * @param timestamp: the timestamp of the proximity value determination
     * @param payload: the message size in bytes
     */
    NetworkProximityLookupAnswerMessage::NetworkProximityLookupAnswerMessage(std::pair<std::string, std::string> hosts,
                                                                             double proximity_value, double timestamp,
                                                                             double payload) :
            NetworkProximityMessage(payload) {
        if ((std::get<0>(hosts) == "") || (std::get<1>(hosts) == "")) {
            throw std::invalid_argument(
                    "NetworkProximityLookupAnswerMessage::NetworkProximityLookupAnswerMessage(): Invalid argument");
        }
        this->hosts = hosts;
        this->proximity_value = proximity_value;
        this->timestamp = timestamp;
    }

    /**
     * @brief Constructor
     * @param hosts: a pair of hosts
     * @param proximity_value: the proximity value between the pair of hosts
     * @param payload: the message size in bytes
     */
    NetworkProximityComputeAnswerMessage::NetworkProximityComputeAnswerMessage(
            std::pair<std::string, std::string> hosts, double proximity_value, double payload) :
            NetworkProximityMessage(payload) {
        if ((std::get<0>(hosts) == "") || (std::get<1>(hosts) == "")) {
            throw std::invalid_argument(
                    "NetworkProximityComputeAnswerMessage::NetworkProximityComputeAnswerMessage(): Invalid argument");
        }
        this->hosts = hosts;
        this->proximity_value = proximity_value;
    }


    /**
     * @brief Constructor
     * @param daemon: the NetworkProximityDaemon to return the request to
     * @param payload: the message size in bytes
     */
    NextContactDaemonRequestMessage::NextContactDaemonRequestMessage(std::shared_ptr<NetworkProximityDaemon> daemon,
                                                                     double payload) :
            NetworkProximityMessage(payload) {
        if (daemon == nullptr) {
            throw std::invalid_argument(
                    "NextContactDaemonRequestMessage::NextContactDaemonRequestMessage(): Invalid argument");
        }
        this->daemon = daemon;
    }

    /**
     * @brief Constructor
     * @param next_host_to_send: the next host to contact
     * @param next_daemon_to_send: the next daemon to contact
     * @param next_mailbox_to_send: the next mailbox to contact
     * @param payload: the message size in bytes
     */
    NextContactDaemonAnswerMessage::NextContactDaemonAnswerMessage(std::string next_host_to_send,
                                                                   std::shared_ptr<NetworkProximityDaemon> next_daemon_to_send,
                                                                   simgrid::s4u::Mailbox *next_mailbox_to_send, double payload) :
            NetworkProximityMessage(payload) {
        this->next_host_to_send = next_host_to_send;
        this->next_daemon_to_send = next_daemon_to_send;
        this->next_mailbox_to_send = next_mailbox_to_send;
    }


    /**
     * @brief Constructor
     * @param payload: the message size in bytes
     */
    NetworkProximityTransferMessage::NetworkProximityTransferMessage(double payload) :
            NetworkProximityMessage(payload) {
    }

    /**
     * @brief Constructor
     * @param answer_mailbox: the mailbox to return the answer to
     * @param requested_host: the naje of the host whose coordinates are being requested
     * @param payload: the message size in bytes
     */
    CoordinateLookupRequestMessage::CoordinateLookupRequestMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                                                   std::string requested_host, double payload) :
            NetworkProximityMessage(payload), answer_mailbox(answer_mailbox), requested_host(requested_host) {
        if (answer_mailbox == nullptr || requested_host == "") {
            throw std::invalid_argument(
                    "CoordinateLookupRequestMessage::CoordinateLookupRequestMessage(): Invalid argument");
        }
    }

    /**
     * @brief Constructor
     * @param requested_host: the name of the host whose coordinates are being requested
     * @param success: whether coordinates where found or not
     * @param xy_coordinate: the (x,y) coordinate of the host
     * @param timestamp: the timestamp for the coordinates
     * @param payload: the message size in bytes
     */
    CoordinateLookupAnswerMessage::CoordinateLookupAnswerMessage(std::string requested_host,
                                                                 bool success,
                                                                 std::pair<double, double> xy_coordinate,
                                                                 double timestamp,
                                                                 double payload) :
            NetworkProximityMessage(payload) {
        if (requested_host == "") {
            throw std::invalid_argument(
                    "CoordinateLookupAnswerMessage::CoordinateLookupAnswerMessage(): Invalid argument");
        }
        this->requested_host = requested_host;
        this->success = success;
        this->xy_coordinate = xy_coordinate;
        this->timestamp = timestamp;
    }
}
