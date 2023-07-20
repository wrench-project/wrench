/**
 * Copyright (c) 2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <utility>
#include "crow.h"
#define toStr(name) (#name)

#ifndef WRENCH_REST_API_H
#define WRENCH_REST_API_H

class REST_API {

public:
    REST_API(crow::SimpleApp &app,
             std::function<void(const crow::request &req)> display_request_function,
             std::shared_ptr<wrench::SimulationController> &sc) : display_request_function(std::move(display_request_function)) {

        // Set up all request handlers (automatically generated code!)
#include "./callback-map.h"

#include "./routes.h"
    }

    void genericRequestHandler(const json &req, crow::response &res, const std::string &api_function) {
        //display_request_function(req);
        //        std::cerr << "API FUNC: " << api_function << "\n";

        json answer;
        try {
            auto request_handler = this->request_handlers[api_function];
            answer = request_handler(req);
            answer["wrench_api_request_success"] = true;
        } catch (std::exception &e) {
            answer["wrench_api_request_success"] = false;
            answer["failure_cause"] = e.what();
        }
        res.set_header("access-control-allow-origin", "*");
        res.body = to_string(answer);
    }

private:
    std::map<std::string, std::function<json(json)>> request_handlers;
    std::function<void(const crow::request &req)> display_request_function;
};

#endif//WRENCH_REST_API_H
