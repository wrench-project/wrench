/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

// #include <math.h>
#include <gtest/gtest.h>
#include <wrench-dev.h>

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"
#include "wrench/services/compute/serverless/schedulers/FCFSServerlessScheduler.h"
#include "wrench/services/compute/serverless/schedulers/RandomServerlessScheduler.h"

#define GFLOP (1000.0 * 1000.0 * 1000.0)
#define MB (1000000ULL)

WRENCH_LOG_CATEGORY(serverless_timing_tests,
                    "Log category for ServerlessTimingTests tests");

class ServerlessTimingTest : public ::testing::Test {
public:
    std::shared_ptr<wrench::StorageService> storage_service1 = nullptr;
    std::shared_ptr<wrench::ServerlessComputeService> compute_service = nullptr;

    void do_FunctionInvocationTest_test();

protected:
    ~ServerlessTimingTest() override {
        wrench::Simulation::removeAllFiles();
    }

    ServerlessTimingTest() {
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
        <link id="wide_area" bandwidth="20MBps" latency="20us"/>
        <link id="local_area" bandwidth="100Gbps" latency="1ns"/>

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

class MyFunctionOutput : public wrench::FunctionOutput {
public:
    MyFunctionOutput(const std::string& msg) : msg_(msg) {
    }

    std::string msg_;
};

/**********************************************************************/
/**  FUNCTION INVOCATION TEST                                       **/
/**********************************************************************/

class ServerlessTimingTestFunctionInvocationController : public wrench::ExecutionController {
public:
    ServerlessTimingTestFunctionInvocationController(ServerlessTimingTest* test,
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
    ServerlessTimingTest* test;
    std::shared_ptr<wrench::ServerlessComputeService> compute_service;
    std::shared_ptr<wrench::StorageService> storage_service;

    int main() override {
        // Register a function
        auto function_manager = this->createFunctionManager();
        std::function lambda = [](const std::shared_ptr<wrench::FunctionInput>& input,
                                  const std::shared_ptr<wrench::StorageService>& service) -> std::shared_ptr<wrench::FunctionOutput> {
            auto real_input = std::dynamic_pointer_cast<MyFunctionInput>(input);
            wrench::Simulation::sleep(5);
            return std::make_shared<MyFunctionOutput>("Processed!");
        };

        auto image_file = wrench::Simulation::addFile("image_file", 100 * MB);
        auto image_location = wrench::FileLocation::LOCATION(this->storage_service, image_file);
        wrench::StorageService::createFileAtLocation(image_location);
        auto function = wrench::FunctionManager::createFunction("Function", lambda, image_location);
        auto input = std::make_shared<MyFunctionInput>(1, 2);
        auto registered_function = function_manager->registerFunction(function, this->compute_service, 10, 2000 * MB, 8000 * MB, 10 * MB, 1 * MB);

        // Place an invocation
        {
            auto now = wrench::Simulation::getCurrentSimulatedDate();
            auto invocation = function_manager->invokeFunction(registered_function, this->compute_service, input);
            function_manager->wait_one(invocation);
            auto elapsed = wrench::Simulation::getCurrentSimulatedDate() - now;
            double remote_download = 5.4;  // estimated (bottleneck = wide area)
            double copy_to_compute_node = 1; // estimated (bottleneck = disk)
            double local_image_read = 1; // estimated (bottleneck = disk)
            double compute = 5; // estimate (bottleneck = sleep)
            double expected_elapsed = remote_download + copy_to_compute_node + local_image_read  + compute;

            if (fabs(elapsed - expected_elapsed) > 0.05) {
                throw std::runtime_error("1) Unexpected elapsed time " + std::to_string(elapsed) + " (expected: " + std::to_string(expected_elapsed) + ")");
            }
        }

        // Place another invocation (which saves on some stuff)
        {
            auto now = wrench::Simulation::getCurrentSimulatedDate();
            auto invocation = function_manager->invokeFunction(registered_function, this->compute_service, input);
            function_manager->wait_one(invocation);
            auto elapsed = wrench::Simulation::getCurrentSimulatedDate() - now;
            double remote_download = 0;  // cached
            double local_copy = 0; // ALREADY ON DISK!
            double local_image_read = 0; // ALREADY IN RAM!
            double compute = 5; // extimate (bottleneck = sleep)
            double expected_elapsed = remote_download + local_copy + local_image_read + compute;

            if (fabs(elapsed - expected_elapsed) > 0.05) {
                throw std::runtime_error("2) Unexpected elapsed time " + std::to_string(elapsed) + " (expected: " + std::to_string(expected_elapsed) + ")");
            }
        }

        return 0;
    }
};

TEST_F(ServerlessTimingTest, FunctionInvocation) {
    DO_TEST_WITH_FORK(do_FunctionInvocationTest_test);
}

void ServerlessTimingTest::do_FunctionInvocationTest_test() {
    int argc = 1;
    auto argv = (char**)calloc(argc, sizeof(char*));
    argv[0] = strdup("unit_test");
    // argv[1] = strdup("--wrench-full-log");

    auto simulation = wrench::Simulation::createSimulation();
    simulation->init(&argc, argv);

    simulation->instantiatePlatform(this->platform_file_path);

    auto storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
        "UserHost", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "0"}}, {}));

    std::vector<std::string> batch_nodes = {"ServerlessComputeNode1"};
    auto serverless_provider = simulation->add(new wrench::ServerlessComputeService(
        "ServerlessHeadNode", batch_nodes, "/", std::make_shared<wrench::FCFSServerlessScheduler>(), {}, {}));

    std::string user_host = "UserHost";
    auto wms = simulation->add(
        new ServerlessTimingTestFunctionInvocationController(this, user_host, serverless_provider, storage_service));

    simulation->launch();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
