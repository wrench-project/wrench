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

WRENCH_LOG_CATEGORY(serverless_load_balancing_scheduler_tests,
                    "Log category for ServerlessLoadBalancingSchedulerTest tests");

class ServerlessLoadBalancingSchedulerTest : public ::testing::Test {
public:
    std::shared_ptr<wrench::StorageService> storage_service1 = nullptr;
    std::shared_ptr<wrench::ServerlessComputeService> compute_service = nullptr;

    void do_Basic_test();

protected:
    ~ServerlessLoadBalancingSchedulerTest() {
        wrench::Simulation::removeAllFiles();
    }

    ServerlessLoadBalancingSchedulerTest() {
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
        <host id="ServerlessComputeNode2" speed="50Gf" core="10">
            <prop id="ram" value="64GB" />
            <disk id="hard_drive" read_bw="100MBps" write_bw="100MBps">
                <prop id="size" value="5000GiB"/>
                <prop id="mount" value="/"/>
            </disk>
        </host>

        <!-- A network link that connects both hosts -->
        <link id="network_link" bandwidth="10MBps" latency="20us"/>

        <!-- Network routes -->
        <route src="UserHost" dst="ServerlessHeadNode"> <link_ctn id="network_link"/></route>
        <route src="UserHost" dst="ServerlessComputeNode1"> <link_ctn id="network_link"/></route>
        <route src="UserHost" dst="ServerlessComputeNode2"> <link_ctn id="network_link"/></route>
        <route src="ServerlessHeadNode" dst="ServerlessComputeNode1"> <link_ctn id="network_link"/></route>
        <route src="ServerlessHeadNode" dst="ServerlessComputeNode2"> <link_ctn id="network_link"/></route>

    </zone>
</platform>)";

        FILE* platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};

/**********************************************************************/
/**  BASIC TEST                                                      **/
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

class ServerlessLoadBalancingSchedulerTestBasicController : public wrench::ExecutionController {
public:
    ServerlessLoadBalancingSchedulerTestBasicController(ServerlessLoadBalancingSchedulerTest* test,
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
    ServerlessLoadBalancingSchedulerTest* test;
    std::shared_ptr<wrench::ServerlessComputeService> compute_service;
    std::shared_ptr<wrench::StorageService> storage_service;

    int main() override {
        WRENCH_INFO("ServerlessExampleExecutionController started");

        // Register a function
        auto function_manager = this->createFunctionManager();
        std::function lambda = [](const std::shared_ptr<wrench::FunctionInput>& input,
                                  const std::shared_ptr<wrench::StorageService>& service) -> std::shared_ptr<wrench::FunctionOutput> {
            auto real_input = std::dynamic_pointer_cast<MyFunctionInput>(input);
            WRENCH_INFO("I am in user code!");
            return std::make_shared<MyFunctionOutput>("Processed: " + std::to_string(real_input->x1_ + real_input->x2_));
        };

        auto image_file = wrench::Simulation::addFile("image_file", 100 * MB);
        auto source_code = wrench::Simulation::addFile("source_code", 10 * MB);
        auto image_location = wrench::FileLocation::LOCATION(this->storage_service, image_file);
        auto code_location = wrench::FileLocation::LOCATION(this->storage_service, source_code);
        wrench::StorageService::createFileAtLocation(image_location);
        wrench::StorageService::createFileAtLocation(code_location);

        auto function1 = wrench::FunctionManager::createFunction("Function 1", lambda, image_location, code_location);

        WRENCH_INFO("Registering function 1");
        auto registered_function1 = function_manager->registerFunction(function1, this->compute_service, 10, 2000 * MB, 8000 * MB, 10 * MB, 1 * MB);
        WRENCH_INFO("Function 1 registered");

        auto function2 = wrench::FunctionManager::createFunction("Function 2", lambda, image_location, code_location);

        WRENCH_INFO("Registering function 2");
        auto registered_function2 = function_manager->registerFunction(function2, this->compute_service, 10, 2000 * MB, 8000 * MB, 10 * MB, 1 * MB);
        WRENCH_INFO("Function 2 registered");

        std::vector<std::shared_ptr<wrench::Invocation>> invocations;

        auto input = std::make_shared<MyFunctionInput>(1, 2);
        for (unsigned char i = 0; i < 200; i++) {
            WRENCH_INFO("Invoking function 1");
            invocations.push_back(function_manager->invokeFunction(registered_function1, this->compute_service, input));
            std::cerr << "HERE IN THE TEST\n";
            WRENCH_INFO("Function 1 invoked");
            // wrench::Simulation::sleep(1);
        }

        WRENCH_INFO("Waiting for all invocations to complete");
        function_manager->wait_all(invocations);
        WRENCH_INFO("All invocations completed");

        WRENCH_INFO("Invoking function 2");
        std::shared_ptr<wrench::Invocation> new_invocation = function_manager->invokeFunction(
            registered_function2, this->compute_service, input);
        WRENCH_INFO("Function 2 invoked");

        function_manager->wait_one(new_invocation);


        return 0;
    }
};

TEST_F(ServerlessLoadBalancingSchedulerTest, Basic) {
    DO_TEST_WITH_FORK(do_Basic_test);
}

void ServerlessLoadBalancingSchedulerTest::do_Basic_test() {
    int argc = 2;
    auto argv = (char**)calloc(argc, sizeof(char*));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-full-log");

    auto simulation = wrench::Simulation::createSimulation();
    simulation->init(&argc, argv);

    simulation->instantiatePlatform(this->platform_file_path);

    auto storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
        "UserHost", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "50MB"}}, {}));

    std::vector<std::string> batch_nodes = {"ServerlessComputeNode1", "ServerlessComputeNode2"};
    auto serverless_provider = simulation->add(new wrench::ServerlessComputeService(
        "ServerlessHeadNode", batch_nodes, "/", std::make_shared<wrench::RandomServerlessScheduler>(), {}, {}));

    std::string user_host = "UserHost";
    auto wms = simulation->add(
        new ServerlessLoadBalancingSchedulerTestBasicController(this, user_host, serverless_provider, storage_service));

    simulation->launch();

    double end_date = wrench::Simulation::getCurrentSimulatedDate();
    // ASSERT_DOUBLE_EQ(end_date, 120.0);
    // ASSERT_TRUE(end_date > 119.0 and end_date < 121.0);

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
