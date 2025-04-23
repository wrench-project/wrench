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
                                  const std::shared_ptr<wrench::StorageService>& service) -> std::string {
            auto real_input = std::dynamic_pointer_cast<MyFunctionInput>(input);
            WRENCH_INFO("I AM USER CODE");
            return "Processed: " + std::to_string(real_input->x1_ + real_input->x2_);
        };

        auto image_file = wrench::Simulation::addFile("input_file", 100 * MB);
        auto source_code = wrench::Simulation::addFile("source_code", 10 * MB);
        auto image_location = wrench::FileLocation::LOCATION(this->storage_service, image_file);
        auto code_location = wrench::FileLocation::LOCATION(this->storage_service, source_code);
        wrench::StorageService::createFileAtLocation(image_location);
        wrench::StorageService::createFileAtLocation(code_location);

        auto function1 = function_manager->createFunction("Function 1", lambda, image_location, code_location);

        WRENCH_INFO("Registering function 1");
        function_manager->registerFunction(function1, this->compute_service, 10, 2000 * MB, 8000 * MB, 10 * MB, 1 * MB);
        WRENCH_INFO("Function 1 registered");

        // Try to register the same function name
        WRENCH_INFO("Trying to register function 1 again");
        try {
            function_manager->registerFunction(function1, this->compute_service, 10, 2000 * MB, 8000 * MB, 10 * MB,
                                               1 * MB);
        }
        catch (wrench::ExecutionException& expected) {
            WRENCH_INFO("As expected, got exception: %s", expected.getCause()->toString().c_str());
        }

        auto function2 = function_manager->createFunction("Function 2", lambda, image_location, code_location);
        // Try to invoke a function that is not registered yet
        WRENCH_INFO("Invoking a non-registered function");
        auto input = std::make_shared<MyFunctionInput>(1, 2);

        try {
            function_manager->invokeFunction(function2, this->compute_service, input);
        }
        catch (wrench::ExecutionException& expected) {
            WRENCH_INFO("As expected, got exception: %s", expected.getCause()->toString().c_str());
        }

        WRENCH_INFO("Registering function 2");
        function_manager->registerFunction(function2, this->compute_service, 10, 2000 * MB, 8000 * MB, 10 * MB, 1 * MB);
        WRENCH_INFO("Function 2 registered");

        std::vector<std::shared_ptr<wrench::Invocation>> invocations;

        for (unsigned char i = 0; i < 200; i++) {
            WRENCH_INFO("Invoking function 1");
            invocations.push_back(function_manager->invokeFunction(function1, this->compute_service, input));
            WRENCH_INFO("Function 1 invoked");
            // wrench::Simulation::sleep(1);
        }

        WRENCH_INFO("Waiting for all invocations to complete");
        function_manager->wait_all(invocations);
        WRENCH_INFO("All invocations completed");

        WRENCH_INFO("Invoking function 2");
        std::shared_ptr<wrench::Invocation> new_invocation = function_manager->invokeFunction(
            function2, this->compute_service, input);
        WRENCH_INFO("Function 2 invoked");

        try {
            new_invocation->isSuccess();
        }
        catch (std::runtime_error& expected) {
            WRENCH_INFO("As expected, got exception");
        }

        try {
            new_invocation->getOutput();
        }
        catch (std::runtime_error& expected) {
            WRENCH_INFO("As expected, got exception");
        }

        try {
            new_invocation->getFailureCause();
        }
        catch (std::runtime_error& expected) {
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
        }
        catch (std::runtime_error& expected) {
            WRENCH_INFO("Not expected, got exception");
        }

        // wrench::Simulation::sleep(100);
        //
        // WRENCH_INFO("Invoking function 1 AGAIN");
        // function_manager->invokeFunction(function1, this->compute_service, input);
        // WRENCH_INFO("Function 1 invoked");

        wrench::Simulation::sleep(1000000);
        // WRENCH_INFO("Execution complete");

        // function_manager->invokeFunction(function2, this->compute_service, input);
        // function_manager->invokeFunction(function1, this->compute_service, input);

        return 0;
    }
};

TEST_F(ServerlessLoadBalancingSchedulerTest, Basic) {
    DO_TEST_WITH_FORK(do_Basic_test);
}

void ServerlessLoadBalancingSchedulerTest::do_Basic_test() {
    int argc = 1;
    auto argv = (char**)calloc(argc, sizeof(char*));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

    /* Initialize the simulation, which may entail extracting WRENCH-specific and
     * Simgrid-specific command-line arguments that can modify general simulation behavior.
     * Two special command-line arguments are --help-wrench and --help-simgrid, which print
     * details about available command-line arguments. */
    simulation->init(&argc, argv);

    /* Parsing of the command-line arguments for this WRENCH simulation */
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <xml platform file> [--log=custom_controller.threshold=info]" <<
            std::endl;
        exit(1);
    }

    std::cerr << "Instantiating simulated platform..." << std::endl;
    simulation->instantiatePlatform(argv[1]);

    std::cerr << "Instantiating a SimpleStorageService on UserHost..." << std::endl;
    auto storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
        "UserHost", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "50MB"}}, {}));

    /* Instantiate a serverless compute service */
    std::cerr << "Instantiating a serverless compute service on ServerlessHeadNode..." << std::endl;
    std::vector<std::string> batch_nodes = {"ServerlessComputeNode1", "ServerlessComputeNode2"};
    auto serverless_provider = simulation->add(new wrench::ServerlessComputeService(
        "ServerlessHeadNode", batch_nodes, "/", std::make_shared<wrench::RandomServerlessScheduler>(), {}, {}));

    /* Instantiate an Execution controller, to be stated on UserHost, which is responsible
     * for executing the workflow-> */
    std::string user_host = "UserHost";
    auto wms = simulation->add(
        new ServerlessLoadBalancingSchedulerTestBasicController(this, user_host, serverless_provider, storage_service));

    /* Launch the simulation. This call only returns when the simulation is complete. */
    std::cerr << "Launching the Simulation..." << std::endl;
    simulation->launch();
    std::cerr << "Simulation done!" << std::endl;

    double end_date = wrench::Simulation::getCurrentSimulatedDate();
    // ASSERT_DOUBLE_EQ(end_date, 120.0);
    // ASSERT_TRUE(end_date > 119.0 and end_date < 121.0);

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
