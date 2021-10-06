/**
 * Copyright (c) 2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SIMULATION_LAUNCHER_H
#define WRENCH_SIMULATION_LAUNCHER_H

#include "SimulationController.h"
#include <unistd.h>
#include <nlohmann/json.hpp>

/**
 * @brief A class that handles all simulation changes and holds all simulation state.
 */
class SimulationLauncher {

public:
    ~SimulationLauncher() = default;

    void createSimulation(bool full_log,
                          const std::string &platform_xml,
                          const std::string &controller_host,
                          int sleep_us);

    void launchSimulation();

    bool launchError() const { return this->launch_error; }

    std::string launchErrorMessage() const { return this->launch_error_message; }

    std::shared_ptr <wrench::SimulationController> getController() const { return this->controller; }

private:
    wrench::Simulation simulation;
    std::shared_ptr <wrench::SimulationController> controller;
    bool launch_error = false;
    std::string launch_error_message;

};

#endif // WRENCH_SIMULATION_LAUNCHER_H
