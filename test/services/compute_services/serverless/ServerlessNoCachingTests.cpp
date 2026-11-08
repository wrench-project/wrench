/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <cmath>
#include <utility>
#include <gtest/gtest.h>
#include <wrench-dev.h>

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"
#include "wrench/failure_causes/OperationTimeout.h"
#include "wrench/services/compute/serverless/schedulers/greedy_scheduler/GreedyServerlessScheduler.h"
#include "wrench/services/compute/serverless/schedulers/scheduling_policies/invocation_ordering_policies/RandomInvocationOrderingPolicy.h"
#include "wrench/services/compute/serverless/schedulers/scheduling_policies/node_activation_policies/ActivateAllNodePolicy.h"

#define GFLOP (1000.0 * 1000.0 * 1000.0)
#define MB (1000000ULL)
#define GB (1000000000ULL)

#include "ServerlessTestUtil.h"

WRENCH_LOG_CATEGORY(serverless_no_caching_tests,
                    "Log category for ServerlessNoCachingTests tests");

class ServerlessNoCachingTest : public ::testing::Test {
public:
    std::shared_ptr<wrench::StorageService> storage_service1 = nullptr;
    std::shared_ptr<wrench::ServerlessComputeService> compute_service_caching = nullptr;
    std::shared_ptr<wrench::ServerlessComputeService> compute_service_non_caching = nullptr;

    void do_BasicTest_test();

protected:
    ~ServerlessNoCachingTest() override {
        wrench::Simulation::removeAllFiles();
    }

    ServerlessNoCachingTest() {
        // Create a platform file
        std::string xml = R"(<?xml version='1.0'?>
<!DOCTYPE platform SYSTEM "https://simgrid.org/simgrid.dtd">
<platform version="4.1">
    <zone id="AS0" routing="Full">

        <!-- The host on which the WMS will run -->
        <host id="UserHost" speed="10Gf" core="1">
            <disk id="hard_drive" read_bw="100TBps" write_bw="100TBps">
                <prop id="size" value="5000GiB"/>
                <prop id="mount" value="/"/>
            </disk>
        </host>

        <!-- The host on which the Serverless compute service will run -->
        <host id="ServerlessHeadNode" speed="10Gf" core="1">
            <prop id="ram" value="16GB" />
            <disk id="hard_drive" read_bw="100GBps" write_bw="100GBps">
                <prop id="size" value="5000GiB"/>
                <prop id="mount" value="/"/>
            </disk>
       </host>
        <host id="ServerlessComputeNode1" speed="1f" core="1">
            <prop id="ram" value="64GB" />
            <disk id="hard_drive" read_bw="100TBps" write_bw="100TBps">
                <prop id="size" value="5000GiB"/>
                <prop id="mount" value="/"/>
            </disk>
        </host>
        <host id="ServerlessComputeNode2" speed="1f" core="1">
            <prop id="ram" value="64GB" />
            <disk id="hard_drive" read_bw="100TBps" write_bw="100TBps">
                <prop id="size" value="5000GiB"/>
                <prop id="mount" value="/"/>
            </disk>
        </host>

        <!-- A network link that connects both hosts -->
        <link id="wide_area" bandwidth="20MBps" latency="20us"/>
        <link id="local_area" bandwidth="100Tbps" latency="1ns"/>

        <!-- Network routes -->
        <route src="UserHost" dst="ServerlessHeadNode"> <link_ctn id="wide_area"/></route>
        <route src="UserHost" dst="ServerlessComputeNode1"> <link_ctn id="wide_area"/> </route>
        <route src="UserHost" dst="ServerlessComputeNode2"> <link_ctn id="wide_area"/> </route>
        <route src="ServerlessHeadNode" dst="ServerlessComputeNode1">  <link_ctn id="local_area"/></route>
        <route src="ServerlessHeadNode" dst="ServerlessComputeNode2">  <link_ctn id="local_area"/></route>

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
    explicit MyFunctionOutput(std::string  msg) : msg_(std::move(msg)) {
    }

    std::string msg_;
};


/**********************************************************************/
/**  FUNCTION INVOCATION TEST                                       **/
/**********************************************************************/

class ServerlessNoCachingTestFunctionInvocationController : public wrench::ExecutionController {
public:
    ServerlessNoCachingTestFunctionInvocationController(
        ServerlessNoCachingTest* test,
        const std::string& hostname,
        const std::shared_ptr<wrench::ServerlessComputeService>& compute_service_caching,
        const std::shared_ptr<wrench::ServerlessComputeService>& compute_service_non_caching,
        const std::shared_ptr<wrench::StorageService>& storage_service) :
        wrench::ExecutionController(hostname, "test") {
        this->test = test;
        this->compute_service_caching = compute_service_caching;
        this->compute_service_non_caching = compute_service_non_caching;
        this->storage_service = storage_service;
    }

private:
    ServerlessNoCachingTest* test;
    std::shared_ptr<wrench::ServerlessComputeService> compute_service_caching;
    std::shared_ptr<wrench::ServerlessComputeService> compute_service_non_caching;
    std::shared_ptr<wrench::StorageService> storage_service;

    int main() override {
        // Register a function that computes for 10 seconds
        auto function_manager = this->createFunctionManager();
        std::function lambda = [](const std::shared_ptr<wrench::FunctionInput>& input,
                                  const std::shared_ptr<wrench::StorageService>& service) -> std::shared_ptr<
            wrench::FunctionOutput> {
            auto real_input = std::dynamic_pointer_cast<MyFunctionInput>(input);
            wrench::Simulation::compute(10);
            return std::make_shared<MyFunctionOutput>("DONE");
        };

        // Create layer 1
        auto layer1_file = wrench::Simulation::addFile("layer1_file", 100 * MB);
        auto layer1_location = wrench::FileLocation::LOCATION(this->storage_service, layer1_file);
        wrench::StorageService::createFileAtLocation(layer1_location);
        auto layer1 = wrench::FunctionManager::createImageLayer("my_layer1", layer1_location, layer1_file->getSize());

        // Create the image
        auto image1 = wrench::FunctionManager::createImage("my_image", {layer1});

        /** CACHING SERVICE **/
        std::vector<double> caching_function_start_dates;
        {
            auto registered_function1 = function_manager->registerFunction("Function 1", noop_init, lambda, image1,
                                                                           this->compute_service_caching, 3600, 2000 * MB,
                                                                           8000 * MB, 10 * MB, 1 * MB);

            auto input = std::make_shared<MyFunctionInput>(1, 2);

            // Place 4 invocations and wait for them
            std::vector<std::shared_ptr<wrench::Invocation>> invocations;
            for (int i=0; i < 2; i++) {
                invocations.push_back(function_manager->invokeFunction(registered_function1, this->compute_service_caching, input));
            }
            function_manager->wait_all(invocations);
            for (auto &invocation : invocations) {
                caching_function_start_dates.push_back(invocation->getFunctionStartDate());
            }
        }

        /** NON-CACHING SERVICE **/
        std::vector<double> non_caching_function_start_dates;
        {
            auto registered_function1 = function_manager->registerFunction("Function 1", noop_init, lambda, image1,
                                                                           this->compute_service_non_caching, 3600, 2000 * MB,
                                                                           8000 * MB, 10 * MB, 1 * MB);

            auto input = std::make_shared<MyFunctionInput>(1, 2);

            // Place 4 invocations and wait for them
            auto time_origin = wrench::Simulation::getCurrentSimulatedDate();
            std::vector<std::shared_ptr<wrench::Invocation>> invocations;
            for (int i=0; i < 2; i++) {
                invocations.push_back(function_manager->invokeFunction(registered_function1, this->compute_service_non_caching, input));
            }
            function_manager->wait_all(invocations);
            for (auto &invocation : invocations) {
                non_caching_function_start_dates.push_back(invocation->getFunctionStartDate() - time_origin);
            }
        }

        // Check coherence
        for (int i=0; i < caching_function_start_dates.size(); i++) {
            if (std::abs(2 * caching_function_start_dates.at(i) - non_caching_function_start_dates.at(i)) > 0.1) {
                throw std::runtime_error("Unexpected function start dates: caching=" +
                    std::to_string(caching_function_start_dates.at(i)) +
                    " non-caching=" + std::to_string(non_caching_function_start_dates.at(i)) + ")");
            }
        }



        return 0;
    }
};

TEST_F(ServerlessNoCachingTest, BasicTest) {
    DO_TEST_WITH_FORK(do_BasicTest_test);
}

void ServerlessNoCachingTest::do_BasicTest_test() {
    int argc = 1;
    auto argv = (char**)calloc(argc, sizeof(char*));
    argv[0] = strdup("unit_test");
    // argv[1] = strdup("--wrench-full-log");

    auto simulation = wrench::Simulation::createSimulation();
    simulation->init(&argc, argv);

    simulation->instantiatePlatform(this->platform_file_path);

    auto storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
        "UserHost", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "50MB"}}, {}));

    std::vector<std::string> compute_nodes = {"ServerlessComputeNode1", "ServerlessComputeNode2"};

    auto serverless_provider_caching = simulation->add(new wrench::ServerlessComputeService(
        "ServerlessHeadNode", "/", compute_nodes,
        std::make_shared<wrench::GreedyServerlessScheduler>(),
        {
            {wrench::ServerlessComputeServiceProperty::CACHE_IMAGES_ON_HEAD_NODE, "true"},
        }, {}));

    auto serverless_provider_non_caching = simulation->add(new wrench::ServerlessComputeService(
        "ServerlessHeadNode", "/", compute_nodes,
        std::make_shared<wrench::GreedyServerlessScheduler>(),
        {
            {wrench::ServerlessComputeServiceProperty::NUM_COMPUTE_NODES_INITIALLY_ACTIVATED, "all"},
            {wrench::ServerlessComputeServiceProperty::CACHE_IMAGES_ON_HEAD_NODE, "false"},
        }, {}));

    std::string user_host = "UserHost";
    auto wms = simulation->add(
        new ServerlessNoCachingTestFunctionInvocationController(this, user_host,
                                                                serverless_provider_caching,
                                                                serverless_provider_non_caching, storage_service));

    simulation->launch();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
