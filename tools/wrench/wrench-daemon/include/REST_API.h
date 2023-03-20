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
                    json req_json = json::parse(req.body);
                    crow::response res;
                    this->genericRequestHandler(req_json, res, "startSimulation");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/getTime").methods(crow::HTTPMethod::GET)
                ([this](const crow::request& req, const std::string& simid){
                    json req_json = json::parse(req.body);
                    req_json[toStr(simid)] = simid;
                    crow::response res;
                    this->genericRequestHandler(req_json, res, "getTime");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/advanceTime").methods(crow::HTTPMethod::PUT)
                ([this](const crow::request& req, const std::string& simid){
                    json req_json = json::parse(req.body);
                    req_json[toStr(simid)] = simid;
                    crow::response res;
                    this->genericRequestHandler(req_json, res, "advanceTime");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/createTask").methods(crow::HTTPMethod::PUT)
                ([this](const crow::request& req, const std::string& simid){
                    json req_json = json::parse(req.body);
                    req_json[toStr(simid)] = simid;
                    crow::response res;
                    this->genericRequestHandler(req_json, res, "createTask");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/waitForNextSimulationEvent").methods(crow::HTTPMethod::GET)
                ([this](const crow::request& req, const std::string& simid){
                    json req_json = json::parse(req.body);
                    req_json[toStr(simid)] = simid;
                    crow::response res;
                    this->genericRequestHandler(req_json, res, "waitForNextSimulationEvent");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/simulationEvents").methods(crow::HTTPMethod::GET)
                ([this](const crow::request& req, const std::string& simid){
                    json req_json = json::parse(req.body);
                    req_json[toStr(simid)] = simid;
                    crow::response res;
                    this->genericRequestHandler(req_json, res, "getSimulationEvents");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/hostnames").methods(crow::HTTPMethod::GET)
                ([this](const crow::request& req, const std::string& simid){
                    json req_json = json::parse(req.body);
                    req_json[toStr(simid)] = simid;
                    crow::response res;
                    this->genericRequestHandler(req_json, res, "getAllHostnames");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/tasks/<string>/inputFiles").methods(crow::HTTPMethod::GET)
                ([this](const crow::request& req, const std::string& simid, const std::string& tid){
                    json req_json = json::parse(req.body);
                    req_json[toStr(simid)] = simid;
                    req_json[toStr(tid)] = tid;
                    crow::response res;
                    this->genericRequestHandler(req_json, res, "getTaskInputFiles");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/tasks/<string>/addInputFile").methods(crow::HTTPMethod::PUT)
                ([this](const crow::request& req, const std::string& simid, const std::string& tid){
                    json req_json = json::parse(req.body);
                    req_json[toStr(simid)] = simid;
                    req_json[toStr(tid)] = tid;
                    crow::response res;
                    this->genericRequestHandler(req_json, res, "addInputFile");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/tasks/<string>/addOutputFile").methods(crow::HTTPMethod::PUT)
                ([this](const crow::request& req, const std::string& simid, const std::string& tid){
                    json req_json = json::parse(req.body);
                    req_json[toStr(simid)] = simid;
                    req_json[toStr(tid)] = tid;
                    crow::response res;
                    this->genericRequestHandler(req_json, res, "addOutputFile");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/inputFiles").methods(crow::HTTPMethod::GET)
                ([this](const crow::request& req, const std::string& simid){
                    json req_json = json::parse(req.body);
                    req_json[toStr(simid)] = simid;
                    crow::response res;
                    this->genericRequestHandler(req_json, res, "getInputFiles");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/addFile").methods(crow::HTTPMethod::PUT)
                ([this](const crow::request& req, const std::string& simid){
                    json req_json = json::parse(req.body);
                    req_json[toStr(simid)] = simid;
                    crow::response res;
                    this->genericRequestHandler(req_json, res, "addFile");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/files/<string>/size").methods(crow::HTTPMethod::GET)
                ([this](const crow::request& req, const std::string& simid, const std::string& file_id){
                    json req_json = json::parse(req.body);
                    req_json[toStr(simid)] = simid;
                    req_json[toStr(file_id)] = file_id;
                    crow::response res;
                    this->genericRequestHandler(req_json, res, "fileGetSize");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/jobs/<string>/tasks").methods(crow::HTTPMethod::GET)
                ([this](const crow::request& req, const std::string& simid, const std::string& job_name){
                    json req_json = json::parse(req.body);
                    req_json[toStr(simid)] = simid;
                    req_json[toStr(job_name)] = job_name;
                    crow::response res;
                    this->genericRequestHandler(req_json, res, "standardJobGetTasks");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/tasks/<string>/taskGetFlops").methods(crow::HTTPMethod::GET)
                ([this](const crow::request& req, const std::string& simid, const std::string& name){
                    json req_json = json::parse(req.body);
                    req_json[toStr(simid)] = simid;
                    req_json[toStr(name)] = name;
                    crow::response res;
                    this->genericRequestHandler(req_json, res, "taskGetFlops");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/tasks/<string>/taskGetMinNumCores").methods(crow::HTTPMethod::GET)
                ([this](const crow::request& req, const std::string& simid, const std::string& name){
                    json req_json = json::parse(req.body);
                    req_json[toStr(simid)] = simid;
                    req_json[toStr(name)] = name;
                    crow::response res;
                    this->genericRequestHandler(req_json, res, "taskGetMinNumCores");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/tasks/<string>/taskGetMaxNumCores").methods(crow::HTTPMethod::GET)
                ([this](const crow::request& req, const std::string& simid, const std::string& name){
                    json req_json = json::parse(req.body);
                    req_json[toStr(simid)] = simid;
                    req_json[toStr(name)] = name;
                    crow::response res;
                    this->genericRequestHandler(req_json, res, "taskGetMaxNumCores");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/tasks/<string>/taskGetMemory").methods(crow::HTTPMethod::GET)
                ([this](const crow::request& req, const std::string& simid, const std::string& name){
                    json req_json = json::parse(req.body);
                    req_json[toStr(simid)] = simid;
                    req_json[toStr(name)] = name;
                    crow::response res;
                    this->genericRequestHandler(req_json, res, "taskGetMemory");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/createStandardJob").methods(crow::HTTPMethod::PUT)
                ([this](const crow::request& req, const std::string& simid){
                    json req_json = json::parse(req.body);
                    req_json[toStr(simid)] = simid;
                    crow::response res;
                    this->genericRequestHandler(req_json, res, "createStandardJob");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/<string>/submit").methods(crow::HTTPMethod::POST)
                ([this](const crow::request& req, const std::string& simid, const std::string& job_name){
                    json req_json = json::parse(req.body);
                    req_json[toStr(simid)] = simid;
                    req_json[toStr(job_name)] = job_name;
                    crow::response res;
                    this->genericRequestHandler(req_json, res, "submitStandardJob");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/addBareMetalComputeService").methods(crow::HTTPMethod::POST)
                ([this](const crow::request& req, const std::string& simid){
                    json req_json = json::parse(req.body);
                    req_json[toStr(simid)] = simid;
                    crow::response res;
                    this->genericRequestHandler(req_json, res, "addBareMetalComputeService");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/addSimpleStorageService").methods(crow::HTTPMethod::POST)
                ([this](const crow::request& req, const std::string& simid){
                    json req_json = json::parse(req.body);
                    req_json[toStr(simid)] = simid;
                    crow::response res;
                    this->genericRequestHandler(req_json, res, "addSimpleStorageService");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/addFileRegistryService").methods(crow::HTTPMethod::POST)
                ([this](const crow::request& req, const std::string& simid){
                    json req_json = json::parse(req.body);
                    req_json[toStr(simid)] = simid;
                    crow::response res;
                    this->genericRequestHandler(req_json, res, "addFileRegistryService");
                    return res;
                });

        CROW_ROUTE(app, "/simulation/<string>/<string>/createFileCopy").methods(crow::HTTPMethod::POST)
                ([this](const crow::request& req, const std::string& simid, const std::string& storage_service_name){
                    json req_json = json::parse(req.body);
                    req_json[toStr(simid)] = simid;
                    req_json[toStr(storage_service_name)] = storage_service_name;
                    crow::response res;
                    this->genericRequestHandler(req_json, res, "createFileCopyAtStorageService");
                    return res;
                });
    }

    void genericRequestHandler(const json &req, crow::response &res, const std::string& api_function) {
        //display_request_function(req);

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
