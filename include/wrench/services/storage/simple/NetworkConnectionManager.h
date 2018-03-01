/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_NETWORKCONNECTIONMANAGER_H
#define WRENCH_NETWORKCONNECTIONMANAGER_H


#include <string>
#include <deque>
#include "wrench/services/storage/simple/NetworkConnection.h"

namespace wrench {

    class NetworkConnectionManager {

    public:
        NetworkConnectionManager(unsigned long num_connections);

        void addConnection(std::unique_ptr<NetworkConnection> connection);

        std::pair<std::unique_ptr<NetworkConnection>, bool> waitForNetworkConnection();


    private:
        unsigned long num_connections;

        void startQueuedConnections();

        std::deque<std::unique_ptr<NetworkConnection>> queued_connections;
        std::vector<std::unique_ptr<NetworkConnection>> running_connections;

    };

};


#endif //WRENCH_NETWORKCONNECTIONMANAGER_H
