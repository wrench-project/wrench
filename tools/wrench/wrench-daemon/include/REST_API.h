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
#include "./REST_API_generated_code.h"

        // Set up all post request handlers
        CROW_ROUTE(app, "/simulation/startSimulation").methods(crow::HTTPMethod::POST)
                ([this](const crow::request& req){
                    json result = json::parse(req.body);
                    crow::response res;
                    this->genericRequestHandler(req, res, "startSimulation");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/getTime").methods(crow::HTTPMethod::GET)
                ([this](const crow::request& req, const std::string& simid){
                    json result = json::parse(req.body);
                    result[toStr(simid)] = simid;
                    crow::response res;
                    this->genericRequestHandler(req, res, "getTime");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/advanceTime").methods(crow::HTTPMethod::PUT)
                ([this](const crow::request& req, const std::string& simid){
                    json result = json::parse(req.body);
                    result[toStr(simid)] = simid;
                    crow::response res;
                    this->genericRequestHandler(req, res, "advanceTime");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/createTask").methods(crow::HTTPMethod::PUT)
                ([this](const crow::request& req, const std::string& simid){
                    json result = json::parse(req.body);
                    result[toStr(simid)] = simid;
                    crow::response res;
                    this->genericRequestHandler(req, res, "createTask");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/waitForNextSimulationEvent").methods(crow::HTTPMethod::GET)
                ([this](const crow::request& req, const std::string& simid){
                    json result = json::parse(req.body);
                    result[toStr(simid)] = simid;
                    crow::response res;
                    this->genericRequestHandler(req, res, "waitForNextSimulationEvent");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/simulationEvents").methods(crow::HTTPMethod::GET)
                ([this](const crow::request& req, const std::string& simid){
                    json result = json::parse(req.body);
                    result[toStr(simid)] = simid;
                    crow::response res;
                    this->genericRequestHandler(req, res, "getSimulationEvents");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/hostnames").methods(crow::HTTPMethod::GET)
                ([this](const crow::request& req, const std::string& simid){
                    json result = json::parse(req.body);
                    result[toStr(simid)] = simid;
                    crow::response res;
                    this->genericRequestHandler(req, res, "getAllHostnames");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/tasks/<string>/inputFiles").methods(crow::HTTPMethod::GET)
                ([this](const crow::request& req, const std::string& simid, const std::string& tid){
                    json result = json::parse(req.body);
                    result[toStr(simid)] = simid;
                    result[toStr(tid)] = tid;
                    crow::response res;
                    this->genericRequestHandler(req, res, "getTaskInputFiles");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/tasks/<string>/addInputFile").methods(crow::HTTPMethod::PUT)
                ([this](const crow::request& req, const std::string& simid, const std::string& tid){
                    json result = json::parse(req.body);
                    result[toStr(simid)] = simid;
                    result[toStr(tid)] = tid;
                    crow::response res;
                    this->genericRequestHandler(req, res, "addInputFile");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/tasks/<string>/addOutputFile").methods(crow::HTTPMethod::PUT)
                ([this](const crow::request& req, const std::string& simid, const std::string& tid){
                    json result = json::parse(req.body);
                    result[toStr(simid)] = simid;
                    result[toStr(tid)] = tid;
                    crow::response res;
                    this->genericRequestHandler(req, res, "addOutputFile");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/inputFiles").methods(crow::HTTPMethod::GET)
                ([this](const crow::request& req, const std::string& simid){
                    json result = json::parse(req.body);
                    result[toStr(simid)] = simid;
                    crow::response res;
                    this->genericRequestHandler(req, res, "getInputFiles");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/addFile").methods(crow::HTTPMethod::PUT)
                ([this](const crow::request& req, const std::string& simid){
                    json result = json::parse(req.body);
                    result[toStr(simid)] = simid;
                    crow::response res;
                    this->genericRequestHandler(req, res, "addFile");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/files/<string>/size").methods(crow::HTTPMethod::GET)
                ([this](const crow::request& req, const std::string& simid, const std::string& file_id){
                    json result = json::parse(req.body);
                    result[toStr(simid)] = simid;
                    result[toStr(file_id)] = file_id;
                    crow::response res;
                    this->genericRequestHandler(req, res, "fileGetSize");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/jobs/<string>/tasks").methods(crow::HTTPMethod::GET)
                ([this](const crow::request& req, const std::string& simid, const std::string& jid){
                    json result = json::parse(req.body);
                    result[toStr(simid)] = simid;
                    result[toStr(jid)] = jid;
                    crow::response res;
                    this->genericRequestHandler(req, res, "standardJobGetTasks");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/tasks/<string>/taskGetFlops").methods(crow::HTTPMethod::GET)
                ([this](const crow::request& req, const std::string& simid, const std::string& tid){
                    json result = json::parse(req.body);
                    result[toStr(simid)] = simid;
                    result[toStr(tid)] = tid;
                    crow::response res;
                    this->genericRequestHandler(req, res, "taskGetFlops");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/tasks/<string>/taskGetMinNumCores").methods(crow::HTTPMethod::GET)
                ([this](const crow::request& req, const std::string& simid, const std::string& tid){
                    json result = json::parse(req.body);
                    result[toStr(simid)] = simid;
                    result[toStr(tid)] = tid;
                    crow::response res;
                    this->genericRequestHandler(req, res, "taskGetMinNumCores");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/tasks/<string>/taskGetMaxNumCores").methods(crow::HTTPMethod::GET)
                ([this](const crow::request& req, const std::string& simid, const std::string& tid){
                    json result = json::parse(req.body);
                    result[toStr(simid)] = simid;
                    result[toStr(tid)] = tid;
                    crow::response res;
                    this->genericRequestHandler(req, res, "taskGetMaxNumCores");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/tasks/<string>/taskGetMemory").methods(crow::HTTPMethod::GET)
                ([this](const crow::request& req, const std::string& simid, const std::string& tid){
                    json result = json::parse(req.body);
                    result[toStr(simid)] = simid;
                    result[toStr(tid)] = tid;
                    crow::response res;
                    this->genericRequestHandler(req, res, "taskGetMemory");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/createStandardJob").methods(crow::HTTPMethod::PUT)
                ([this](const crow::request& req, const std::string& simid){
                    json result = json::parse(req.body);
                    result[toStr(simid)] = simid;
                    crow::response res;
                    this->genericRequestHandler(req, res, "createStandardJob");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/<string>/submit").methods(crow::HTTPMethod::POST)
                ([this](const crow::request& req, const std::string& simid, const std::string& jid){
                    json result = json::parse(req.body);
                    result[toStr(simid)] = simid;
                    result[toStr(jid)] = jid;
                    crow::response res;
                    this->genericRequestHandler(req, res, "submitStandardJob");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/addBareMetalComputeService").methods(crow::HTTPMethod::POST)
                ([this](const crow::request& req, const std::string& simid){
                    json result = json::parse(req.body);
                    result[toStr(simid)] = simid;
                    crow::response res;
                    this->genericRequestHandler(req, res, "addBareMetalComputeService");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/addSimpleStorageService").methods(crow::HTTPMethod::POST)
                ([this](const crow::request& req, const std::string& simid){
                    json result = json::parse(req.body);
                    result[toStr(simid)] = simid;
                    crow::response res;
                    this->genericRequestHandler(req, res, "addSimpleStorageService");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/addFileRegistryService").methods(crow::HTTPMethod::POST)
                ([this](const crow::request& req, const std::string& simid){
                    json result = json::parse(req.body);
                    result[toStr(simid)] = simid;
                    crow::response res;
                    this->genericRequestHandler(req, res, "addFileRegistryService");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/<string>/createFileCopy").methods(crow::HTTPMethod::POST)
                ([this](const crow::request& req, const std::string& simid, const std::string& storage_service_id){
                    json result = json::parse(req.body);
                    result[toStr(simid)] = simid;
                    result[toStr(storage_service_id)] = storage_service_id;
                    crow::response res;
                    this->genericRequestHandler(req, res, "createFileCopyAtStorageService");
                    return res;
                });
    }

    void genericRequestHandler(const crow::request &req, crow::response &res, const std::string& api_function) {
        display_request_function(req);

        json answer;
        try {
            auto request_handler = this->request_handlers[api_function];
            answer = request_handler(json::parse(req.body));
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
