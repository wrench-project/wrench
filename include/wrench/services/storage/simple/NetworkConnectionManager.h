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

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief An abstraction that manages a pool of network connections
     */
    class NetworkConnectionManager {

    public:
        NetworkConnectionManager(unsigned long max_num_data_connections);

        void addConnection(std::unique_ptr<NetworkConnection> connection);

        std::pair<std::unique_ptr<NetworkConnection>, bool> waitForNetworkConnection();


    private:
        unsigned long max_num_data_connections;

        void startQueuedDataConnections();

        std::deque<std::unique_ptr<NetworkConnection>> queued_data_connections;
        std::vector<std::unique_ptr<NetworkConnection>> running_data_connections;
        std::unique_ptr<NetworkConnection> running_control_connection = nullptr;


    };

    /***********************/
    /** \endcond           */
    /***********************/
};


#endif //WRENCH_NETWORKCONNECTIONMANAGER_H
