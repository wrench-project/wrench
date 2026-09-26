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
#include "wrench/failure_causes/OperationTimeout.h"
#include "wrench/failure_causes/FunctionNotFound.h"
#include "wrench/services/compute/serverless/schedulers/greedy_scheduler/GreedyServerlessScheduler.h"
#include "wrench/services/compute/serverless/schedulers/greedy_scheduler/eviction_policies/LRUServerlessEvictionPolicy.h"
#include "wrench/services/compute/serverless/schedulers/greedy_scheduler/eviction_policies/FewestServerlessEvictionPolicy.h"
#include "wrench/services/compute/serverless/schedulers/greedy_scheduler/invocation_sorting_policies/FCFSServerlessInvocationOrderingPolicy.h"
#include "wrench/services/compute/serverless/schedulers/greedy_scheduler/invocation_sorting_policies/RandomServerlessInvocationOrderingPolicy.h"
#include "wrench/services/compute/serverless/schedulers/greedy_scheduler/plan_selection_policies/EvictionAverseServerlessPlanSelectionPolicy.h"

#define GFLOP (1000.0 * 1000.0 * 1000.0)
#define MB (1000000ULL)
#define GB (1000000000ULL)

WRENCH_LOG_CATEGORY(serverless_multi_layer_tests,
                    "Log category for ServerlessMultiLayerTests tests");

class ServerlessMultiLayerTest : public ::testing::Test {
public:
    std::shared_ptr<wrench::StorageService> storage_service1 = nullptr;
    std::shared_ptr<wrench::ServerlessComputeService> compute_service = nullptr;

    void do_FunctionInvocationTest_test();

protected:
    ~ServerlessMultiLayerTest() override {
        wrench::Simulation::removeAllFiles();
    }

    ServerlessMultiLayerTest() {
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
        <host id="HostWrongMountPoint" speed="50Gf" core="10">
            <prop id="ram" value="64GB" />
            <disk id="hard_drive" read_bw="100MBps" write_bw="100MBps">
                <prop id="size" value="5000GiB"/>
                <prop id="mount" value="/stuff"/>
            </disk>
        </host>
        <host id="HostWrongSpeed" speed="5Gf" core="10">
            <prop id="ram" value="64GB" />
            <disk id="hard_drive" read_bw="100MBps" write_bw="100MBps">
                <prop id="size" value="5000GiB"/>
                <prop id="mount" value="/"/>
            </disk>
        </host>
        <host id="HostWrongCores" speed="50Gf" core="1">
            <prop id="ram" value="64GB" />
            <disk id="hard_drive" read_bw="100MBps" write_bw="100MBps">
                <prop id="size" value="5000GiB"/>
                <prop id="mount" value="/"/>
            </disk>
        </host>
        <host id="HostWrongRAM" speed="50Gf" core="10">
            <prop id="ram" value="32GB" />
            <disk id="hard_drive" read_bw="100MBps" write_bw="100MBps">
                <prop id="size" value="5000GiB"/>
                <prop id="mount" value="/stuff"/>
            </disk>
        </host>
        <host id="HostWrongDiskSpace" speed="50Gf" core="10">
            <prop id="ram" value="64GB" />
            <disk id="hard_drive" read_bw="100MBps" write_bw="100MBps">
                <prop id="size" value="50GiB"/>
                <prop id="mount" value="/stuff"/>
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
    explicit MyFunctionOutput(const std::string& msg) : msg_(msg) {
    }

    std::string msg_;
};


/**********************************************************************/
/**  FUNCTION INVOCATION TEST                                       **/
/**********************************************************************/

class ServerlessMultiLayerTestFunctionInvocationController : public wrench::ExecutionController {
public:
    ServerlessMultiLayerTestFunctionInvocationController(ServerlessMultiLayerTest* test,
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
    ServerlessMultiLayerTest* test;
    std::shared_ptr<wrench::ServerlessComputeService> compute_service;
    std::shared_ptr<wrench::StorageService> storage_service;

    int main() override {
        // Register a function that sleeps 0

        auto function_manager = this->createFunctionManager();
        std::function lambda = [](const std::shared_ptr<wrench::FunctionInput>& input,
                                  const std::shared_ptr<wrench::StorageService>& service) -> std::shared_ptr<
            wrench::FunctionOutput> {
            auto real_input = std::dynamic_pointer_cast<MyFunctionInput>(input);
            wrench::Simulation::sleep(0);
            return std::make_shared<MyFunctionOutput>("DONE");
        };

        // RUN A LAYER_1:LAYER2 function

        // Create layer 1
        auto layer1_file = wrench::Simulation::addFile("layer1_file", 100 * MB);
        auto layer1_location = wrench::FileLocation::LOCATION(this->storage_service, layer1_file);
        wrench::StorageService::createFileAtLocation(layer1_location);
        auto layer1 = wrench::FunctionManager::createImageLayer("my_layer1", layer1_location, layer1_file->getSize());
        // Create layer 2
        auto layer2_file = wrench::Simulation::addFile("layer2_file", 100 * MB);
        auto layer2_location = wrench::FileLocation::LOCATION(this->storage_service, layer2_file);
        wrench::StorageService::createFileAtLocation(layer2_location);
        auto layer2 = wrench::FunctionManager::createImageLayer("my_layer2", layer2_location, layer2_file->getSize());
        
        // Create the image
        auto image1 = wrench::FunctionManager::createImage("my_image", {layer1, layer2});

        // Registering a function
        auto registered_function1 = function_manager->registerFunction("Function 1", lambda, image1, this->compute_service, 10, 2000 * MB,
                                                                       8000 * MB, 10 * MB, 1 * MB);

        // Place an invocation
        auto input1 = std::make_shared<MyFunctionInput>(1, 2);
        auto invocation1 = function_manager->invokeFunction(registered_function1, this->compute_service, input1);
        function_manager->wait_one(invocation1);

        // RUN A LAYER1:LAYER3 function
        // Create layer 3
        auto layer3_file = wrench::Simulation::addFile("layer3_file", 100 * MB);
        auto layer3_location = wrench::FileLocation::LOCATION(this->storage_service, layer3_file);
        wrench::StorageService::createFileAtLocation(layer3_location);
        auto layer3 = wrench::FunctionManager::createImageLayer("my_layer3", layer3_location, layer3_file->getSize());

        // Create the image
        auto image2 = wrench::FunctionManager::createImage("my_image", {layer1, layer3});

        // Registering a function
        auto registered_function2 = function_manager->registerFunction("Function 2", lambda, image2, this->compute_service, 10, 2000 * MB,
                                                                       8000 * MB, 10 * MB, 1 * MB);

        // Place an invocation
        auto input2 = std::make_shared<MyFunctionInput>(1, 2);
        auto invocation2 = function_manager->invokeFunction(registered_function2, this->compute_service, input2);
        function_manager->wait_one(invocation2);


        // RUN A LAYER4 function
        // Create layer 3
        auto layer4_file = wrench::Simulation::addFile("layer4_file", 100 * MB);
        auto layer4_location = wrench::FileLocation::LOCATION(this->storage_service, layer4_file);
        wrench::StorageService::createFileAtLocation(layer4_location);
        auto layer4 = wrench::FunctionManager::createImageLayer("my_layer4", layer4_location, layer4_file->getSize());

        // Create the image
        auto image3 = wrench::FunctionManager::createImage("my_image", {layer4});

        // Registering a function
        auto registered_function3 = function_manager->registerFunction("Function 2", lambda, image3, this->compute_service, 10, 2000 * MB,
                                                                       8000 * MB, 10 * MB, 1 * MB);

        // Place an invocation
        auto input3 = std::make_shared<MyFunctionInput>(1, 2);
        auto invocation3 = function_manager->invokeFunction(registered_function3, this->compute_service, input3);
        function_manager->wait_one(invocation3);

        auto elapsed1 = invocation1->getDispatchDate() - invocation1->getSubmitDate();
        auto elapsed2 = invocation2->getDispatchDate() - invocation2->getSubmitDate();
        auto elapsed3 = invocation3->getDispatchDate() - invocation3->getSubmitDate();

        if (std::abs(elapsed2 - elapsed3) > DBL_EPSILON) {
            throw std::runtime_error("Inv2 and Inv3 should have the same elapsed time");
        }
        if (elapsed1 < elapsed2 + 5) {
            throw std::runtime_error("Inv1 should be markedly longer than Inv2/Inv3");
        }

        return 0;
    }
};

TEST_F(ServerlessMultiLayerTest, FunctionInvocation) {
    DO_TEST_WITH_FORK(do_FunctionInvocationTest_test);
}

void ServerlessMultiLayerTest::do_FunctionInvocationTest_test() {
    int argc = 1;
    auto argv = (char**)calloc(argc, sizeof(char*));
    argv[0] = strdup("unit_test");
    // argv[1] = strdup("--wrench-full-log");

    auto simulation = wrench::Simulation::createSimulation();
    simulation->init(&argc, argv);

    simulation->instantiatePlatform(this->platform_file_path);

    auto storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
        "UserHost", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "50MB"}}, {}));

    std::vector<std::string> compute_nodes = {"ServerlessComputeNode1"};
    auto serverless_provider = simulation->add(new wrench::ServerlessComputeService(
        "ServerlessHeadNode", "/", compute_nodes,
        std::make_shared<wrench::GreedyServerlessScheduler>(std::make_shared<wrench::RandomServerlessInvocationOrderingPolicy>(0), std::make_shared<wrench::FewestServerlessEvictionPolicy>(), std::make_shared<wrench::EvictionAverseServerlessPlanSelectionPolicy>()), {}, {}));

    std::string user_host = "UserHost";
    auto wms = simulation->add(
        new ServerlessMultiLayerTestFunctionInvocationController(this, user_host, serverless_provider, storage_service));

    simulation->launch();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

