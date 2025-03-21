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

#include "crow.h"

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
                 unsigned long num_commports,
                 int port_number,
                 int fixed_simulation_port_number,
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

private:
    crow::SimpleApp app;

    static std::vector<std::string> allowed_origins;

    bool simulation_logging;
    bool daemon_logging;
    unsigned long num_commports;
    int port_number;
    int fixed_simulation_port_number;
    std::string allowed_origin;
    int sleep_us;

    void startSimulation(const crow::request &req, crow::response &res);

    static bool isPortTaken(int port);

};

#endif// WRENCH_DAEMON_H
