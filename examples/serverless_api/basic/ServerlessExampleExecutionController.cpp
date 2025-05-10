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

#define MB (1000000ULL)

WRENCH_LOG_CATEGORY(custom_controller, "Log category for ServerlessExampleExecutionController");

namespace wrench {
    /**
     *  A class that implements the notion of what a function takes in as input.
     *  This is purely logical, in that no data size is associated with this object. For instance,
     *  this class could contain pointers to file locations, and the actual code of the function
     *  would then read/write these files.  In this simple example, the function input is an id and a sleep time.
     */
    class MyFunctionInput final : public FunctionInput {
    public:
        MyFunctionInput(const int id, const int sleep_time) : id_(id), sleep_time_(sleep_time) {
        }

        int id_;
        int sleep_time_;
    };

    /**
     *  Similarly, a class that implements the notion of a function's output.
     */
    class MyFunctionOutput final : public FunctionOutput {
    public:
        explicit MyFunctionOutput(const std::string& msg) : msg_(msg) {
        }

        std::string msg_;
    };


    /**
     * @brief Constructor, which calls the super constructor
     *
     * @param compute_service: the serverless compute service to use
     * @param storage_service: the storage service on which files can be stored
     * @param hostname: the name of the host on which to start the WMS
     * @param num_invocations: the number of invocations to place
     */
    ServerlessExampleExecutionController::ServerlessExampleExecutionController(
        const std::shared_ptr<ServerlessComputeService>& compute_service,
        const std::shared_ptr<SimpleStorageService>& storage_service,
        const std::string& hostname, const unsigned long num_invocations) : ExecutionController(hostname, "me"),
                                                                  compute_service(compute_service),
                                                                  storage_service(storage_service),
                                                                  num_invocations_(num_invocations) {
    }

    /**
     * @brief main method of the ServerlessExampleExecutionController daemon
     *
     * @return 0 on completion
     */
    int ServerlessExampleExecutionController::main() {
        WRENCH_INFO("ServerlessExampleExecutionController starting");

        // Start a function manager
        auto function_manager = this->createFunctionManager();

        WRENCH_INFO("Creating a function");

        // Create the code for a function. This function sleeps for a number of seconds specified in its
        // input object, and returns an output object that contains a string message.
        auto function_code = [](const std::shared_ptr<FunctionInput>& input,
                                const std::shared_ptr<StorageService>& my_storage_service) -> std::shared_ptr<
            FunctionOutput> {
            auto my_input = std::dynamic_pointer_cast<MyFunctionInput>(input);
            Simulation::sleep(my_input->sleep_time_);
            return std::make_shared<MyFunctionOutput>("Function " + std::to_string(my_input->id_) + " completed");
        };

        // Create a 250MB image file on the storage service (e.g., the docker image for the function environment)
        auto image_file = Simulation::addFile("image_file", 250 * MB);
        auto image_file_location = FileLocation::LOCATION(storage_service, image_file);
        StorageService::createFileAtLocation(image_file_location);

        // Create a 10MB source code file on the storage service (e.g., some GitHub repo to clone for the function code)
        auto code_repo = Simulation::addFile("code_repo", 250 * MB);
        auto code_repo_location = FileLocation::LOCATION(storage_service, code_repo);
        StorageService::createFileAtLocation(code_repo_location);

        // Create the function object
        auto function = wrench::FunctionManager::createFunction("my_function", function_code, image_file_location,
                                                         code_repo_location);

        WRENCH_INFO("Registering the function with the serverless compute service");

        // Define limits for the function execution
        double time_limit = 300.0;
        sg_size_t disk_space_limit_in_bytes = 500 * MB;
        sg_size_t RAM_limit_in_bytes = 200 * MB;
        sg_size_t ingress_in_bytes = 30 * MB;
        sg_size_t egress_in_bytes = 40 * MB;

        // Register the function to the serverless compute service, via the function manager
        auto registered_function = function_manager->registerFunction(function, compute_service,
                                                                      time_limit,
                                                                      disk_space_limit_in_bytes,
                                                                      RAM_limit_in_bytes,
                                                                      ingress_in_bytes,
                                                                      egress_in_bytes);

        WRENCH_INFO("Placing %lu function invocations", num_invocations_);

        // Creating a random distribution for the sleep time... note that
        // although the specified time limit (when registering the function)
        // was 60 seconds, here we could have function invocations that go over
        // that time limit, meaning that some invocations will fail.
        std::mt19937 gen{42};
        std::uniform_int_distribution<> dist(10, 90);

        // Placing all invocations
        std::vector<std::shared_ptr<wrench::Invocation>> invocations;
        for (unsigned long i = 0; i < num_invocations_; ++i) {
            double sleep_time = dist(gen);
            WRENCH_INFO("Invoking a function that will sleep for %lf seconds", sleep_time);
            auto inv = function_manager->invokeFunction(registered_function, compute_service,
                                                        std::make_shared<MyFunctionInput>(i, sleep_time));
            invocations.push_back(inv);
            wrench::Simulation::sleep(10);
        }

        // Waiting for them all to finish
        WRENCH_INFO("Waiting for all %zu invocations to be done...", invocations.size());
        function_manager->wait_all(invocations);

        // Printing some information
        WRENCH_INFO("All invocations have completed:");
        unsigned long num_succeeded = 0;
        unsigned long num_failed = 0;
        for (unsigned long i = 0; i < invocations.size(); ++i) {
            const auto& invocation = invocations[i];
            bool succeeded = invocation->hasSucceeded();
            double submit_date = invocation->getSubmitDate();
            double start_date = invocation->getStartDate();
            double finish_date = invocation->getFinishDate();
            WRENCH_INFO("  - Invocation #%lu: [%s] submitted: %.2lf  started: %.2lf  finished: %.2lf",
                i, (succeeded ? "SUCCEEDED" : "FAILED"), submit_date, start_date, finish_date);
            if (not succeeded) {
                WRENCH_INFO("       (%s)", invocation->getFailureCause()->toString().c_str());
            }
        }

        return 0;
    }
} // namespace wrench
