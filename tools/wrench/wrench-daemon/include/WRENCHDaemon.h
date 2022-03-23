/**
 * Copyright (c) 2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_DAEMON_H
#define WRENCH_DAEMON_H

#include "httplib.h"

using httplib::Request;
using httplib::Response;

#include <wrench-dev.h>
#include <map>
#include <vector>
#include <queue>
#include <mutex>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

/**
 * @brief A class that implements a the main wrench-daemon process (in the run() method)
 */
class WRENCHDaemon {

public:
    WRENCHDaemon(bool simulation_logging,
                 bool daemon_logging,
                 int port_number,
                 int sleep_us);

    void run();

private:
    httplib::Server server;

    bool simulation_logging;
    bool daemon_logging;
    int port_number;
    int sleep_us;

    void startSimulation(const Request &req, Response &res);

    static bool isPortTaken(int port);

    static void error_handling(const Request &req, Response &res);
};

#endif// WRENCH_DAEMON_H
