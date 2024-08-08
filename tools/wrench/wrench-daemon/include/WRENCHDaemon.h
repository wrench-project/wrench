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
#include "crow.h"

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
                 const std::string &allowed_origin,
                 int sleep_us);

    void run();

    static void allow_origin(crow::response &res) {
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        for (auto const &origin: allowed_origins) {
            res.set_header("Access-Control-Allow-Origin", origin);
        }
    }

    static void allow_origin(Response &res) {
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        for (auto const &origin: allowed_origins) {
            res.set_header("Access-Control-Allow-Origin", origin);
        }
    }

private:
    httplib::Server server;

    static std::vector<std::string> allowed_origins;

    bool simulation_logging;
    bool daemon_logging;
    int port_number;
    std::string allowed_origin;
    int sleep_us;

    void startSimulation(const Request &req, Response &res);

    static bool isPortTaken(int port);

    static void error_handling(const Request &req, Response &res);
};

#endif// WRENCH_DAEMON_H
