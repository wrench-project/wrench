/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <math.h>
#include <gtest/gtest.h>
#include <wrench-dev.h>

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"
#include "wrench/services/compute/serverless/schedulers/RandomServerlessScheduler.h"


#define GFLOP (1000.0 * 1000.0 * 1000.0)
#define MB (1000000ULL)

WRENCH_LOG_CATEGORY(serverless_basic_tests,
                    "Log category for ServerlessBasicTests tests");

class ServerlessBasicTest : public ::testing::Test {
public:
    std::shared_ptr<wrench::StorageService> storage_service1 = nullptr;
    std::shared_ptr<wrench::ServerlessComputeService> compute_service = nullptr;

    void do_FunctionRegistrationTest_test();
    void do_FunctionInvocationTest_test();

protected:
    ~ServerlessBasicTest() override {
        wrench::Simulation::removeAllFiles();
    }

    ServerlessBasicTest() {
        // Create a platform file
        std::string xml = R"(<?xml version='1.0'?>
<!DOCTYPE platform SYSTEM "https://simgrid.org/simgrid.dtd">
<platform version="4.1">
    <zone id="AS0" routing="Full">

        <!-- The host on which the WMS will run -->
        <host id="UserHost" speed="10Gf" core="1">
            <disk id="hard_drive" read_bw="100MBps" write_bw="100MBps">
                <prop id="size" value="5000GiB"/>
                <prop id="mount" value="/"/>
            </disk>
        </host>

        <!-- The host on which the Serverless compute service will run -->
        <host id="ServerlessHeadNode" speed="10Gf" core="1">
            <prop id="ram" value="16GB" />
            <disk id="hard_drive" read_bw="100MBps" write_bw="100MBps">
                <prop id="size" value="5000GiB"/>
                <prop id="mount" value="/"/>
            </disk>
       </host>
        <host id="ServerlessComputeNode1" speed="50Gf" core="10">
            <prop id="ram" value="64GB" />
            <disk id="hard_drive" read_bw="100MBps" write_bw="100MBps">
                <prop id="size" value="5000GiB"/>
                <prop id="mount" value="/"/>
            </disk>
        </host>

        <!-- A network link that connects both hosts -->
        <link id="wide_area" bandwidth="10MBps" latency="20us"/>
        <link id="local_area" bandwidth="10000MBps" latency="1ns"/>

        <!-- Network routes -->
        <route src="UserHost" dst="ServerlessHeadNode"> <link_ctn id="wide_area"/></route>
        <route src="UserHost" dst="ServerlessComputeNode1"> <link_ctn id="wide_area"/> <link_ctn id="wide_area"/></route>
        <route src="ServerlessHeadNode" dst="ServerlessComputeNode1">  <link_ctn id="local_area"/></route>

    </zone>
</platform>)";

        FILE* platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};

/**********************************************************************/
/**  HELPER CLASSES                                                  **/
/**********************************************************************/

class MyFunctionInput : public wrench::FunctionInput {
public:
    MyFunctionInput(int x1, int x2) : x1_(x1), x2_(x2) {
    }

    int x1_;
    int x2_;
};


/**********************************************************************/
/**  FUNCTION REGISTRATION TEST                                      **/
/**********************************************************************/

class ServerlessBasicTestFunctionRegistrationController : public wrench::ExecutionController {
public:
    ServerlessBasicTestFunctionRegistrationController(ServerlessBasicTest* test,
                                                      const std::string& hostname,
                                                      const std::shared_ptr<wrench::ServerlessComputeService>
                                                      & compute_service,
                                                      const std::shared_ptr<wrench::StorageService>& storage_service) :
        wrench::ExecutionController(hostname, "test") {
        this->test = test;
        this->compute_service = compute_service;
        this->storage_service = storage_service;
    }

private:
    ServerlessBasicTest* test;
    std::shared_ptr<wrench::ServerlessComputeService> compute_service;
    std::shared_ptr<wrench::StorageService> storage_service;

    int main() override {
        // Register a function
        auto function_manager = this->createFunctionManager();
        std::function lambda = [](const std::shared_ptr<wrench::FunctionInput>& input,
                                  const std::shared_ptr<wrench::StorageService>& service) -> std::string {
            auto real_input = std::dynamic_pointer_cast<MyFunctionInput>(input);
            return "Processed: " + std::to_string(real_input->x1_ + real_input->x2_);
        };

        auto image_file = wrench::Simulation::addFile("input_file", 100 * MB);
        auto source_code = wrench::Simulation::addFile("source_code", 10 * MB);
        auto image_location = wrench::FileLocation::LOCATION(this->storage_service, image_file);
        auto code_location = wrench::FileLocation::LOCATION(this->storage_service, source_code);
        wrench::StorageService::createFileAtLocation(image_location);
        wrench::StorageService::createFileAtLocation(code_location);

        auto function1 = wrench::FunctionManager::createFunction("Function 1", lambda, image_location, code_location);

        function_manager->registerFunction(function1, this->compute_service, 10, 2000 * MB, 8000 * MB, 10 * MB, 1 * MB);

        // Try to register the same function name
        try {
            function_manager->registerFunction(function1, this->compute_service, 10, 2000 * MB, 8000 * MB, 10 * MB,
                                               1 * MB);
            throw std::runtime_error("Redundant function registration should have failed");
        }
        catch (wrench::ExecutionException& expected) {
            WRENCH_INFO("As expected, got exception: %s", expected.getCause()->toString().c_str());
        }

        auto function2 = wrench::FunctionManager::createFunction("Function 2", lambda, image_location, code_location);

        auto registered_function2 = function_manager->registerFunction(function2, this->compute_service, 10, 2000 * MB,
                                                                       8000 * MB, 10 * MB, 1 * MB);
        if (registered_function2->getFunction() != function2) {
            throw std::runtime_error("Registered function should be function2");
        }
        if (registered_function2->getFunctionImage() != image_location) {
            throw std::runtime_error("Registered function image should be image location");
        }
        if (registered_function2->getTimeLimit() != 10.0) {
            throw std::runtime_error("Registered function time limit should be 10 seconds");
        }

        return 0;
    }
};

TEST_F(ServerlessBasicTest, FunctionRegistration) {
    DO_TEST_WITH_FORK(do_FunctionRegistrationTest_test);
}

void ServerlessBasicTest::do_FunctionRegistrationTest_test() {
    int argc = 1;
    auto argv = (char**)calloc(argc, sizeof(char*));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

    auto simulation = wrench::Simulation::createSimulation();
    simulation->init(&argc, argv);

    simulation->instantiatePlatform(this->platform_file_path);

    auto storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
        "UserHost", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "50MB"}}, {}));

    std::vector<std::string> batch_nodes = {"ServerlessComputeNode1"};
    auto serverless_provider = simulation->add(new wrench::ServerlessComputeService(
        "ServerlessHeadNode", batch_nodes, "/", std::make_shared<wrench::RandomServerlessScheduler>(), {}, {}));

    std::string user_host = "UserHost";
    auto wms = simulation->add(
        new ServerlessBasicTestFunctionRegistrationController(this, user_host, serverless_provider, storage_service));

    simulation->launch();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  FUNCTION INVOCATION TEST                                       **/
/**********************************************************************/

class ServerlessBasicTestFunctionInvocationController : public wrench::ExecutionController {
public:
    ServerlessBasicTestFunctionInvocationController(ServerlessBasicTest* test,
                                                    const std::string& hostname,
                                                    const std::shared_ptr<wrench::ServerlessComputeService>
                                                    & compute_service,
                                                    const std::shared_ptr<wrench::StorageService>& storage_service) :
        wrench::ExecutionController(hostname, "test") {
        this->test = test;
        this->compute_service = compute_service;
        this->storage_service = storage_service;
    }

private:
    ServerlessBasicTest* test;
    std::shared_ptr<wrench::ServerlessComputeService> compute_service;
    std::shared_ptr<wrench::StorageService> storage_service;

    int main() override {
        // Register a function
        auto function_manager = this->createFunctionManager();
        std::function lambda = [](const std::shared_ptr<wrench::FunctionInput>& input,
                                  const std::shared_ptr<wrench::StorageService>& service) -> std::string {
            auto real_input = std::dynamic_pointer_cast<MyFunctionInput>(input);
            wrench::Simulation::sleep(5);
            return "Processed: " + std::to_string(real_input->x1_ + real_input->x2_);
        };

        auto image_file = wrench::Simulation::addFile("input_file", 100 * MB);
        auto source_code = wrench::Simulation::addFile("source_code", 10 * MB);
        auto image_location = wrench::FileLocation::LOCATION(this->storage_service, image_file);
        auto code_location = wrench::FileLocation::LOCATION(this->storage_service, source_code);
        wrench::StorageService::createFileAtLocation(image_location);
        wrench::StorageService::createFileAtLocation(code_location);

        auto function1 = wrench::FunctionManager::createFunction("Function 1", lambda, image_location, code_location);

        // Invoking a non-registered function
        auto input = std::make_shared<MyFunctionInput>(1, 2);

        try {
            function_manager->invokeFunction(function1, this->compute_service, input);
            throw std::runtime_error("Unregistered function invocation should have failed");
        }
        catch (wrench::ExecutionException& expected) {
            WRENCH_INFO("As expected, got exception: %s", expected.getCause()->toString().c_str());
        }

        function_manager->registerFunction(function1, this->compute_service, 10, 2000 * MB, 8000 * MB, 10 * MB, 1 * MB);

        // Place an invocation
        {
            auto invocation = function_manager->invokeFunction(function1, this->compute_service, input);


            auto registered_function = invocation->getRegisteredFunction();
            if (invocation->getRegisteredFunction()->getFunction() != function1) {
                throw std::runtime_error("Invocation's associated function should be function1");
            }
            if (invocation->getRegisteredFunction()->getTimeLimit() != 10.00) {
                throw std::runtime_error("Invocation's associated time limit should be 10.0");
            }
            if (invocation->getRegisteredFunction()->getFunctionImage() != image_location) {
                throw std::runtime_error("Invocation's associated image should be the image location");
            }

            if (invocation->isDone()) {
                throw std::runtime_error("Invocation should not be done, it hasn't been started!");
            }
            try {
                auto ignore = invocation->isSuccess();
                throw std::runtime_error("Shouldn't be able to call isSuccess() on an invocation that's not done yet");
            }
            catch (std::runtime_error& expected) {
            }

            try {
                auto ignore = invocation->getFailureCause();
                throw std::runtime_error("Shouldn't be able to call getFailureCause() on an invocation that's not done");
            } catch (std::runtime_error &expected) {
            }

            try {
                auto ignore = invocation->getOutput();
                throw std::runtime_error("Shouldn't be able to call getOutput() on an invocation that's not done");
            } catch (std::runtime_error &expected) {
            }

            wrench::Simulation::sleep(1);

            if (invocation->isDone()) {
                throw std::runtime_error("Invocation should not be done yet");
            }

            function_manager->wait_one(invocation);

            if (!invocation->isDone()) {
                throw std::runtime_error("Invocation should be done by now");
            }
            if (!invocation->isSuccess()) {
                throw std::runtime_error("Invocation should have succeeded");
            }
            if (invocation->getFailureCause()) {
                throw std::runtime_error("There should be no failure cause");
            }
        }

        // Place another invocation (same image, which should be cached, and 1-byte code for zero time git clone)
        {
            auto source_code_2 = wrench::Simulation::addFile("source_code_2", 1);  // 1 byte!
            auto code_location_2 = wrench::FileLocation::LOCATION(this->storage_service, source_code_2);
            wrench::StorageService::createFileAtLocation(code_location_2);

            auto function2 = wrench::FunctionManager::createFunction("Function 2", lambda, image_location, code_location_2);
            function_manager->registerFunction(function2, this->compute_service, 10, 2000 * MB, 8000 * MB, 10 * MB, 1 * MB);

            auto now = wrench::Simulation::getCurrentSimulatedDate();
            auto invocation2 = function_manager->invokeFunction(function2, this->compute_service, input);

            function_manager->wait_one(invocation2);
            auto elapsed = wrench::Simulation::getCurrentSimulatedDate() - now;
            // Expected is 6: 1 sec to read image from disk, since image will be cached there
            if (fabs(elapsed - 6.0) > 0.001) {
                throw std::runtime_error(
                    "Invocation should have finished in about ~5s (instead: " + std::to_string(elapsed) + ")");
            }
        }


        return 0;
    }
};

TEST_F(ServerlessBasicTest, FunctionInvocation) {
    DO_TEST_WITH_FORK(do_FunctionInvocationTest_test);
}

void ServerlessBasicTest::do_FunctionInvocationTest_test() {
    int argc = 1;
    auto argv = (char**)calloc(argc, sizeof(char*));
    argv[0] = strdup("unit_test");
    // argv[1] = strdup("--wrench-full-log");

    auto simulation = wrench::Simulation::createSimulation();
    simulation->init(&argc, argv);

    simulation->instantiatePlatform(this->platform_file_path);

    auto storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
        "UserHost", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "50MB"}}, {}));

    std::vector<std::string> batch_nodes = {"ServerlessComputeNode1"};
    auto serverless_provider = simulation->add(new wrench::ServerlessComputeService(
        "ServerlessHeadNode", batch_nodes, "/", std::make_shared<wrench::RandomServerlessScheduler>(), {}, {}));

    std::string user_host = "UserHost";
    auto wms = simulation->add(
        new ServerlessBasicTestFunctionInvocationController(this, user_host, serverless_provider, storage_service));

    simulation->launch();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
