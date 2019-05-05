/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/storage/simple/NetworkConnectionManager.h"
#include <wrench/simgrid_S4U_util/S4U_PendingCommunication.h>
#include <wrench-dev.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(network_connection_manager, "Log category for Network Connection Manager");


namespace wrench {

    /**
     * @brief Constructor
     * @param max_num_data_connections: maximum number of data connections that can be active at the same time
     */
    NetworkConnectionManager::NetworkConnectionManager(unsigned long max_num_data_connections) {
        this->max_num_data_connections = max_num_data_connections;
    }

    /**
     * @brief Wait for the next network connection to change state
     * @return a network connection that has finished and its status (true: success, false: failure)
     */
    std::pair<std::unique_ptr<NetworkConnection>, bool> NetworkConnectionManager::waitForNetworkConnection() {

        if (this->running_data_connections.empty() and (this->running_control_connection == nullptr)) {
            throw std::runtime_error(
                    "NetworkConnectionManager::waitForNetworkConnection(): there is no running connection!");
        }

        // Create an array of S4U_Pending_Connections
        std::vector<S4U_PendingCommunication *> pending_s4u_comms;
        for (auto it = this->running_data_connections.begin(); it != this->running_data_connections.end(); it++) {
            pending_s4u_comms.push_back((*it)->comm.get());
        }

        if (this->running_control_connection) {
            pending_s4u_comms.push_back(this->running_control_connection->comm.get());
        }

        // Do the wait for any with no timeout
        unsigned long target_index = S4U_PendingCommunication::waitForSomethingToHappen(pending_s4u_comms, -1);

        // Get the relevant data connection
        std::unique_ptr<NetworkConnection> target_connection;
        if (target_index == this->running_data_connections.size()) {
            target_connection = std::move(this->running_control_connection);
            this->running_control_connection = nullptr;
        } else {
            target_connection = std::move(this->running_data_connections[target_index]);
            this->running_data_connections.erase(this->running_data_connections.begin() + target_index);
            // Start other queued connections
            this->startQueuedDataConnections();
        }

        // Get its status
        bool status = not target_connection->hasFailed();


        return {std::move(target_connection), status};

    }

    /**
     * @brief Add a new connection
     * @param connection: a network connection
     */
    void NetworkConnectionManager::addConnection(std::unique_ptr<NetworkConnection> connection) {
        if ((connection->type == NetworkConnection::INCOMING_DATA) or
            (connection->type == NetworkConnection::OUTGOING_DATA)) {
            this->queued_data_connections.push_front(std::move(connection));
            this->startQueuedDataConnections();
        } else {
            if (this->running_control_connection == nullptr) {
                this->running_control_connection = std::move(connection);
                if (not this->running_control_connection->start()) {
                    throw std::runtime_error(
                            "NetworkConnectionManager::addConnection(): Cannot start incoming control connection!");
                }
            }
        }
    }

    /**
     * @brief Start queued connections
     */
    void NetworkConnectionManager::startQueuedDataConnections() {


        while ((this->running_data_connections.size() < this->max_num_data_connections) and
               (not this->queued_data_connections.empty())) {
            // Extract the next connection that should be started
            std::unique_ptr<NetworkConnection> next_connection = std::move(this->queued_data_connections.back());
            this->queued_data_connections.pop_back();
            // Start that connection
            if (not next_connection->start()) {
                continue; // Just give up on that connection (freeing it) if it cannot be started
            }
            // Put that connection into the running list
            this->running_data_connections.push_back(std::move(next_connection));
        }
    }
};