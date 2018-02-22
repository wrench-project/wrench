/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "NetworkConnectionManager.h"
#include <wrench/simgrid_S4U_util/S4U_PendingCommunication.h>


namespace wrench {

    NetworkConnectionManager::NetworkConnectionManager(unsigned long num_connections) {
      this->num_connections = num_connections;

    }

    /**
     *
     * @return a NetworkConnection, or nullptr if none is currently running, and true if that connection
     *         has successfully completed, false otherwise
     */
    std::pair<std::unique_ptr<NetworkConnection>, bool> NetworkConnectionManager::waitForNetworkConnection() {

      if (this->running_connections.empty()) {
        return nullptr;
      }

      // Create an array of S4U_Pending_Connections
      std::vector<S4U_PendingCommunication *> pending_s4u_comms;
      for (auto it = this->running_connections.begin(); it < this->running_connections.end(); it++) {
        pending_s4u_comms.push_back((*it)->comm.get());
      }

      // Do the wait for any with no timeout
      unsigned long target_index = S4U_PendingCommunication::waitForSomethingToHappen(pending_s4u_comms, -1);

      // Get the relevant data connection
      std::unique_ptr<NetworkConnection> target_connection = std::move(this->running_connections[target_index]);
      this->running_connections.erase(this->running_connections.begin() + target_index);

      // Get its status
      bool success = not target_connection->hasFailed();
      
      try {
        target_connection->comm->comm_ptr->test();
      } catch (xbt_ex &e) {
        if (e.category == network_error) {
          break;




      return nullptr;

    }

    void NetworkConnectionManager::addConnection(std::unique_ptr<NetworkConnection> connection) {
      this->queued_connections.push_front(std::move(connection));
    }

    void NetworkConnectionManager::startQueuedConnections() {

      while ((this->running_connections.size() < this->num_connections) and
              (not this->queued_connections.empty())) {
        // Extract the next connection that should be started
        std::unique_ptr<NetworkConnection> next_connection = std::move(this->queued_connections.back());
        this->queued_connections.pop_back();
        // Start that connection
        if (not next_connection->start()) {
          continue; // Just give up on that connection (freeing it) if it cannot be started
        }
        // Put that connection into the running list
        this->running_connections.push_back(std::move(next_connection));
      }
    }
};