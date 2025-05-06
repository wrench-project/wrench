/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/network_proximity/NetworkProximityMessage.h"

#include <utility>

namespace wrench {
    /**
     * @brief Constructor
     * @param payload: the message size in bytes
     */
    NetworkProximityMessage::NetworkProximityMessage(sg_size_t payload) : ServiceMessage(payload) {
    }


    /**
     * @brief Constructor
     * @param answer_commport: the commport to which the answer message should be sent
     * @param hosts: the pair of hosts to look up
     * @param payload: the message size in bytes
     */
    NetworkProximityLookupRequestMessage::NetworkProximityLookupRequestMessage(S4U_CommPort *answer_commport,
                                                                               std::pair<std::string, std::string> hosts,
                                                                               sg_size_t payload) : NetworkProximityMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((answer_commport == nullptr) || (std::get<0>(hosts).empty()) || (std::get<1>(hosts).empty())) {
            throw std::invalid_argument(
                    "NetworkProximityLookupRequestMessage::NetworkProximityLookupRequestMessage(): Invalid argument");
        }
#endif
        this->answer_commport = answer_commport;
        this->hosts = std::move(hosts);
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
                                                                             sg_size_t payload) : NetworkProximityMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((std::get<0>(hosts).empty()) || (std::get<1>(hosts).empty())) {
            throw std::invalid_argument(
                    "NetworkProximityLookupAnswerMessage::NetworkProximityLookupAnswerMessage(): Invalid argument");
        }
#endif
        this->hosts = std::move(hosts);
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
            std::pair<std::string, std::string> hosts, double proximity_value, sg_size_t payload) : NetworkProximityMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((std::get<0>(hosts).empty()) || (std::get<1>(hosts).empty())) {
            throw std::invalid_argument(
                    "NetworkProximityComputeAnswerMessage::NetworkProximityComputeAnswerMessage(): Invalid argument");
        }
#endif
        this->hosts = std::move(hosts);
        this->proximity_value = proximity_value;
    }


    /**
     * @brief Constructor
     * @param daemon: the NetworkProximitySenderDaemon to return the request to
     * @param payload: the message size in bytes
     */
    NextContactDaemonRequestMessage::NextContactDaemonRequestMessage(std::shared_ptr<NetworkProximitySenderDaemon> daemon,
                                                                     sg_size_t payload) : NetworkProximityMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if (daemon == nullptr) {
            throw std::invalid_argument(
                    "NextContactDaemonRequestMessage::NextContactDaemonRequestMessage(): Invalid argument");
        }
#endif
        this->daemon = std::move(daemon);
    }

    /**
     * @brief Constructor
     * @param next_host_to_send: the next host to contact
     * @param next_daemon_to_send: the next daemon to contact
     * @param next_commport_to_send: the next commport to contact
     * @param payload: the message size in bytes
     */
    NextContactDaemonAnswerMessage::NextContactDaemonAnswerMessage(std::string next_host_to_send,
                                                                   std::shared_ptr<NetworkProximityReceiverDaemon> next_daemon_to_send,
                                                                   S4U_CommPort *next_commport_to_send, sg_size_t payload) : NetworkProximityMessage(payload) {
        this->next_host_to_send = std::move(next_host_to_send);
        this->next_daemon_to_send = std::move(next_daemon_to_send);
        this->next_commport_to_send = next_commport_to_send;
    }


    /**
     * @brief Constructor
     * @param payload: the message size in bytes
     */
    NetworkProximityTransferMessage::NetworkProximityTransferMessage(sg_size_t payload) : NetworkProximityMessage(payload) {
    }

    /**
     * @brief Constructor
     * @param answer_commport: the commport to return the answer to
     * @param requested_host: the name of the host whose coordinates are being requested
     * @param payload: the message size in bytes
     */
    CoordinateLookupRequestMessage::CoordinateLookupRequestMessage(S4U_CommPort *answer_commport,
                                                                   std::string requested_host, sg_size_t payload) : NetworkProximityMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if (answer_commport == nullptr || requested_host.empty()) {
            throw std::invalid_argument(
                    "CoordinateLookupRequestMessage::CoordinateLookupRequestMessage(): Invalid argument");
        }
#endif
        this->answer_commport = answer_commport;
        this->requested_host = std::move(requested_host);
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
                                                                 const std::pair<double, double>& xy_coordinate,
                                                                 double timestamp,
                                                                 sg_size_t payload) : NetworkProximityMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if (requested_host.empty()) {
            throw std::invalid_argument(
                    "CoordinateLookupAnswerMessage::CoordinateLookupAnswerMessage(): Invalid argument");
        }
#endif
        this->requested_host = std::move(requested_host);
        this->success = success;
        this->xy_coordinate = xy_coordinate;
        this->timestamp = timestamp;
    }
}// namespace wrench
