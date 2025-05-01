/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** An execution controller implementation that repeatedly submits two compute actions at a time
 ** to the bare-metal compute service
 **/

#include <iostream>
#include <utility>

#include "ServerlessExampleExecutionController.h"

#define GFLOP (1000.0 * 1000.0 * 1000.0)
#define MB (1000000ULL)

WRENCH_LOG_CATEGORY(custom_controller, "Log category for ServerlessExampleExecutionController");

namespace wrench {

    class MyFunctionInput: public FunctionInput {
    public:
        MyFunctionInput(int x1, int x2) : x1_(x1), x2_(x2) {}

        int x1_;
        int x2_;
    };
    

    /**
     * @brief Constructor, which calls the super constructor
     *
     * @param compute_service
     * @param storage_service
     * @param compute_service: a set of compute services available to run actions
     * @param storage_service: a set of storage services available to store data files
     * @param hostname: the name of the host on which to start the WMS
     */
    ServerlessExampleExecutionController::ServerlessExampleExecutionController(const std::shared_ptr<ServerlessComputeService>& compute_service,
                                                                               const std::shared_ptr<SimpleStorageService>& storage_service,
                                                                               const std::string &hostname, const int numInvocations) : ExecutionController(hostname, "me"),
                                                                                                              compute_service(compute_service),
                                                                                                              storage_service(storage_service), numInvocations(numInvocations) {
    }

    /**
     * @brief main method of the TwoActionsAtATimeExecutionController daemon
     *
     * @return 0 on completion
     *
     */
    int ServerlessExampleExecutionController::main() {
        WRENCH_INFO("ServerlessExampleExecutionController started");

        struct FuncDef {
            std::string name;
            size_t imageMB, codeMB;
            double runtimeSec;
        };
        std::vector<FuncDef> defs = {
            { "F1",   100,  10,  5.0 },   // light
            { "F2",  1000, 100, 50.0 },   // heavy
            { "F3",   200,  20, 20.0 },   // medium
            { "F4",   1000,  50, 800.0 },    // extra‐heavy
            // { "F5",   5000,  50, 1000.0 },    // extra‐heavy
            // { "F6",   1000,  50, 100.0 },    // heavy
            // { "F7",   1000,  50, 200.0 },    // heavy
            // { "F8",   1000,  50, 300.0 },    // heavy
            // { "F9",   1000,  50, 400.0 },    // heavy
            // { "F10",   1000, 50, 500.0 },    // heavy
            // { "F11",   1000, 50, 600.0 },    // heavy
            // { "F12",   1000, 50, 700.0 },    // heavy
            // { "F13",   1000, 50, 800.0 },    // heavy
            // { "F14",   1000, 50, 900.0 },    // heavy
            // { "F15",   1000, 50, 1000.0 }     // heavy
        };


        // Register a function
        auto function_manager = this->createFunctionManager();
        std::vector<std::shared_ptr<Function>> regs;
        regs.reserve(defs.size());

        for (auto& d : defs) {
            // create files
            auto image = Simulation::addFile("img_" + d.name, d.imageMB * MB);
            auto code  = Simulation::addFile("code_" + d.name, d.codeMB * MB);
            auto imgLoc  = FileLocation::LOCATION(storage_service, image);
            auto codeLoc = FileLocation::LOCATION(storage_service, code);
            StorageService::createFileAtLocation(imgLoc);
            StorageService::createFileAtLocation(codeLoc);

            // build the lambda
            auto lambda = [d](std::shared_ptr<FunctionInput>, const std::shared_ptr<StorageService>&) -> std::string {
                Simulation::sleep(d.runtimeSec);
                return "Completed " + d.name;
            };

            // register
            auto f = function_manager->createFunction(d.name, lambda, imgLoc, codeLoc);
            function_manager->registerFunction(f, compute_service,
                                 d.runtimeSec,
                                 /*diskIn*/2000 * MB,
                                 /*diskOut*/8000 * MB,
                                 /*netIn*/10 * MB,
                                 /*netOut*/1 * MB);
            regs.push_back(f);
            WRENCH_INFO("Registered %s (%.1f s)", d.name.c_str(), d.runtimeSec);
        }

        std::vector<std::shared_ptr<Invocation>> invocations;
        invocations.reserve(numInvocations);

        std::vector<double> weights;
        for (auto &f : regs) {
        // give F1 twice the chance:
            weights.push_back( f->getName()=="F1" ? 2.0 : 1.0 );
        }

        std::mt19937 gen{42};
        std::discrete_distribution<> dist(weights.begin(), weights.end());

        for (int i = 0; i < numInvocations; ++i) {
        size_t idx = dist(gen);
        auto &choice = regs[idx];
        auto inv = function_manager->invokeFunction(choice, compute_service,
            std::make_shared<MyFunctionInput>(i, i+1));
        invocations.push_back(inv);
        }

        // for (int i = 0; i < numInvocations; ++i) {
        //     size_t idx = static_cast<size_t>(i) % regs.size();
        //     auto& choice = regs[idx];
        //     WRENCH_INFO("Submitting invocation %d -> %s",
        //                 i+1,
        //                 choice->getName().c_str());
        //     auto inv = function_manager->invokeFunction(
        //         choice,
        //         compute_service,
        //         std::make_shared<MyFunctionInput>(i, i+1)
        //     );
        //     wrench::Simulation::sleep(0.5); // simulate some delay
        //     // auto inv2 = function_manager->invokeFunction(
        //     //     choice,
        //     //     compute_service,
        //     //     std::make_shared<MyFunctionInput>(i, i+1)
        //     // );
        //     invocations.push_back(inv);
        //     // invocations.push_back(inv2);
        // }

        // 4) Wait for completion and log makespan
        WRENCH_INFO("Waiting for all %zu invocations...", invocations.size());
        function_manager->wait_all(invocations);



        // std::function lambda_1 = [](std::shared_ptr<FunctionInput> input,
        //     const std::shared_ptr<StorageService>& service) -> std::string {
        // // If possible, log the type info (make sure the object is valid)

        //     auto real_input = std::dynamic_pointer_cast<MyFunctionInput>(input);
        //     if (!real_input) {
        //     WRENCH_INFO("Invalid FunctionInput type: expected MyFunctionInput");
        //     return "Error: invalid input type";
        //     }
        //     WRENCH_INFO("I AM USER CODE FOR FUNCTION 1");
        //     Simulation::sleep(5);
        //     return "Processed: " + std::to_string(real_input->x1_ + real_input->x2_);
        // };

        std::function lambda_2 = [](std::shared_ptr<FunctionInput> input,
            const std::shared_ptr<StorageService>& service) -> std::string {
            // If possible, log the type info (make sure the object is valid)

            auto real_input = std::dynamic_pointer_cast<MyFunctionInput>(input);
            if (!real_input) {
                WRENCH_INFO("Invalid FunctionInput type: expected MyFunctionInput");
                return "Error: invalid input type";
            }
            WRENCH_INFO("I AM USER CODE FOR FUNCTION 2");
            Simulation::sleep(50);
            return "Processed: " + std::to_string(real_input->x1_ + real_input->x2_);
        };


        // auto image_file_1 = Simulation::addFile("input_file_1", 100 * MB);
        // auto source_code_1 = Simulation::addFile("source_code_1", 10 * MB);
        // auto image_location_1 = FileLocation::LOCATION(this->storage_service, image_file_1);
        // auto code_location_1 = FileLocation::LOCATION(this->storage_service, source_code_1);
        // StorageService::createFileAtLocation(image_location_1);
        // StorageService::createFileAtLocation(code_location_1);

        auto image_file_2 = Simulation::addFile("input_file_2", 1000 * MB);
        auto source_code_2 = Simulation::addFile("source_code_2", 100 * MB);
        auto image_location_2 = FileLocation::LOCATION(this->storage_service, image_file_2);
        auto code_location_2 = FileLocation::LOCATION(this->storage_service, source_code_2);
        StorageService::createFileAtLocation(image_location_2);
        StorageService::createFileAtLocation(code_location_2);

        // auto function1 = function_manager->createFunction("Function 1", lambda_1, image_location_1, code_location_1);

        // WRENCH_INFO("Registering function 1");
        // function_manager->registerFunction(function1, this->compute_service, 5, 2000 * MB, 8000 * MB, 10 * MB, 1 * MB);
        // WRENCH_INFO("Function 1 registered");

        // // Try to register the same function name
        // WRENCH_INFO("Trying to register function 1 again");
        // try {
        //     function_manager->registerFunction(function1, this->compute_service, 5, 2000 * MB, 8000 * MB, 10 * MB, 1 * MB);
        // } catch (ExecutionException& expected) {
        //     WRENCH_INFO("As expected, got exception: %s", expected.getCause()->toString().c_str());

        // }

        auto function2 = function_manager->createFunction("Function 2", lambda_2, image_location_2, code_location_2);
        // // Try to invoke a function that is not registered yet
        // WRENCH_INFO("Invoking a non-registered function");
        auto input = std::make_shared<MyFunctionInput>(1,2);
        // WRENCH_INFO("Created FunctionInput of type: %s", typeid(*input).name());

        // try {
        //     function_manager->invokeFunction(function2, this->compute_service, input);
        // } catch (ExecutionException& expected) {
        //     WRENCH_INFO("As expected, got exception: %s", expected.getCause()->toString().c_str());
        // }

        // WRENCH_INFO("Registering function 2");
        // function_manager->registerFunction(function2, this->compute_service, 50, 2000 * MB, 8000 * MB, 10 * MB, 1 * MB);
        // WRENCH_INFO("Function 2 registered");

        // std::vector<std::shared_ptr<Invocation>> invocations;

        // for (unsigned int i = 0; i < numInvocations; i++) {
        //     WRENCH_INFO("Invoking function 1");
        //     invocations.push_back(function_manager->invokeFunction(function1, this->compute_service, input));
        //     WRENCH_INFO("Function 1 invoked");
        //     WRENCH_INFO("Invoking function 2");
        //     invocations.push_back(function_manager->invokeFunction(function2, this->compute_service, input));
        //     WRENCH_INFO("Function 2 invoked");
        //     WRENCH_INFO("Invoking function 1");
        //     invocations.push_back(function_manager->invokeFunction(function1, this->compute_service, input));
        //     WRENCH_INFO("Function 1 invoked");
        //     // wrench::Simulation::sleep(1);
        // }

        // WRENCH_INFO("Waiting for all invocations to complete");
        // function_manager->wait_all(invocations);
        // WRENCH_INFO("All invocations completed");

        WRENCH_INFO("Invoking function 2");
        std::shared_ptr<Invocation> new_invocation = function_manager->invokeFunction(function2, this->compute_service, input);
        WRENCH_INFO("Function 2 invoked");

        try {
            new_invocation->isSuccess();
        } catch (std::runtime_error& expected) {
            WRENCH_INFO("As expected, got exception");
        }

        try {
            new_invocation->getOutput();
        } catch (std::runtime_error& expected) {
            WRENCH_INFO("As expected, got exception");
        }

        try {
            new_invocation->getFailureCause();
        } catch (std::runtime_error& expected) {
            WRENCH_INFO("As expected, got exception");
        }

        function_manager->wait_one(new_invocation);

        try {
            new_invocation->getOutput();
            WRENCH_INFO("First check passed");
            new_invocation->isSuccess();
            WRENCH_INFO("Second check passed");
            new_invocation->getFailureCause();
            WRENCH_INFO("Third check passed");
        } catch (std::runtime_error& expected) {
            WRENCH_INFO("Not expected, got exception");
        }

        return 0;
    }



}// namespace wrench
