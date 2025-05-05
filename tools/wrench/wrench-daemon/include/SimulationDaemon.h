/**
 * Copyright (c) 2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef SIMULATION_DAEMON_H
#define SIMULATION_DAEMON_H

#include "crow.h"

#include <wrench-dev.h>
#include <map>
#include <vector>
#include <queue>
#include <mutex>

#include <nlohmann/json.hpp>

using json = nlohmann::json;


/**
 * @brief A class that implements a Simulation Daemon process (in the run() method)
 */
class SimulationDaemon {

public:
    SimulationDaemon(bool daemon_logging, int simulation_port_number,
                     std::shared_ptr<wrench::SimulationController> simulation_controller,
                     std::thread &simulation_thread);

    void run();

private:
    crow::SimpleApp app;

    bool daemon_logging;
    int simulation_port_number;
    std::shared_ptr<wrench::SimulationController> simulation_controller;
    std::thread &simulation_thread;

    void displayRequest(const crow::request &req) const;

    void terminateSimulation(const crow::request &req, crow::response &res);

    void alive(const crow::request &req, crow::response &res);
};


#endif// SIMULATION_DAEMON_H
