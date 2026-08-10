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
#include "wrench/services/compute/serverless/schedulers/RandomServerlessScheduler.h"

#define GFLOP (1000.0 * 1000.0 * 1000.0)
#define MB (1000000ULL)
#define GB (1000000000ULL)

WRENCH_LOG_CATEGORY(serverless_failure_tests,
                    "Log category for ServerlessFailureTests tests");

class ServerlessFailureTest : public ::testing::Test {
public:
    std::shared_ptr<wrench::StorageService> storage_service1 = nullptr;
    std::shared_ptr<wrench::ServerlessComputeService> compute_service = nullptr;

    void do_RemoteDownloadFailure_test();

protected:
    ~ServerlessFailureTest() override {
        wrench::Simulation::removeAllFiles();
    }

    ServerlessFailureTest() {
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

        <!-- The host on which the Image Repo will run -->
        <host id="RepoHost" speed="10Gf" core="1">
            <disk id="hard_drive" read_bw="100MBps" write_bw="100MBps">
                <prop id="size" value="5000GiB"/>
                <prop id="mount" value="/"/>
            </disk>
        </host>

        <!-- The hosts on which the Serverless compute service will run -->
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

        <!-- A network link that connects the hosts hosts -->
        <link id="remote" bandwidth="1MBps" latency="20us"/>
        <link id="wide_area" bandwidth="20MBps" latency="20us"/>
        <link id="local_area" bandwidth="100Gbps" latency="1ns"/>

        <!-- Network routes -->
        <route src="RepoHost" dst="ServerlessHeadNode"> <link_ctn id="remote"/></route>
        <route src="UserHost" dst="ServerlessHeadNode"> <link_ctn id="wide_area"/></route>
        <route src="UserHost" dst="ServerlessComputeNode1"> <link_ctn id="wide_area"/> <link_ctn id="wide_area"/></route>
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
    explicit MyFunctionOutput(const std::string& msg) : msg_(msg) {
    }

    std::string msg_;
};


/**********************************************************************/
/**  REMOVE DOWNLOAD FAILURE TEST                                    **/
/**********************************************************************/

class ServerlessFailureTestRemoteDownloadController : public wrench::ExecutionController {
public:
    ServerlessFailureTestRemoteDownloadController(ServerlessFailureTest* test,
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
    ServerlessFailureTest* test;
    std::shared_ptr<wrench::ServerlessComputeService> compute_service;
    std::shared_ptr<wrench::StorageService> storage_service;

    int main() override {
        // Register a function
        auto function_manager = this->createFunctionManager();
        std::function lambda = [](const std::shared_ptr<wrench::FunctionInput>& input,
                                  const std::shared_ptr<wrench::StorageService>& service) -> std::shared_ptr<
            wrench::FunctionOutput> {
            auto real_input = std::dynamic_pointer_cast<MyFunctionInput>(input);
            wrench::Simulation::sleep(5);
            return std::make_shared<MyFunctionOutput>("DONE");
        };

        auto image_file = wrench::Simulation::addFile("image_file", 100 * MB);
        auto image_location = wrench::FileLocation::LOCATION(this->storage_service, image_file);
        wrench::StorageService::createFileAtLocation(image_location);

        auto function1 = wrench::FunctionManager::createFunction("Function 1", lambda, image_location);

        // Registering a function
        auto input = std::make_shared<MyFunctionInput>(1, 2);
        auto registered_function1 = function_manager->registerFunction(function1, this->compute_service, 10, 2000 * MB,
                                                                       8000 * MB, 10 * MB, 1 * MB);

        // Place a couple of invocations
        std::vector<std::shared_ptr<wrench::Invocation>> invocations;
        invocations.push_back(function_manager->invokeFunction(registered_function1, this->compute_service, input));
        // invocations.push_back(function_manager->invokeFunction(registered_function1, this->compute_service, input));

        // Sleep 5 seconds
        wrench::Simulation::sleep(5);

        // Turn of the remote link
        wrench::Simulation::turnOffLink("remote");

        // Wait for the two invocations
        function_manager->wait_all(invocations);

        for (auto const& invocation : invocations) {
            if (invocation->hasSucceeded()) {
                throw std::runtime_error("Invocation should not have succeeded");
            }
            auto failure_cause = invocation->getFailureCause();
            if (not std::dynamic_pointer_cast<wrench::NetworkError>(failure_cause)) {
                throw std::runtime_error("Failure cause should be a NetworkError, instead we got: " + failure_cause->toString());
            }
        }

        return 0;
    }
};

TEST_F(ServerlessFailureTest, RemoteDownloadFailure) {
    DO_TEST_WITH_FORK(do_RemoteDownloadFailure_test);
}

void ServerlessFailureTest::do_RemoteDownloadFailure_test() {
    int argc = 3;
    auto argv = (char**)calloc(argc, sizeof(char*));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-link-shutdown-simulation");
    argv[2] = strdup("--wrench-default-control-message-size=1024");
    // argv[3] = strdup("--wrench-full-log");

    auto simulation = wrench::Simulation::createSimulation();
    simulation->init(&argc, argv);

    simulation->instantiatePlatform(this->platform_file_path);

    auto storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
        "RepoHost", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "50MB"}}, {}));

    std::vector<std::string> compute_nodes = {"ServerlessComputeNode1"};
    auto serverless_provider = simulation->add(new wrench::ServerlessComputeService(
        "ServerlessHeadNode", "/", compute_nodes, std::make_shared<wrench::RandomServerlessScheduler>(),
        {{wrench::ServerlessComputeServiceProperty::STORAGE_SERVICES_BUFFER_SIZE, "50MB"}}, {}));

    std::string user_host = "UserHost";
    auto wms = simulation->add(
        new ServerlessFailureTestRemoteDownloadController(this, user_host, serverless_provider, storage_service));

    simulation->launch();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
