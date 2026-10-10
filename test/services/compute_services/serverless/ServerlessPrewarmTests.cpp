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
#include "../../../include/UniqueTmpPathPrefix.h"
#include "wrench/failure_causes/OperationTimeout.h"
#include "wrench/failure_causes/FunctionNotFound.h"
#include "wrench/services/compute/serverless/schedulers/ServerlessScheduler.h"
#include "wrench/services/compute/serverless/schedulers/greedy_scheduler/GreedyServerlessScheduler.h"
#include "wrench/services/compute/serverless/schedulers/greedy_scheduler/eviction_policies/LRUServerlessEvictionPolicy.h"
#include "wrench/services/compute/serverless/schedulers/greedy_scheduler/eviction_policies/FewestServerlessEvictionPolicy.h"
#include "wrench/services/compute/serverless/schedulers/greedy_scheduler/invocation_sorting_policies/FCFSServerlessInvocationOrderingPolicy.h"
#include "wrench/services/compute/serverless/schedulers/greedy_scheduler/invocation_sorting_policies/RandomServerlessInvocationOrderingPolicy.h"
#include "wrench/services/compute/serverless/schedulers/greedy_scheduler/plan_selection_policies/EvictionAverseServerlessPlanSelectionPolicy.h"

#define GFLOP (1000.0 * 1000.0 * 1000.0)
#define MB (1000000ULL)
#define GB (1000000000ULL)

WRENCH_LOG_CATEGORY(serverless_prewarm_tests,
                    "Log category for ServerlessPrewarmTests tests");

class ServerlessPrewarmTest : public ::testing::Test {
public:
    std::shared_ptr<wrench::StorageService> storage_service1 = nullptr;
    std::shared_ptr<wrench::ServerlessComputeService> compute_service = nullptr;

    void do_BasicTest_test();

protected:
    ~ServerlessPrewarmTest() override {
        wrench::Simulation::removeAllFiles();
    }

    ServerlessPrewarmTest() {
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
        <host id="ServerlessComputeNode1" speed="50Gf" core="1">
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

    class CustomTestScheduler : public wrench::ServerlessScheduler {
    public:
        CustomTestScheduler() = default;

        std::shared_ptr<wrench::ServerlessSchedulingDecisions> schedule(
            const std::vector<std::shared_ptr<wrench::Invocation>>& schedulable_invocations,
            const wrench::ServerlessStateOfTheSystem* state) override {
            static int called = 0;
            static std::shared_ptr<wrench::Function> function = nullptr;
            std::shared_ptr<wrench::Container> container;

            auto decisions = std::make_shared<wrench::ServerlessSchedulingDecisions>();

            called++;

            std::shared_ptr<wrench::Invocation> inv = (schedulable_invocations.empty()
                                                           ? nullptr
                                                           : schedulable_invocations.front());
            if (inv) {
                function = inv->getFunction();
            }
            auto layer = (function ? *(function->getImage()->getLayers().begin()) : nullptr);
            auto compute_node = state->getComputeNodes().front();

            switch (called) {
            case 3:
                decisions->image_layer_copies_to_disk.push_back({layer, compute_node});
                break;
            case 4:
                decisions->image_layer_loads_to_RAM.push_back({layer, compute_node});
                break;
            case 5:
                decisions->invocation_dispatches.push_back({inv, compute_node});
                break;
            case 6:
                break;
            case 7:
                decisions->container_prewarms.push_back({function, compute_node});
                break;
            case 8:
                break;
            case 9:
                container = *(state->getComputeNodes().front()->getIdleContainers().begin());
                decisions->invocation_dispatches.push_back({inv, compute_node, container});
                break;
            default:
                break;
            }

            return decisions;
        }
    };
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

class ServerlessPrewarmTestFunctionInvocationController : public wrench::ExecutionController {
public:
    ServerlessPrewarmTestFunctionInvocationController(ServerlessPrewarmTest* test,
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
    ServerlessPrewarmTest* test;
    std::shared_ptr<wrench::ServerlessComputeService> compute_service;
    std::shared_ptr<wrench::StorageService> storage_service;

    int main() override {
        // Register a function that sleeps 10

        auto function_manager = this->createFunctionManager();
        std::function lambda = [](const std::shared_ptr<wrench::FunctionInput>& input,
                                  const std::shared_ptr<wrench::StorageService>& service) -> std::shared_ptr<
            wrench::FunctionOutput> {
            auto real_input = std::dynamic_pointer_cast<MyFunctionInput>(input);
            wrench::Simulation::sleep(10);
            return std::make_shared<MyFunctionOutput>("DONE");
        };

        // Create layer 1
        auto layer1_file = wrench::Simulation::addFile("layer1_file", 100 * MB);
        auto layer1_location = wrench::FileLocation::LOCATION(this->storage_service, layer1_file);
        wrench::StorageService::createFileAtLocation(layer1_location);
        auto layer1 = wrench::FunctionManager::createImageLayer("my_layer1", layer1_location, layer1_file->getSize());

        // Create the image
        auto image1 = wrench::FunctionManager::createImage("my_image", {layer1});

        // Registering a function
        auto registered_function1 = function_manager->registerFunction("Function 1", lambda, image1,
                                                                       this->compute_service, 3600, 2000 * MB,
                                                                       8000 * MB, 10 * MB, 1 * MB);

        // Place an invocation and wait for it
        auto input1 = std::make_shared<MyFunctionInput>(1, 2);
        auto invocation1 = function_manager->invokeFunction(registered_function1, this->compute_service, input1);
        function_manager->wait_one(invocation1);

        // Carefully sleep so that the sleep ends halfway through the pre-warm container startup overhead!
        wrench::Simulation::sleep(3605);

        // The next invocation should pay only 5s of startup overhead!
        auto invocation2 = function_manager->invokeFunction(registered_function1, this->compute_service, input1);
        function_manager->wait_one(invocation2);

        auto elapsed2 = invocation2->getFunctionEndDate() - invocation2->getSubmitDate();

        if (std::abs(elapsed2 - 15) > 10.0E-6) {
            throw std::runtime_error("The second invocation should have taken exactly 15 seconds (5 seconds of overhead only).");
        }

        return 0;
    }
};

TEST_F(ServerlessPrewarmTest, BasicTest) {
    DO_TEST_WITH_FORK(do_BasicTest_test);
}

void ServerlessPrewarmTest::do_BasicTest_test() {
    int argc = 1;
    auto argv = (char**)calloc(argc, sizeof(char*));
    argv[0] = strdup("unit_test");
//    argv[1] = strdup("--wrench-full-log");

    auto simulation = wrench::Simulation::createSimulation();
    simulation->init(&argc, argv);

    simulation->instantiatePlatform(this->platform_file_path);

    auto storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
        "UserHost", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "50MB"}}, {}));

    std::vector<std::string> compute_nodes = {"ServerlessComputeNode1"};
    auto serverless_provider = simulation->add(new wrench::ServerlessComputeService(
        "ServerlessHeadNode", "/", compute_nodes,
        std::make_shared<CustomTestScheduler>(),
        {
            {wrench::ServerlessComputeServiceProperty::CONTAINER_STARTUP_OVERHEAD, "10"},
            {wrench::ServerlessComputeServiceProperty::CONTAINER_IDLE_TIMEOUT, "3600"}
        }, {}));

    std::string user_host = "UserHost";
    auto wms = simulation->add(
        new ServerlessPrewarmTestFunctionInvocationController(this, user_host, serverless_provider, storage_service));

    simulation->launch();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
