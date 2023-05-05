/**
 * Copyright (c) 2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <string>
#include <utility>
#include <vector>
#include <thread>
#include <boost/program_options.hpp>
#include <nlohmann/json.hpp>

#include "SimulationController.h"
#include "SimulationDaemon.h"
#include "REST_API.h"

using json = nlohmann::json;

/**
 * @brief The Simulation Daemon's "main" method
 */
void SimulationDaemon::run() {
    // Set up GET request handler for the (likely useless) "alive" path
    CROW_ROUTE(app, "/simulation/<string>/alive").methods("GET"_method)([this](const crow::request &req, const string &simid) {
        crow::response res;
        this->alive(req, res);
        return res;
    });
    // Set up POST request handler for terminating simulation
    CROW_ROUTE(app, "/simulation/<string>/terminateSimulation").methods("POST"_method)([this](const crow::request &req, const string &simid) {
        crow::response res;
        this->terminateSimulation(req, res);
        return res;
    });
    // Set up ALL POST request handlers for API calls

    REST_API rest_api(
            this->app,
            [this](const crow::request &req) { this->displayRequest(req); },
            this->simulation_controller);

    if (daemon_logging) {
        std::cerr << " PID " << getpid() << " listening on port " << simulation_port_number << "\n";
    }

    // Test ToDo: Investigate multi-threads later
    app.port(simulation_port_number).run();

    //    exit(0);
}

/**
 * @brief Constructor
 *
 * @param daemon_logging true if daemon logging should be printed
 * @param simulation_port_number port number on which this daemon is listening
 * @param simulation_controller the simulation execution_controller
 * @param simulation_thread the simulation thread
 */
SimulationDaemon::SimulationDaemon(
        bool daemon_logging,
        int simulation_port_number,
        std::shared_ptr<wrench::SimulationController> simulation_controller,
        std::thread &simulation_thread) : daemon_logging(daemon_logging),
                                          simulation_port_number(simulation_port_number),
                                          simulation_controller(std::move(std::move(std::move(simulation_controller)))),
                                          simulation_thread(simulation_thread) {
}

/**
 * @brief Helper method for logging
 * @param req HTTP request
 */
void SimulationDaemon::displayRequest(const crow::request &req) const {
    unsigned long max_line_length = 120;
    if (daemon_logging) {
        std::cerr << req.url << " " << req.body.substr(0, max_line_length)
                  << (req.body.length() > max_line_length ? "..." : "") << std::endl;
    }
}

void SimulationDaemon::alive(const crow::request &req, crow::response &res) {
    SimulationDaemon::displayRequest(req);

    // Create json answer
    json answer;
    answer["wrench_api_request_success"] = true;
    answer["alive"] = true;

    res.set_header("access-control-allow-origin", "*");
    res.body = to_string(answer);
}

/***********************
 ** ALL PATH HANDLERS **
 ***********************/

/**
 * @brief REST API Handler
 * @param req HTTP request
 * @param res HTTP response
 *
 * BEGIN_REST_API_DOCUMENTATION
 * {
 *   "REST_func": "terminateSimulation",
 *   "documentation":
 *     {
 *       "purpose": "Terminate the simulation",
 *       "json_input": {
 *       },
 *       "json_output": {
 *       }
 *     }
 * }
 * END_REST_API_DOCUMENTATION
 */
void SimulationDaemon::terminateSimulation(const crow::request &req, crow::response &res) {
    displayRequest(req);

    // Stop the simulation thread and wait for it to have stopped
    simulation_controller->stopSimulation();
    simulation_thread.join();

    // Create an json answer
    json answer;
    answer["wrench_api_request_success"] = true;

    res.set_header("access-control-allow-origin", "*");
    res.body = to_string(answer);

    app.stop();
    if (daemon_logging) {
        std::cerr << " PID " << getpid() << " terminated.\n";
    }
    exit(1);
}
