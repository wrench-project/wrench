/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_DATACONNECTIONMANAGER_H
#define WRENCH_DATACONNECTIONMANAGER_H


#include <string>
#include <deque>
#include <wrench/services/storage/simple/IncomingFile.h>

namespace wrench {

    class DataConnectionManager {

    public:
        DataConnectionManager(unsigned long num_connections);

    private:
        unsigned long num_connections;

        std::deque<std::tuple<std::string, std::unique_ptr<IncomingFile>>> queued_connections;

    };

};


#endif //WRENCH_DATACONNECTIONMANAGER_H
