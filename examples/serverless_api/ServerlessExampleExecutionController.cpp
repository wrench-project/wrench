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
     * @param num_actions: the number of actions
     * @param compute_services: a set of compute services available to run actions
     * @param storage_services: a set of storage services available to store data files
     * @param hostname: the name of the host on which to start the WMS
     */
    ServerlessExampleExecutionController::ServerlessExampleExecutionController(std::shared_ptr<ServerlessComputeService> compute_service,
                                                                               std::shared_ptr<SimpleStorageService> storage_service,
                                                                               const std::string &hostname) : ExecutionController(hostname, "me"),
                                                                                                              compute_service(std::move(compute_service)),
                                                                                                              storage_service(std::move(storage_service)) {
    }

    /**
     * @brief main method of the TwoActionsAtATimeExecutionController daemon
     *
     * @return 0 on completion
     *
     */
    int ServerlessExampleExecutionController::main() {
        WRENCH_INFO("ServerlessExampleExecutionController started");

        // TODO: Interact with Serverless provider to do stuff
        // Register a function
        auto function_manager = this->createFunctionManager();
        std::function lambda = [](const std::shared_ptr<FunctionInput>& input, const std::shared_ptr<StorageService>& service) -> std::string {
            auto real_input = std::dynamic_pointer_cast<MyFunctionInput>(input);
            return "Processed: " + std::to_string(real_input->x1_ + real_input->x2_);
        };

        auto image_file = wrench::Simulation::addFile("input_file_", 100 * MB);
        auto source_code = wrench::Simulation::addFile("source_code", 10 * MB);
        auto image_location = wrench::FileLocation::LOCATION(this->storage_service, image_file);
        auto code_location = wrench::FileLocation::LOCATION(this->storage_service, source_code);
        wrench::StorageService::createFileAtLocation(image_location);
        wrench::StorageService::createFileAtLocation(code_location);

        auto function1 = function_manager->createFunction("Function 1", lambda, image_location, code_location);
        function_manager->registerFunction(function1, this->compute_service, 10, 2000 * MB, 8000 * MB, 10 * MB, 1 * MB);
        
        // Try to register the same function name
        try {
            function_manager->registerFunction(function1, this->compute_service, 10, 2000 * MB, 8000 * MB, 10 * MB, 1 * MB);
        } catch (ExecutionException& expected) {
            WRENCH_INFO(expected.getCause()->toString().c_str());

        }
        auto function2 = function_manager->createFunction("Function 2", lambda, image_location, code_location);
        
        // Try to invoke a function that is not registered yet
        auto input = std::make_shared<MyFunctionInput>(1,2);

        try {
            function_manager->invokeFunction(function2, this->compute_service, input);
        } catch (ExecutionException& expected) {
            WRENCH_INFO(expected.getCause()->toString().c_str());
        }

        function_manager->registerFunction(function2, this->compute_service, 10, 2000 * MB, 8000 * MB, 10 * MB, 1 * MB);

        //TODO: Should the EC be responsibile for keeping track of its invocations?
        function_manager->invokeFunction(function1, this->compute_service, input);
        function_manager->invokeFunction(function2, this->compute_service, input);
        function_manager->invokeFunction(function1, this->compute_service, input);

        WRENCH_INFO("Execution complete");
        return 0;
    }



}// namespace wrench
