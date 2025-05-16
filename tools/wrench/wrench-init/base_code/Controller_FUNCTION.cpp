/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** An execution controller to execute a workflow
 **/

#define MB (1000.0 * 1000.0)
#define GB (1000.0 * 1000.0 * 1000.0)

#include <iostream>

#include "Controller.h"

WRENCH_LOG_CATEGORY(controller, "Log category for Controller");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param bare_metal_compute_service: a set of compute services available to run actions
     * @param storage_services: a set of storage services available to store files
     * @param hostname: the name of the host on which to start the Execution Controller
     */
    Controller::Controller(const std::shared_ptr<ServerlessComputeService> &serverless_compute_service,
                           const std::shared_ptr<SimpleStorageService> &storage_service,
                           const std::string &hostname) : ExecutionController(hostname, "controller"),
                                                          serverless_compute_service(serverless_compute_service), storage_service(storage_service) {}

    /**
     * @brief main method of the Controller
     *
     * @return 0 on completion
     *
     */
    int Controller::main() {

        /* Set the logging output to GREEN */
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);
        WRENCH_INFO("Controller starting");

        // Start a function manager
        auto function_manager = this->createFunctionManager();

        /* Create a function image file on the storage at the user's host */
        auto some_file = wrench::Simulation::addFile("image_file", 10 * GB);
        auto image_file_location = wrench::FileLocation::LOCATION(this->storage_service, some_file);
        wrench::StorageService::createFileAtLocation(image_file_location);

         // Create the code for a function. This function sleeps for 10 seconds. It has absolutely generic Input and Output
         // objects that one would have to specialize to do whatever makes sense
        auto function_code = [](const std::shared_ptr<FunctionInput>& input,
                                const std::shared_ptr<StorageService>& my_storage_service) -> std::shared_ptr<
            FunctionOutput> {
            Simulation::sleep(10);
            return std::make_shared<FunctionOutput>();
        };

        // Create a function object
        auto function = wrench::FunctionManager::createFunction("my_function", function_code, image_file_location);

        WRENCH_INFO("Registering the function with the serverless compute service");

        // Define limits for the function execution
        double time_limit = 60.0;
        sg_size_t disk_space_limit_in_bytes = 500 * MB;
        sg_size_t RAM_limit_in_bytes = 200 * MB;
        sg_size_t ingress_in_bytes = 30 * MB;
        sg_size_t egress_in_bytes = 40 * MB;

        // Register the function to the serverless compute service, via the function manager
        auto registered_function = function_manager->registerFunction(function, serverless_compute_service,
                                                                      time_limit,
                                                                      disk_space_limit_in_bytes,
                                                                      RAM_limit_in_bytes,
                                                                      ingress_in_bytes,
                                                                      egress_in_bytes);

        WRENCH_INFO("Placing a function invocation!");
        auto invocation = function_manager->invokeFunction(registered_function, serverless_compute_service,
                                                       std::make_shared<FunctionInput>());

        WRENCH_INFO("Waiting for invocation to be done");
        function_manager->wait_one(invocation);

        std::cout << "Function invocation completed at time " << invocation->getEndDate() << " with a " <<
                    (invocation->hasSucceeded() ? "SUCCESS" : "FAILURE") << std::endl;
        return 0;
    }

}// namespace wrench
