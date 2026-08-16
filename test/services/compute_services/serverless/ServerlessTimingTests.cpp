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

#include <utility>

#include "../../../include/RuntimeAssert.h"
#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"
#include "wrench/services/compute/serverless/schedulers/greedy/FCFSServerlessScheduler.h"
#include "wrench/services/compute/serverless/schedulers/greedy/RandomServerlessScheduler.h"
#include "wrench/services/compute/serverless/schedulers/workload_balancing/WorkloadBalancingServerlessScheduler.h"

#define GFLOP (1000.0 * 1000.0 * 1000.0)
#define MB (1000000ULL)
#define GB (1000000000ULL)
#define EPSILON 0.01

WRENCH_LOG_CATEGORY(serverless_timing_tests,
                    "Log category for ServerlessTimingTests tests");

class ServerlessTimingTest : public ::testing::Test {
public:
    std::shared_ptr<wrench::StorageService> storage_service1 = nullptr;
    std::shared_ptr<wrench::ServerlessComputeService> compute_service = nullptr;

    void do_ImageReuse_test(const std::shared_ptr<wrench::ServerlessScheduler>& scheduler);
    void do_CorePressure_test(const std::shared_ptr<wrench::ServerlessScheduler>& scheduler);
    void do_RAMPressureDueToImages_test(const std::shared_ptr<wrench::ServerlessScheduler>& scheduler);
    void do_RAMPressureDueToInvocations_test(const std::shared_ptr<wrench::ServerlessScheduler>& scheduler);
    void do_DiskPressureDueToImages_test(const std::shared_ptr<wrench::ServerlessScheduler>& scheduler);
    void do_DiskPressureDueToInvocations_test(const std::shared_ptr<wrench::ServerlessScheduler>& scheduler);
    void do_HotStart_test(const std::shared_ptr<wrench::ServerlessScheduler>& scheduler);
    void do_SimpleImageEvictionFromDisk_test(const std::shared_ptr<wrench::ServerlessScheduler>& scheduler);
    void do_SimpleImageEvictionFromRAM_test(const std::shared_ptr<wrench::ServerlessScheduler>& scheduler);
    void do_TwoIdleContainers_test(const std::shared_ptr<wrench::ServerlessScheduler>& scheduler);
    void do_OneIdleContainerTwoInvocations_test(const std::shared_ptr<wrench::ServerlessScheduler>& scheduler);
    void do_TmpStorageClearing_test(const std::shared_ptr<wrench::ServerlessScheduler>& scheduler);
    void do_IdleContainerEviction_test(const std::shared_ptr<wrench::ServerlessScheduler>& scheduler);
    void do_ImageDownloadSimulation_test(const std::shared_ptr<wrench::ServerlessScheduler>& scheduler);

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
                <prop id="size" value="200GB"/>
                <prop id="mount" value="/"/>
            </disk>
        </host>
        <host id="ServerlessComputeNodeSmallDisk" speed="50Gf" core="10">
            <prop id="ram" value="64000GB" />
            <disk id="hard_drive" read_bw="100MBps" write_bw="100MBps">
                <prop id="size" value="100GB"/>
                <prop id="mount" value="/"/>
            </disk>
        </host>

        <!-- A network link that connects both hosts -->
        <link id="wide_area" bandwidth="20MBps" latency="20us"/>
        <link id="local_area" bandwidth="100Gbps" latency="1ns"/>

        <!-- Network routes -->
        <route src="UserHost" dst="ServerlessHeadNode"> <link_ctn id="wide_area"/></route>
        <route src="UserHost" dst="ServerlessComputeNode1"> <link_ctn id="wide_area"/> <link_ctn id="wide_area"/></route>
        <route src="UserHost" dst="ServerlessComputeNodeSmallDisk"> <link_ctn id="wide_area"/> <link_ctn id="wide_area"/></route>
        <route src="ServerlessHeadNode" dst="ServerlessComputeNode1">  <link_ctn id="local_area"/></route>
        <route src="ServerlessHeadNode" dst="ServerlessComputeNodeSmallDisk">  <link_ctn id="local_area"/></route>

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
    explicit MyFunctionOutput(std::string msg) : msg_(std::move(msg)) {
    }

    std::string toString() const { return msg_; }

    std::string msg_;
};

/**********************************************************************/
/**  IMAGE REUSE TEST                                                **/
/**********************************************************************/

class ServerlessImageReuseController : public wrench::ExecutionController {
public:
    ServerlessImageReuseController(ServerlessTimingTest* test,
                                   const std::string& hostname,
                                   const std::shared_ptr<wrench::ServerlessComputeService>
                                   & compute_service,
                                   const std::shared_ptr<wrench::StorageService>& storage_service) :
        ExecutionController(hostname, "test") {
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
                                  const std::shared_ptr<wrench::StorageService>& service) -> std::shared_ptr<
            wrench::FunctionOutput> {
            auto real_input = std::dynamic_pointer_cast<MyFunctionInput>(input);
            wrench::Simulation::sleep(5);
            return std::make_shared<MyFunctionOutput>("Processed!");
        };

        auto image_file = wrench::Simulation::addFile("image_file", 100 * MB);
        auto image_location = wrench::FileLocation::LOCATION(this->storage_service, image_file);
        wrench::StorageService::createFileAtLocation(image_location);
        auto image = wrench::FunctionManager::createImage("my_image", image_location, image_file->getSize());
        auto function = wrench::FunctionManager::createFunction("Function", lambda, image);
        auto input = std::make_shared<MyFunctionInput>(1, 2);
        auto registered_function = function_manager->registerFunction(function, this->compute_service, 10, 2000 * MB,
                                                                      8000 * MB, 10 * MB, 1 * MB);

        // Place an invocation
        {
            auto now = wrench::Simulation::getCurrentSimulatedDate();
            auto invocation = function_manager->invokeFunction(registered_function, this->compute_service, input);
            function_manager->wait_one(invocation);
            auto elapsed = wrench::Simulation::getCurrentSimulatedDate() - now;
            double remote_download = 5.4; // estimated (bottleneck = wide area)
            double copy_to_compute_node = 1; // estimated (bottleneck = disk)
            double local_image_read = 1; // estimated (bottleneck = disk)
            double compute = this->compute_service->getPropertyValueAsDouble(
                wrench::ServerlessComputeServiceProperty::CONTAINER_STARTUP_OVERHEAD) + 5;
            // estimate (bottleneck = sleep)
            double expected_elapsed = remote_download + copy_to_compute_node + local_image_read + compute;

            if (fabs(elapsed - expected_elapsed) > 0.05) {
                throw std::runtime_error(
                    "1) Unexpected elapsed time " + std::to_string(elapsed) + " (expected: " + std::to_string(
                        expected_elapsed) + ")");
            }
        }

        // Place another invocation (which will reuse the same idle container)
        {
            auto now = wrench::Simulation::getCurrentSimulatedDate();
            auto invocation = function_manager->invokeFunction(registered_function, this->compute_service, input);
            function_manager->wait_one(invocation);
            auto elapsed = wrench::Simulation::getCurrentSimulatedDate() - now;
            double remote_download = 0; // cached
            double local_copy = 0; // ALREADY ON DISK!
            double local_image_read = 0; // ALREADY IN RAM!
            double compute = 5; // estimate (bottleneck = sleep)
            double expected_elapsed = remote_download + local_copy + local_image_read + compute;

            if (fabs(elapsed - expected_elapsed) > 0.05) {
                throw std::runtime_error(
                    "2) Unexpected elapsed time " + std::to_string(elapsed) + " (expected: " + std::to_string(
                        expected_elapsed) + ")");
            }
        }

        // Register another function for that same image and invoke it (will reuse the image, but NOT the container)
        {
            auto function2 = wrench::FunctionManager::createFunction("Function2", lambda, image);
            auto input2 = std::make_shared<MyFunctionInput>(1, 2);
            auto registered_function2 = function_manager->registerFunction(
                function, this->compute_service, 10, 2000 * MB,
                8000 * MB, 10 * MB, 1 * MB);
            auto now = wrench::Simulation::getCurrentSimulatedDate();
            auto invocation = function_manager->invokeFunction(registered_function2, this->compute_service, input2);
            function_manager->wait_one(invocation);
            auto elapsed = wrench::Simulation::getCurrentSimulatedDate() - now;
            double remote_download = 0; // cached
            double local_copy = 0; // ALREADY ON DISK!
            double local_image_read = 0; // ALREADY IN RAM!
            double compute = this->compute_service->getPropertyValueAsDouble(
                wrench::ServerlessComputeServiceProperty::CONTAINER_STARTUP_OVERHEAD) + 5;
            // estimate (bottleneck = sleep)
            double expected_elapsed = remote_download + local_copy + local_image_read + compute;

            if (fabs(elapsed - expected_elapsed) > 0.05) {
                throw std::runtime_error(
                    "2) Unexpected elapsed time " + std::to_string(elapsed) + " (expected: " + std::to_string(
                        expected_elapsed) + ")");
            }
        }


        return 0;
    }
};

TEST_F(ServerlessTimingTest, ImageReuse) {
    std::vector<std::shared_ptr<wrench::ServerlessScheduler>> schedulers = {
        std::make_shared<wrench::FCFSServerlessScheduler>(),
        std::make_shared<wrench::RandomServerlessScheduler>(0),
        std::make_shared<wrench::WorkloadBalancingServerlessScheduler>(),
    };
    for (auto& scheduler : schedulers) {
        DO_TEST_WITH_FORK_ONE_ARG(do_ImageReuse_test, scheduler);
    }
}

void ServerlessTimingTest::do_ImageReuse_test(const std::shared_ptr<wrench::ServerlessScheduler>& scheduler) {
    int argc = 1;
    auto argv = (char**)calloc(argc, sizeof(char*));
    argv[0] = strdup("unit_test");
    // argv[1] = strdup("--wrench-full-log");

    auto simulation = wrench::Simulation::createSimulation();
    simulation->init(&argc, argv);

    simulation->instantiatePlatform(this->platform_file_path);

    auto storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
        "UserHost", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "0"}}, {}));

    std::vector<std::string> compute_nodes = {"ServerlessComputeNode1"};
    auto serverless_provider = simulation->add(new wrench::ServerlessComputeService(
        "ServerlessHeadNode", "/", compute_nodes, scheduler,
        {
            {wrench::ServerlessComputeServiceProperty::CONTAINER_STARTUP_OVERHEAD, "10.0"},
            {wrench::ServerlessComputeServiceProperty::CONTAINER_IDLE_TIMEOUT, "1000.0"}
        }, {}));

    std::string user_host = "UserHost";
    auto wms = simulation->add(
        new ServerlessImageReuseController(this, user_host, serverless_provider, storage_service));

    simulation->launch();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  RAM CORE PRESSURE TEST                                          **/
/**********************************************************************/

class ServerlessCorePressureController : public wrench::ExecutionController {
public:
    ServerlessCorePressureController(ServerlessTimingTest* test,
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
        auto function_manager = this->createFunctionManager();

        // Create a function
        std::function lambda = [](const std::shared_ptr<wrench::FunctionInput>& input,
                                  const std::shared_ptr<wrench::StorageService>& service) -> std::shared_ptr<
            wrench::FunctionOutput> {
            auto real_input = std::dynamic_pointer_cast<MyFunctionInput>(input);
            wrench::Simulation::sleep(50);
            return std::make_shared<MyFunctionOutput>("Processed!");
        };

        // Register that function with an image file
        auto image_file_1 = wrench::Simulation::addFile("image_file_1", 60 * GB);
        auto image_location_1 = wrench::FileLocation::LOCATION(this->storage_service, image_file_1);
        wrench::StorageService::createFileAtLocation(image_location_1);
        auto image_1 = wrench::FunctionManager::createImage("my_image_1", image_location_1, image_file_1->getSize());

        auto function_1 = wrench::FunctionManager::createFunction("Function_1", lambda, image_1);
        auto input_1 = std::make_shared<MyFunctionInput>(1, 2);
        // Pick the RAM limit so that only 4 invocations can run at a time
        auto registered_function_1 = function_manager->registerFunction(function_1, this->compute_service, 100,
                                                                        2000 * MB, 1 * MB, 10 * MB, 1 * MB);

        // Place 20 invocations, knowing that only 10 can run at a time
        std::vector<std::shared_ptr<wrench::Invocation>> invocations;
        unsigned long num_invocations = 20;
        invocations.reserve(num_invocations);
        for (unsigned long i = 0; i < num_invocations; i++) {
            auto invocation = function_manager->invokeFunction(registered_function_1, this->compute_service, input_1);
            invocations.push_back(invocation);
            wrench::Simulation::sleep(0.1);
        }

        // Wait for all of them to complete
        function_manager->wait_all(invocations);

        // for (int i=0; i < num_invocations; i++) {
        //     std::cerr << "INVOCATION #" << i << ": START TIME - COMPLETION TIME: " << invocations.at(i)->getSubmitDate() << ": " << invocations.at(i)->getDispatchDate() << " -> " << invocations.at(i)->getFunctionStartDate() << " -> " << invocations.at(i)->getFunctionEndDate() << std::endl;
        // }
        for (unsigned long i = 0; i < num_invocations; i += 10) {
            double start_date = invocations.at(i)->getFunctionStartDate();
            double end_date = invocations.at(i)->getFunctionEndDate();
            for (unsigned long j = i + 1; j < std::min<unsigned long>(i + 10, num_invocations); j++) {
                if (fabs(start_date - invocations.at(j)->getFunctionStartDate()) > 0.1) {
                    throw std::runtime_error("Unexpected execution pattern");
                }
                if (fabs(end_date - invocations.at(j)->getFunctionEndDate()) > 0.1) {
                    throw std::runtime_error("Unexpected execution pattern");
                }
            }
        }

        return 0;
    }
};

TEST_F(ServerlessTimingTest, CorePressure) {
    std::vector<std::shared_ptr<wrench::ServerlessScheduler>> schedulers = {
        std::make_shared<wrench::FCFSServerlessScheduler>(),
        // std::make_shared<wrench::RandomServerlessScheduler>(),
        // std::make_shared<wrench::WorkloadBalancingServerlessScheduler>(),
    };
    for (auto& scheduler : schedulers) {
        DO_TEST_WITH_FORK_ONE_ARG(do_CorePressure_test, scheduler);
    }
}

void ServerlessTimingTest::do_CorePressure_test(const std::shared_ptr<wrench::ServerlessScheduler>& scheduler) {
    int argc = 1;
    auto argv = (char**)calloc(argc, sizeof(char*));
    argv[0] = strdup("unit_test");
    // argv[1] = strdup("--wrench-full-log");

    auto simulation = wrench::Simulation::createSimulation();
    simulation->init(&argc, argv);

    simulation->instantiatePlatform(this->platform_file_path);

    auto storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
        "UserHost", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "0"}}, {}));

    std::vector<std::string> compute_nodes = {"ServerlessComputeNode1"};
    auto serverless_provider = simulation->add(new wrench::ServerlessComputeService(
        "ServerlessHeadNode", "/", compute_nodes, scheduler, {}, {}));

    std::string user_host = "UserHost";
    auto wms = simulation->add(
        new ServerlessCorePressureController(this, user_host, serverless_provider, storage_service));

    simulation->launch();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  RAM PRESSURE DUE TO IMAGES TEST                                 **/
/**********************************************************************/

class ServerlessRAMPressureDueToImagesController : public wrench::ExecutionController {
public:
    ServerlessRAMPressureDueToImagesController(ServerlessTimingTest* test,
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
        auto function_manager = this->createFunctionManager();

        // Create a function
        std::function lambda = [](const std::shared_ptr<wrench::FunctionInput>& input,
                                  const std::shared_ptr<wrench::StorageService>& service) -> std::shared_ptr<
            wrench::FunctionOutput> {
            auto real_input = std::dynamic_pointer_cast<MyFunctionInput>(input);
            wrench::Simulation::sleep(50);
            return std::make_shared<MyFunctionOutput>("Processed!");
        };

        // Register that function with an image file that will fill up RAM
        auto image_file_1 = wrench::Simulation::addFile("image_file_1", 60 * GB);
        auto image_location_1 = wrench::FileLocation::LOCATION(this->storage_service, image_file_1);
        wrench::StorageService::createFileAtLocation(image_location_1);
        auto image_1 = wrench::FunctionManager::createImage("my_image_1", image_location_1, image_file_1->getSize());

        auto function_1 = wrench::FunctionManager::createFunction("Function_1", lambda, image_1);
        auto input_1 = std::make_shared<MyFunctionInput>(1, 2);
        auto registered_function_1 = function_manager->registerFunction(function_1, this->compute_service, 100,
                                                                        2000 * MB, 1 * MB, 10 * MB, 1 * MB);

        // Place an invocation
        auto invocation_1 = function_manager->invokeFunction(registered_function_1, this->compute_service, input_1);

        // Register another function with an image file that will not fit in RAM
        auto image_file_2 = wrench::Simulation::addFile("image_file_2", 61 * GB);
        auto image_location_2 = wrench::FileLocation::LOCATION(this->storage_service, image_file_2);
        wrench::StorageService::createFileAtLocation(image_location_2);
        auto image_2 = wrench::FunctionManager::createImage("my_image_2", image_location_2, image_file_2->getSize());

        auto function_2 = wrench::FunctionManager::createFunction("Function_2", lambda, image_2);
        auto input_2 = std::make_shared<MyFunctionInput>(1, 2);
        auto registered_function_2 = function_manager->registerFunction(function_2, this->compute_service, 100,
                                                                        2000 * MB, 1 * MB, 10 * MB, 1 * MB);

        auto invocation_2 = function_manager->invokeFunction(registered_function_2, this->compute_service, input_2);

        function_manager->wait_one(invocation_1);
        function_manager->wait_one(invocation_2);

        // Checks
        if (invocation_1->getFunctionEndDate() >= invocation_2->getDispatchDate()) {
            throw std::runtime_error("Invocation #2 should have been dispatched before Invocation #1 finished");
        }
        if (std::abs(invocation_1->getFunctionEndDate() + 61.0 * GB / (100 * MB) - invocation_2->getDispatchDate()) >
            EPSILON) {
            throw std::runtime_error(
                "Invocation #2 dispatch date should roughly be Invocation #1's end date + time to load image to RAM: " +
                std::to_string(invocation_1->getFunctionEndDate() + 61.0 * GB / (100 * MB)) + " vs. " + std::to_string(
                    invocation_2->getDispatchDate()));
        }


        return 0;
    }
};

TEST_F(ServerlessTimingTest, RAMPressureDueToImages) {
    std::vector<std::shared_ptr<wrench::ServerlessScheduler>> schedulers = {
        std::make_shared<wrench::FCFSServerlessScheduler>(),
        std::make_shared<wrench::RandomServerlessScheduler>(0),
        std::make_shared<wrench::WorkloadBalancingServerlessScheduler>(),
    };
    for (auto& scheduler : schedulers) {
        DO_TEST_WITH_FORK_ONE_ARG(do_RAMPressureDueToImages_test, scheduler);
    }
}

void ServerlessTimingTest::do_RAMPressureDueToImages_test(
    const std::shared_ptr<wrench::ServerlessScheduler>& scheduler) {
    int argc = 1;
    auto argv = (char**)calloc(argc, sizeof(char*));
    argv[0] = strdup("unit_test");
    // argv[1] = strdup("--wrench-full-log");

    auto simulation = wrench::Simulation::createSimulation();
    simulation->init(&argc, argv);

    simulation->instantiatePlatform(this->platform_file_path);

    auto storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
        "UserHost", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "0"}}, {}));

    std::vector<std::string> compute_nodes = {"ServerlessComputeNode1"};
    auto serverless_provider = simulation->add(new wrench::ServerlessComputeService(
        "ServerlessHeadNode", "/", compute_nodes, scheduler, {}, {}));

    std::string user_host = "UserHost";
    auto wms = simulation->add(
        new ServerlessRAMPressureDueToImagesController(this, user_host, serverless_provider, storage_service));

    simulation->launch();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  RAM PRESSURE DUE TO INVOCATIONS TEST                            **/
/**********************************************************************/

class ServerlessRAMPressureDueToInvocationsController : public wrench::ExecutionController {
public:
    ServerlessRAMPressureDueToInvocationsController(ServerlessTimingTest* test,
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
        auto function_manager = this->createFunctionManager();

        // Create a function
        std::function lambda = [](const std::shared_ptr<wrench::FunctionInput>& input,
                                  const std::shared_ptr<wrench::StorageService>& service) -> std::shared_ptr<
            wrench::FunctionOutput> {
            auto real_input = std::dynamic_pointer_cast<MyFunctionInput>(input);
            wrench::Simulation::sleep(50);
            return std::make_shared<MyFunctionOutput>("Processed!");
        };

        // Register that function with an image file
        auto image_file_1 = wrench::Simulation::addFile("image_file_1", 60 * GB);
        auto image_location_1 = wrench::FileLocation::LOCATION(this->storage_service, image_file_1);
        wrench::StorageService::createFileAtLocation(image_location_1);
        auto image_1 = wrench::FunctionManager::createImage("my_image_1", image_location_1, image_file_1->getSize());

        auto function_1 = wrench::FunctionManager::createFunction("Function_1", lambda, image_1);
        auto input_1 = std::make_shared<MyFunctionInput>(1, 2);
        // Pick the RAM limit so that only 4 invocations can run at a time
        auto registered_function_1 = function_manager->registerFunction(function_1, this->compute_service, 100,
                                                                        2000 * MB, 1 * GB, 10 * MB, 1 * MB);

        // Place 10 invocations, knowing that only 4 can run at a time
        std::vector<std::shared_ptr<wrench::Invocation>> invocations;
        unsigned long num_invocations = 10;
        invocations.reserve(num_invocations);
        for (unsigned long i = 0; i < num_invocations; i++) {
            auto invocation = function_manager->invokeFunction(registered_function_1, this->compute_service, input_1);
            invocations.push_back(invocation);
            wrench::Simulation::sleep(0.1);
        }

        // Wait for all of them to complete
        function_manager->wait_all(invocations);

        // for (int i=0; i < num_invocations; i++) {
        //     std::cerr << "INVOCATION #" << i << ": START TIME - COMPLETION TIME: " << invocations.at(i)->getSubmitDate() << ": " << invocations.at(i)->getStartDate() << " -> " << invocations.at(i)->getEndDate() << std::endl;
        // }
        for (unsigned long i = 0; i < num_invocations; i += 4) {
            double start_date = invocations.at(i)->getDispatchDate();
            double end_date = invocations.at(i)->getFunctionEndDate();
            for (unsigned long j = i + 1; j < std::min<unsigned long>(i + 4, num_invocations); j++) {
                if (fabs(start_date - invocations.at(j)->getFunctionStartDate()) > 0.1) {
                    throw std::runtime_error("Unexpected execution pattern");
                }
                if (fabs(end_date - invocations.at(j)->getFunctionEndDate()) > 0.1) {
                    throw std::runtime_error("Unexpected execution pattern");
                }
            }
        }

        return 0;
    }
};

TEST_F(ServerlessTimingTest, RAMPressureDueToInvocations) {
    std::vector<std::shared_ptr<wrench::ServerlessScheduler>> schedulers = {
        std::make_shared<wrench::FCFSServerlessScheduler>(),
        // std::make_shared<wrench::RandomServerlessScheduler>(),
        // std::make_shared<wrench::WorkloadBalancingServerlessScheduler>(),
    };
    for (auto& scheduler : schedulers) {
        DO_TEST_WITH_FORK_ONE_ARG(do_RAMPressureDueToInvocations_test, scheduler);
    }
}

void ServerlessTimingTest::do_RAMPressureDueToInvocations_test(
    const std::shared_ptr<wrench::ServerlessScheduler>& scheduler) {
    int argc = 1;
    auto argv = (char**)calloc(argc, sizeof(char*));
    argv[0] = strdup("unit_test");
    // argv[1] = strdup("--wrench-full-log");

    auto simulation = wrench::Simulation::createSimulation();
    simulation->init(&argc, argv);

    simulation->instantiatePlatform(this->platform_file_path);

    auto storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
        "UserHost", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "0"}}, {}));

    std::vector<std::string> compute_nodes = {"ServerlessComputeNode1"};
    auto serverless_provider = simulation->add(new wrench::ServerlessComputeService(
        "ServerlessHeadNode", "/", compute_nodes, scheduler, {}, {}));

    std::string user_host = "UserHost";
    auto wms = simulation->add(
        new ServerlessRAMPressureDueToInvocationsController(this, user_host, serverless_provider, storage_service));

    simulation->launch();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  DISK PRESSURE DUE TO IMAGES TEST                                **/
/**********************************************************************/

class ServerlessDiskPressureDueToImagesController : public wrench::ExecutionController {
public:
    ServerlessDiskPressureDueToImagesController(ServerlessTimingTest* test,
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
        auto function_manager = this->createFunctionManager();

        // Create a function
        std::function lambda = [](const std::shared_ptr<wrench::FunctionInput>& input,
                                  const std::shared_ptr<wrench::StorageService>& service) -> std::shared_ptr<
            wrench::FunctionOutput> {
            auto real_input = std::dynamic_pointer_cast<MyFunctionInput>(input);
            wrench::Simulation::sleep(50);
            return std::make_shared<MyFunctionOutput>("Processed!");
        };

        // Register that function with an image file that will fill up the disk
        auto image_file_1 = wrench::Simulation::addFile("image_file_1", 60 * GB);
        auto image_location_1 = wrench::FileLocation::LOCATION(this->storage_service, image_file_1);
        wrench::StorageService::createFileAtLocation(image_location_1);
        auto image_1 = wrench::FunctionManager::createImage("my_image_1", image_location_1, image_file_1->getSize());

        auto function_1 = wrench::FunctionManager::createFunction("Function_1", lambda, image_1);
        auto input_1 = std::make_shared<MyFunctionInput>(1, 2);
        auto registered_function_1 = function_manager->registerFunction(function_1, this->compute_service, 100,
                                                                        2000 * MB, 1 * MB, 10 * MB, 1 * MB);

        // Place an invocation
        auto invocation_1 = function_manager->invokeFunction(registered_function_1, this->compute_service, input_1);

        // Register another function with an image file that will not fit on disk
        auto image_file_2 = wrench::Simulation::addFile("image_file_2", 61 * GB);
        auto image_location_2 = wrench::FileLocation::LOCATION(this->storage_service, image_file_2);
        wrench::StorageService::createFileAtLocation(image_location_2);
        auto image_2 = wrench::FunctionManager::createImage("my_image_2", image_location_2, image_file_2->getSize());

        auto function_2 = wrench::FunctionManager::createFunction("Function_2", lambda, image_2);
        auto input_2 = std::make_shared<MyFunctionInput>(1, 2);
        auto registered_function_2 = function_manager->registerFunction(function_2, this->compute_service, 100,
                                                                        2000 * MB, 1 * MB, 10 * MB, 1 * MB);

        auto invocation_2 = function_manager->invokeFunction(registered_function_2, this->compute_service, input_2);

        function_manager->wait_one(invocation_1);
        function_manager->wait_one(invocation_2);

        // We expect that as soon as invocation_1 has started, then invocation_2 can finally do the copy and load.
        double expected_invocation_2_start_date = invocation_1->getFunctionEndDate() + 2 * (61.0 * GB / (100 * MB));

        if (fabs(expected_invocation_2_start_date - invocation_2->getDispatchDate()) > 0.1) {
            throw std::runtime_error(
                "Unexpected start date for invocation_2 " + std::to_string(invocation_2->getDispatchDate()) +
                " (expected: " + std::to_string(expected_invocation_2_start_date) + ")");
        }

        return 0;
    }
};

TEST_F(ServerlessTimingTest, DiskPressureDueToImages) {
    std::vector<std::shared_ptr<wrench::ServerlessScheduler>> schedulers = {
        std::make_shared<wrench::FCFSServerlessScheduler>(),
        // std::make_shared<wrench::RandomServerlessScheduler>(),
        // std::make_shared<wrench::WorkloadBalancingServerlessScheduler>(),
    };
    for (auto& scheduler : schedulers) {
        DO_TEST_WITH_FORK_ONE_ARG(do_DiskPressureDueToImages_test, scheduler);
    }
}

void ServerlessTimingTest::do_DiskPressureDueToImages_test(
    const std::shared_ptr<wrench::ServerlessScheduler>& scheduler) {
    int argc = 1;
    auto argv = (char**)calloc(argc, sizeof(char*));
    argv[0] = strdup("unit_test");
    // argv[1] = strdup("--wrench-full-log");

    auto simulation = wrench::Simulation::createSimulation();
    simulation->init(&argc, argv);

    simulation->instantiatePlatform(this->platform_file_path);

    auto storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
        "UserHost", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "0"}}, {}));

    std::vector<std::string> compute_nodes = {"ServerlessComputeNodeSmallDisk"};
    auto serverless_provider = simulation->add(new wrench::ServerlessComputeService(
        "ServerlessHeadNode", "/", compute_nodes, scheduler, {}, {}));

    std::string user_host = "UserHost";
    auto wms = simulation->add(
        new ServerlessDiskPressureDueToImagesController(this, user_host, serverless_provider, storage_service));

    simulation->launch();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  DISK PRESSURE DUE TO INVOCATIONS TEST                           **/
/**********************************************************************/

class ServerlessDiskPressureDueToInvocationsController : public wrench::ExecutionController {
public:
    ServerlessDiskPressureDueToInvocationsController(ServerlessTimingTest* test,
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
        auto function_manager = this->createFunctionManager();

        // Create a function
        std::function lambda = [](const std::shared_ptr<wrench::FunctionInput>& input,
                                  const std::shared_ptr<wrench::StorageService>& service) -> std::shared_ptr<
            wrench::FunctionOutput> {
            auto real_input = std::dynamic_pointer_cast<MyFunctionInput>(input);
            wrench::Simulation::sleep(50);
            return std::make_shared<MyFunctionOutput>("Processed!");
        };

        // Register that function with an image file that will fill up the disk
        auto image_file_1 = wrench::Simulation::addFile("image_file_1", 60 * GB);
        auto image_location_1 = wrench::FileLocation::LOCATION(this->storage_service, image_file_1);
        wrench::StorageService::createFileAtLocation(image_location_1);
        auto image_1 = wrench::FunctionManager::createImage("my_image_1", image_location_1, image_file_1->getSize());

        auto function_1 = wrench::FunctionManager::createFunction("Function_1", lambda, image_1);
        auto input_1 = std::make_shared<MyFunctionInput>(1, 2);
        auto registered_function_1 = function_manager->registerFunction(function_1, this->compute_service, 100,
                                                                        12 * GB, 1 * MB, 10 * MB, 1 * MB);

        // Place invocations, but only 3 should be able to run at a time
        std::vector<std::shared_ptr<wrench::Invocation>> invocations;
        unsigned long num_invocations = 5;
        invocations.reserve(num_invocations);
        for (unsigned long i = 0; i < num_invocations; i++) {
            auto invocation = function_manager->invokeFunction(registered_function_1, this->compute_service, input_1);
            invocations.push_back(invocation);
            wrench::Simulation::sleep(0.1);
        }

        function_manager->wait_all(invocations);

        // for (int i=0; i < num_invocations; i++) {
        //     std::cerr << "INVOCATION #" << i << ": START TIME - COMPLETION TIME: " << ": " << invocations.at(i)->getFunctionStartDate() << " -> " << invocations.at(i)->getFunctionEndDate() << std::endl;
        // }
        for (unsigned long i = 0; i < num_invocations; i += 3) {
            double start_date = invocations.at(i)->getDispatchDate();
            double end_date = invocations.at(i)->getFunctionEndDate();
            for (unsigned long j = i + 1; j < std::min<unsigned long>(i + 3, num_invocations); j++) {
                if (fabs(start_date - invocations.at(j)->getDispatchDate()) > 0.1) {
                    throw std::runtime_error("Unexpected execution pattern");
                }
                if (fabs(end_date - invocations.at(j)->getFunctionEndDate()) > 0.1) {
                    throw std::runtime_error("Unexpected execution pattern");
                }
            }
        }

        return 0;
    }
};

TEST_F(ServerlessTimingTest, DiskPressureDueToInvocations) {
    std::vector<std::shared_ptr<wrench::ServerlessScheduler>> schedulers = {
        std::make_shared<wrench::FCFSServerlessScheduler>(),
        // std::make_shared<wrench::RandomServerlessScheduler>(),
        // std::make_shared<wrench::WorkloadBalancingServerlessScheduler>(),
    };
    for (auto& scheduler : schedulers) {
        DO_TEST_WITH_FORK_ONE_ARG(do_DiskPressureDueToInvocations_test, scheduler);
    }
}

void ServerlessTimingTest::do_DiskPressureDueToInvocations_test(
    const std::shared_ptr<wrench::ServerlessScheduler>& scheduler) {
    int argc = 1;
    auto argv = (char**)calloc(argc, sizeof(char*));
    argv[0] = strdup("unit_test");
    // argv[1] = strdup("--wrench-full-log");

    auto simulation = wrench::Simulation::createSimulation();
    simulation->init(&argc, argv);

    simulation->instantiatePlatform(this->platform_file_path);

    auto storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
        "UserHost", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "0"}}, {}));

    std::vector<std::string> compute_nodes = {"ServerlessComputeNodeSmallDisk"};
    auto serverless_provider = simulation->add(new wrench::ServerlessComputeService(
        "ServerlessHeadNode", "/", compute_nodes, scheduler, {}, {}));

    std::string user_host = "UserHost";
    auto wms = simulation->add(
        new ServerlessDiskPressureDueToInvocationsController(this, user_host, serverless_provider, storage_service));

    simulation->launch();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  HOT START TEST                                                  **/
/**********************************************************************/

class ServerlessHotStartController : public wrench::ExecutionController {
public:
    ServerlessHotStartController(ServerlessTimingTest* test,
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
        auto function_manager = this->createFunctionManager();

        // Create a function
        std::function lambda = [](const std::shared_ptr<wrench::FunctionInput>& input,
                                  const std::shared_ptr<wrench::StorageService>& service) -> std::shared_ptr<
            wrench::FunctionOutput> {
            auto real_input = std::dynamic_pointer_cast<MyFunctionInput>(input);
            wrench::Simulation::sleep(10);
            return std::make_shared<MyFunctionOutput>("Processed!");
        };

        // Register that function with an image file that will fill up the disk
        auto image_file_1 = wrench::Simulation::addFile("image_file_1", 60 * GB);
        auto image_location_1 = wrench::FileLocation::LOCATION(this->storage_service, image_file_1);
        wrench::StorageService::createFileAtLocation(image_location_1);
        auto image_1 = wrench::FunctionManager::createImage("my_image_1", image_location_1, image_file_1->getSize());

        auto function_1 = wrench::FunctionManager::createFunction("Function_1", lambda, image_1);
        auto input_1 = std::make_shared<MyFunctionInput>(1, 2);
        auto registered_function_1 = function_manager->registerFunction(function_1, this->compute_service, 100,
                                                                        30 * GB, 1 * MB, 10 * MB, 1 * MB);

        // Place an invocation and wait for it
        auto inv1 = function_manager->invokeFunction(registered_function_1, this->compute_service, input_1);
        function_manager->wait_one(inv1);

        // Place another invocation and wait for it
        auto inv2 = function_manager->invokeFunction(registered_function_1, this->compute_service, input_1);
        function_manager->wait_one(inv2);

        // Sleep more than the idle timeout
        wrench::Simulation::sleep(10 +
            compute_service->getPropertyValueAsDouble(
                wrench::ServerlessComputeServiceProperty::CONTAINER_IDLE_TIMEOUT));

        // Place another invocation and wait for it
        auto inv3 = function_manager->invokeFunction(registered_function_1, this->compute_service, input_1);
        function_manager->wait_one(inv3);

        if (std::abs(
            (inv1->getFunctionStartDate() - inv1->getDispatchDate()) - compute_service->getPropertyValueAsDouble(
                wrench::ServerlessComputeServiceProperty::CONTAINER_STARTUP_OVERHEAD)) > EPSILON) {
            throw std::runtime_error(
                "Unexpected invocation #1 times: " + std::to_string(inv1->getDispatchDate()) + " " + std::to_string(
                    inv1->getFunctionStartDate()));
        }

        if (std::abs(inv2->getFunctionStartDate() - inv2->getDispatchDate()) > EPSILON) {
            throw std::runtime_error(
                "Unexpected invocation #2 times: " + std::to_string(inv2->getDispatchDate()) + " " + std::to_string(
                    inv2->getFunctionStartDate()));
        }

        if (std::abs(
            (inv3->getFunctionStartDate() - inv3->getDispatchDate()) - compute_service->getPropertyValueAsDouble(
                wrench::ServerlessComputeServiceProperty::CONTAINER_STARTUP_OVERHEAD)) > EPSILON) {
            throw std::runtime_error(
                "Unexpected invocation #3 times: " + std::to_string(inv3->getDispatchDate()) + " " + std::to_string(
                    inv3->getFunctionStartDate()));
        }

        return 0;
    }
};

TEST_F(ServerlessTimingTest, HotStart) {
    std::vector<std::shared_ptr<wrench::ServerlessScheduler>> schedulers = {
        std::make_shared<wrench::FCFSServerlessScheduler>(),
        // std::make_shared<wrench::RandomServerlessScheduler>(),
        // std::make_shared<wrench::WorkloadBalancingServerlessScheduler>(),
    };
    for (auto& scheduler : schedulers) {
        DO_TEST_WITH_FORK_ONE_ARG(do_HotStart_test, scheduler);
    }
}

void ServerlessTimingTest::do_HotStart_test(
    const std::shared_ptr<wrench::ServerlessScheduler>& scheduler) {
    int argc = 1;
    auto argv = (char**)calloc(argc, sizeof(char*));
    argv[0] = strdup("unit_test");
    // argv[1] = strdup("--wrench-full-log");

    auto simulation = wrench::Simulation::createSimulation();
    simulation->init(&argc, argv);

    simulation->instantiatePlatform(this->platform_file_path);

    auto storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
        "UserHost", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "0"}}, {}));

    std::vector<std::string> compute_nodes = {"ServerlessComputeNodeSmallDisk"};
    auto serverless_provider = simulation->add(new wrench::ServerlessComputeService(
        "ServerlessHeadNode", "/", compute_nodes, scheduler,
        {
            {wrench::ServerlessComputeServiceProperty::CONTAINER_STARTUP_OVERHEAD, "5.0"},
            {wrench::ServerlessComputeServiceProperty::CONTAINER_IDLE_TIMEOUT, "60.00"}
        },
        {}));

    std::string user_host = "UserHost";
    auto wms = simulation->add(
        new ServerlessHotStartController(this, user_host, serverless_provider, storage_service));

    simulation->launch();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  EVICTION FROM DISK TEST                                         **/
/**********************************************************************/

class ServerlessSimpleImageEvictionFromDiskController : public wrench::ExecutionController {
public:
    ServerlessSimpleImageEvictionFromDiskController(ServerlessTimingTest* test,
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
        auto function_manager = this->createFunctionManager();

        // Create a function
        std::function lambda = [](const std::shared_ptr<wrench::FunctionInput>& input,
                                  const std::shared_ptr<wrench::StorageService>& service) -> std::shared_ptr<
            wrench::FunctionOutput> {
            auto real_input = std::dynamic_pointer_cast<MyFunctionInput>(input);
            wrench::Simulation::sleep(10);
            return std::make_shared<MyFunctionOutput>("Processed!");
        };

        // Register that function with a 50GB image file that takes %50 of the node disk space
        auto image_file_1 = wrench::Simulation::addFile("image_file_1", 50 * GB);
        auto image_location_1 = wrench::FileLocation::LOCATION(this->storage_service, image_file_1);
        wrench::StorageService::createFileAtLocation(image_location_1);
        auto image_1 = wrench::FunctionManager::createImage("my_image_1", image_location_1, image_file_1->getSize());

        auto function_1 = wrench::FunctionManager::createFunction("Function_1", lambda, image_1);
        auto input_1 = std::make_shared<MyFunctionInput>(1, 2);
        auto registered_function_1 = function_manager->registerFunction(function_1, this->compute_service, 100,
                                                                        30 * GB, 1 * MB, 10 * MB, 1 * MB);

        // Place an invocation to function 1 and wait for it
        auto inv1 = function_manager->invokeFunction(registered_function_1, this->compute_service, input_1);
        function_manager->wait_one(inv1);
        // Keep track of time for invocation when image is not on disk at compute node
        auto inv1_elapsed = inv1->getFunctionEndDate() - inv1->getSubmitDate();

        // At this point, the image is on disk at the compute node (and in RAM, but we don't care)
        // Place an invocation to function 1 and wait for it
        auto inv2 = function_manager->invokeFunction(registered_function_1, this->compute_service, input_1);
        function_manager->wait_one(inv2);
        // Keep track of time for invocation when image is on disk at compute node
        auto inv2_elapsed = inv2->getFunctionEndDate() - inv2->getSubmitDate();

        // Register that function with ANOTHER 50GB image file that takes %50 of the node disk space
        auto image_file_2 = wrench::Simulation::addFile("image_file_2", 50 * GB);
        auto image_location_2 = wrench::FileLocation::LOCATION(this->storage_service, image_file_2);
        wrench::StorageService::createFileAtLocation(image_location_2);
        auto image_2 = wrench::FunctionManager::createImage("my_image_2", image_location_2, image_file_2->getSize());

        auto function_2 = wrench::FunctionManager::createFunction("Function_2", lambda, image_2);
        auto input_2 = std::make_shared<MyFunctionInput>(1, 2);
        auto registered_function_2 = function_manager->registerFunction(function_2, this->compute_service, 100,
                                                                        30 * GB, 1 * MB, 10 * MB, 1 * MB);

        // Place an invocation to function 2 and wait for it
        auto inv3 = function_manager->invokeFunction(registered_function_2, this->compute_service, input_2);
        function_manager->wait_one(inv3);
        auto inv3_elapsed = inv3->getFunctionEndDate() - inv3->getSubmitDate();

        // At this point image_1 should have been kicked out of the node disk (and the node RAM!)
        // Place an invocation to function 1 and wait for it
        auto inv4 = function_manager->invokeFunction(registered_function_1, this->compute_service, input_1);
        function_manager->wait_one(inv4);
        // Keep track of time for invocation when image is not on disk at compute node
        auto inv4_elapsed = inv4->getFunctionEndDate() - inv4->getSubmitDate();

        // std::cerr << "INV1: " << inv1->getSubmitDate() << " --> " << inv1->getFunctionEndDate() << std::endl;
        // std::cerr << "INV2: " << inv2->getSubmitDate() << " --> " << inv2->getFunctionEndDate() << "(" << inv2->
        //     getFunctionEndDate() - inv2->getSubmitDate() << ")" << std::endl;
        // std::cerr << "INV3: " << inv3->getSubmitDate() << " --> " << inv3->getFunctionEndDate() << "(" << inv3->
        //     getFunctionEndDate() - inv3->getSubmitDate() << ")" << std::endl;
        // std::cerr << "INV4: " << inv4->getSubmitDate() << " --> " << inv4->getFunctionEndDate() << "(" << inv4->
        //     getFunctionEndDate() - inv4->getSubmitDate() << ")" << std::endl;

        double expected_inv1_elapsed = (50.0 * GB) / (100.0 * MB) + (50.0 * GB) / (100.0 * MB) + 10;
        double expected_inv2_elapsed = 10;
        double expected_inv3_elapsed = (50.0 * GB) / (100.0 * MB) + (50.0 * GB) / (100.0 * MB) + 10;
        double expected_inv4_elapsed = (50.0 * GB) / (100.0 * MB) + (50.0 * GB) / (100.0 * MB) + 10;

        if (std::abs(inv1_elapsed - expected_inv1_elapsed) > EPSILON) {
            throw std::runtime_error(
                "Unexpected inv1 elapsed: " + std::to_string(inv1_elapsed) + " as opposed to " + std::to_string(
                    expected_inv1_elapsed));
        }
        if (std::abs(inv2_elapsed - expected_inv2_elapsed) > EPSILON) {
            throw std::runtime_error(
                "Unexpected inv2 elapsed: " + std::to_string(inv2_elapsed) + " as opposed to " + std::to_string(
                    expected_inv2_elapsed));
        }
        if (std::abs(inv3_elapsed - expected_inv3_elapsed) > EPSILON) {
            throw std::runtime_error(
                "Unexpected inv3 elapsed: " + std::to_string(inv3_elapsed) + " as opposed to " + std::to_string(
                    expected_inv3_elapsed));
        }
        if (std::abs(inv4_elapsed - expected_inv4_elapsed) > EPSILON) {
            throw std::runtime_error(
                "Unexpected inv4 elapsed: " + std::to_string(inv4_elapsed) + " as opposed to " + std::to_string(
                    expected_inv4_elapsed));
        }

        return 0;
    }
};

TEST_F(ServerlessTimingTest, SimpleImageEvictionFromDisk) {
    std::vector<std::shared_ptr<wrench::ServerlessScheduler>> schedulers = {
        std::make_shared<wrench::FCFSServerlessScheduler>(),
        // std::make_shared<wrench::RandomServerlessScheduler>(),
        // std::make_shared<wrench::WorkloadBalancingServerlessScheduler>(),
    };
    for (auto& scheduler : schedulers) {
        DO_TEST_WITH_FORK_ONE_ARG(do_SimpleImageEvictionFromDisk_test, scheduler);
    }
}

void ServerlessTimingTest::do_SimpleImageEvictionFromDisk_test(
    const std::shared_ptr<wrench::ServerlessScheduler>& scheduler) {
    int argc = 1;
    auto argv = (char**)calloc(argc, sizeof(char*));
    argv[0] = strdup("unit_test");
    // argv[1] = strdup("--wrench-full-log");

    auto simulation = wrench::Simulation::createSimulation();
    simulation->init(&argc, argv);

    simulation->instantiatePlatform(this->platform_file_path);

    auto storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
        "UserHost", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "0"}}, {}));

    std::vector<std::string> compute_nodes = {"ServerlessComputeNodeSmallDisk"};
    auto serverless_provider = simulation->add(new wrench::ServerlessComputeService(
        "ServerlessHeadNode", "/", compute_nodes, scheduler,
        {
            {wrench::ServerlessComputeServiceProperty::CONTAINER_STARTUP_OVERHEAD, "0.0"},
            {wrench::ServerlessComputeServiceProperty::CONTAINER_IDLE_TIMEOUT, "0.00"},
            {wrench::ServerlessComputeServiceProperty::SIMULATE_REMOTE_IMAGE_DOWNLOADS, "false"},
        },
        {}));

    std::string user_host = "UserHost";
    auto wms = simulation->add(
        new ServerlessSimpleImageEvictionFromDiskController(this, user_host, serverless_provider, storage_service));

    simulation->launch();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  EVICTION FROM RAM TEST                                         **/
/**********************************************************************/

class ServerlessSimpleImageEvictionFromRAMController : public wrench::ExecutionController {
public:
    ServerlessSimpleImageEvictionFromRAMController(ServerlessTimingTest* test,
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
        auto function_manager = this->createFunctionManager();

        // Create a function
        std::function lambda = [](const std::shared_ptr<wrench::FunctionInput>& input,
                                  const std::shared_ptr<wrench::StorageService>& service) -> std::shared_ptr<
            wrench::FunctionOutput> {
            auto real_input = std::dynamic_pointer_cast<MyFunctionInput>(input);
            wrench::Simulation::sleep(10);
            return std::make_shared<MyFunctionOutput>("Processed!");
        };

        // Register that function with a 32GB image file that takes %50 of the node disk space
        auto image_file_1 = wrench::Simulation::addFile("image_file_1", 50 * GB);
        auto image_location_1 = wrench::FileLocation::LOCATION(this->storage_service, image_file_1);
        wrench::StorageService::createFileAtLocation(image_location_1);
        auto image_1 = wrench::FunctionManager::createImage("my_image_1", image_location_1, image_file_1->getSize());

        auto function_1 = wrench::FunctionManager::createFunction("Function_1", lambda, image_1);
        auto input_1 = std::make_shared<MyFunctionInput>(1, 2);
        auto registered_function_1 = function_manager->registerFunction(function_1, this->compute_service, 100,
                                                                        30 * GB, 10 * MB, 10 * MB, 1 * MB);

        // Place an invocation to function 1 and wait for it
        auto inv1 = function_manager->invokeFunction(registered_function_1, this->compute_service, input_1);
        function_manager->wait_one(inv1);
        // Keep track of time for invocation when image is not on disk at compute node
        auto inv1_elapsed = inv1->getFunctionEndDate() - inv1->getSubmitDate();

        // Place an invocation to function 2 and wait for it
        auto image_file_2 = wrench::Simulation::addFile("image_file_2", 50 * GB);
        auto image_location_2 = wrench::FileLocation::LOCATION(this->storage_service, image_file_2);
        wrench::StorageService::createFileAtLocation(image_location_2);
        auto image_2 = wrench::FunctionManager::createImage("my_image_2", image_location_2, image_file_2->getSize());

        auto function_2 = wrench::FunctionManager::createFunction("Function_2", lambda, image_2);
        auto input_2 = std::make_shared<MyFunctionInput>(1, 2);
        auto registered_function_2 = function_manager->registerFunction(function_2, this->compute_service, 100,
                                                                        30 * GB, 10 * MB, 10 * MB, 1 * MB);

        auto inv2 = function_manager->invokeFunction(registered_function_2, this->compute_service, input_2);
        function_manager->wait_one(inv2);

        // At this point, the image has been kicked out
        auto inv3 = function_manager->invokeFunction(registered_function_1, this->compute_service, input_1);
        function_manager->wait_one(inv3);
        // Keep track of time for invocation when image is on disk at compute node
        auto inv3_elapsed = inv3->getFunctionEndDate() - inv3->getSubmitDate();

        // Now it should still be in RAM
        auto inv4 = function_manager->invokeFunction(registered_function_1, this->compute_service, input_1);
        function_manager->wait_one(inv4);
        // Keep track of time for invocation when image is on disk at compute node
        auto inv4_elapsed = inv4->getFunctionEndDate() - inv4->getSubmitDate();

        // Invoke function 2 again
        auto inv5 = function_manager->invokeFunction(registered_function_2, this->compute_service, input_2);
        function_manager->wait_one(inv5);

        // At this point image_1 should have been kicked out of the node disk again
        // Place an invocation to function 1 and wait for it
        auto inv6 = function_manager->invokeFunction(registered_function_1, this->compute_service, input_1);
        function_manager->wait_one(inv6);
        // Keep track of time for invocation when image is not on disk at compute node
        auto inv6_elapsed = inv6->getFunctionEndDate() - inv6->getSubmitDate();

        // std::cerr << "INV1: " << inv1_elapsed << std::endl;
        // std::cerr << "INV3: " << inv3_elapsed << std::endl;
        // std::cerr << "INV4: " << inv4_elapsed << std::endl;
        // std::cerr << "INV6: " << inv6_elapsed << std::endl;

        if (std::abs(inv3_elapsed - inv6_elapsed) > EPSILON) {
            throw std::runtime_error(
                "Unexpected times: inv3: " + std::to_string(inv3_elapsed) + "   inv6: " + std::to_string(inv6_elapsed));
        }
        if (std::abs((inv4_elapsed + (50 * GB / (100 * MB))) - inv6_elapsed) > EPSILON) {
            throw std::runtime_error(
                "Unexpected times: inv4: " + std::to_string(inv4_elapsed) + "   inv6: " + std::to_string(inv6_elapsed));
        }
        return 0;
    }
};

TEST_F(ServerlessTimingTest, SimpleImageEvictionFromRAM) {
    std::vector<std::shared_ptr<wrench::ServerlessScheduler>> schedulers = {
        std::make_shared<wrench::FCFSServerlessScheduler>(),
        // std::make_shared<wrench::RandomServerlessScheduler>(),
        // std::make_shared<wrench::WorkloadBalancingServerlessScheduler>(),
    };
    for (auto& scheduler : schedulers) {
        DO_TEST_WITH_FORK_ONE_ARG(do_SimpleImageEvictionFromRAM_test, scheduler);
    }
}

void ServerlessTimingTest::do_SimpleImageEvictionFromRAM_test(
    const std::shared_ptr<wrench::ServerlessScheduler>& scheduler) {
    int argc = 1;
    auto argv = (char**)calloc(argc, sizeof(char*));
    argv[0] = strdup("unit_test");
    // argv[1] = strdup("--wrench-full-log");

    auto simulation = wrench::Simulation::createSimulation();
    simulation->init(&argc, argv);

    simulation->instantiatePlatform(this->platform_file_path);

    auto storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
        "UserHost", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "0"}}, {}));

    std::vector<std::string> compute_nodes = {"ServerlessComputeNode1"};
    auto serverless_provider = simulation->add(new wrench::ServerlessComputeService(
        "ServerlessHeadNode", "/", compute_nodes, scheduler,
        {
            {wrench::ServerlessComputeServiceProperty::CONTAINER_STARTUP_OVERHEAD, "0.0"},
            {wrench::ServerlessComputeServiceProperty::CONTAINER_IDLE_TIMEOUT, "0.0"}
        },
        {}));

    std::string user_host = "UserHost";
    auto wms = simulation->add(
        new ServerlessSimpleImageEvictionFromRAMController(this, user_host, serverless_provider, storage_service));

    simulation->launch();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  TWO IDLE CONTAINERS TEST                                        **/
/**********************************************************************/

class ServerlessTwoIdleContainersController : public wrench::ExecutionController {
public:
    ServerlessTwoIdleContainersController(ServerlessTimingTest* test,
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
        auto function_manager = this->createFunctionManager();

        // Create a function
        std::function lambda = [](const std::shared_ptr<wrench::FunctionInput>& input,
                                  const std::shared_ptr<wrench::StorageService>& service) -> std::shared_ptr<
            wrench::FunctionOutput> {
            auto real_input = std::dynamic_pointer_cast<MyFunctionInput>(input);
            wrench::Simulation::sleep(10);
            return std::make_shared<MyFunctionOutput>("Processed!");
        };

        // Register that function with a 32GB image file that takes %50 of the node disk space
        auto image_file_1 = wrench::Simulation::addFile("image_file_1", 50 * GB);
        auto image_location_1 = wrench::FileLocation::LOCATION(this->storage_service, image_file_1);
        wrench::StorageService::createFileAtLocation(image_location_1);
        auto image_1 = wrench::FunctionManager::createImage("my_image_1", image_location_1, image_file_1->getSize());

        auto function_1 = wrench::FunctionManager::createFunction("Function_1", lambda, image_1);
        auto input_1 = std::make_shared<MyFunctionInput>(1, 2);
        auto registered_function_1 = function_manager->registerFunction(function_1, this->compute_service, 100,
                                                                        30 * GB, 10 * MB, 10 * MB, 1 * MB);

        // Place two invocation to function 1 and wait for them
        auto inv1 = function_manager->invokeFunction(registered_function_1, this->compute_service, input_1);
        auto inv2 = function_manager->invokeFunction(registered_function_1, this->compute_service, input_1);
        function_manager->wait_one(inv1);
        function_manager->wait_one(inv2);
        auto inv1_elapsed = inv1->getFunctionEndDate() - inv1->getSubmitDate();
        auto inv2_elapsed = inv2->getFunctionEndDate() - inv2->getSubmitDate();

        // Do it again immediately
        auto inv3 = function_manager->invokeFunction(registered_function_1, this->compute_service, input_1);
        auto inv4 = function_manager->invokeFunction(registered_function_1, this->compute_service, input_1);
        function_manager->wait_one(inv3);
        function_manager->wait_one(inv4);
        auto inv3_elapsed = inv3->getFunctionEndDate() - inv3->getSubmitDate();
        auto inv4_elapsed = inv4->getFunctionEndDate() - inv4->getSubmitDate();

        // Do it again much later
        wrench::Simulation::sleep(1000);
        auto inv5 = function_manager->invokeFunction(registered_function_1, this->compute_service, input_1);
        auto inv6 = function_manager->invokeFunction(registered_function_1, this->compute_service, input_1);
        function_manager->wait_one(inv5);
        function_manager->wait_one(inv6);
        auto inv5_elapsed = inv5->getFunctionEndDate() - inv5->getSubmitDate();
        auto inv6_elapsed = inv6->getFunctionEndDate() - inv6->getSubmitDate();

        // std::cerr << "INV1: " << inv1_elapsed << std::endl;
        // std::cerr << "INV2: " << inv2_elapsed << std::endl;
        // std::cerr << "INV3: " << inv3_elapsed << std::endl;
        // std::cerr << "INV4: " << inv4_elapsed << std::endl;
        // std::cerr << "INV5: " << inv5_elapsed << std::endl;
        // std::cerr << "INV6: " << inv6_elapsed << std::endl;

        if (std::abs(inv1_elapsed - inv2_elapsed) > EPSILON) {
            throw std::runtime_error(
                "Unexpected elapsed times: inv1: " + std::to_string(inv1_elapsed) + "   inv2: " + std::to_string(
                    inv2_elapsed));
        }
        if (std::abs(inv3_elapsed - inv4_elapsed) > EPSILON) {
            throw std::runtime_error(
                "Unexpected elapsed times: inv1: " + std::to_string(inv3_elapsed) + "   inv2: " + std::to_string(
                    inv4_elapsed));
        }
        if (std::abs(inv5_elapsed - inv6_elapsed) > EPSILON) {
            throw std::runtime_error(
                "Unexpected elapsed times: inv1: " + std::to_string(inv5_elapsed) + "   inv2: " + std::to_string(
                    inv6_elapsed));
        }

        if (std::abs(inv3_elapsed - 10.0) > EPSILON) {
            throw std::runtime_error("Unexpected elapsed time: inv3: " + std::to_string(inv3_elapsed));
        }

        if (std::abs(
            inv5_elapsed - (10.0 + this->compute_service->getPropertyValueAsDouble(
                wrench::ServerlessComputeServiceProperty::CONTAINER_STARTUP_OVERHEAD))) > EPSILON) {
            throw std::runtime_error("Unexpected elapsed time: inv5: " + std::to_string(inv5_elapsed));
        }

        return 0;
    }
};

TEST_F(ServerlessTimingTest, TwoIdleContainers) {
    std::vector<std::shared_ptr<wrench::ServerlessScheduler>> schedulers = {
        std::make_shared<wrench::FCFSServerlessScheduler>(),
        // std::make_shared<wrench::RandomServerlessScheduler>(),
        // std::make_shared<wrench::WorkloadBalancingServerlessScheduler>(),
    };
    for (auto& scheduler : schedulers) {
        DO_TEST_WITH_FORK_ONE_ARG(do_TwoIdleContainers_test, scheduler);
    }
}

void ServerlessTimingTest::do_TwoIdleContainers_test(
    const std::shared_ptr<wrench::ServerlessScheduler>& scheduler) {
    int argc = 1;
    auto argv = (char**)calloc(argc, sizeof(char*));
    argv[0] = strdup("unit_test");
    // argv[1] = strdup("--wrench-full-log");


    auto simulation = wrench::Simulation::createSimulation();
    simulation->init(&argc, argv);

    simulation->instantiatePlatform(this->platform_file_path);

    auto storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
        "UserHost", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "0"}}, {}));

    std::vector<std::string> compute_nodes = {"ServerlessComputeNode1"};
    auto serverless_provider = simulation->add(new wrench::ServerlessComputeService(
        "ServerlessHeadNode", "/", compute_nodes, scheduler,
        {
            {wrench::ServerlessComputeServiceProperty::CONTAINER_STARTUP_OVERHEAD, "10.0"},
            {wrench::ServerlessComputeServiceProperty::CONTAINER_IDLE_TIMEOUT, "100.0"}
        },
        {}));

    std::string user_host = "UserHost";
    auto wms = simulation->add(
        new ServerlessTwoIdleContainersController(this, user_host, serverless_provider, storage_service));

    simulation->launch();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  ONE IDLE CONTAINER TWO INVOCATIONS TEST                         **/
/**********************************************************************/

class ServerlessOneIdleContainerTwoInvocationsController : public wrench::ExecutionController {
public:
    ServerlessOneIdleContainerTwoInvocationsController(ServerlessTimingTest* test,
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
        auto function_manager = this->createFunctionManager();

        // Create a function
        std::function lambda = [](const std::shared_ptr<wrench::FunctionInput>& input,
                                  const std::shared_ptr<wrench::StorageService>& service) -> std::shared_ptr<
            wrench::FunctionOutput> {
            auto real_input = std::dynamic_pointer_cast<MyFunctionInput>(input);
            wrench::Simulation::sleep(10);
            return std::make_shared<MyFunctionOutput>("Processed!");
        };

        // Register that function with a 32GB image file that takes %50 of the node disk space
        auto image_file_1 = wrench::Simulation::addFile("image_file_1", 50 * GB);
        auto image_location_1 = wrench::FileLocation::LOCATION(this->storage_service, image_file_1);
        wrench::StorageService::createFileAtLocation(image_location_1);
        auto image_1 = wrench::FunctionManager::createImage("my_image_1", image_location_1, image_file_1->getSize());

        auto function_1 = wrench::FunctionManager::createFunction("Function_1", lambda, image_1);
        auto input_1 = std::make_shared<MyFunctionInput>(1, 2);
        auto registered_function_1 = function_manager->registerFunction(function_1, this->compute_service, 100,
                                                                        30 * GB, 10 * MB, 10 * MB, 1 * MB);

        // Place one invocation, so that there will be one idle container
        auto inv1 = function_manager->invokeFunction(registered_function_1, this->compute_service, input_1);
        function_manager->wait_one(inv1);
        auto inv1_elapsed = inv1->getFunctionEndDate() - inv1->getSubmitDate();


        // Place two invocations, one of them should run on a idle container, the other should pay the container startup overhead
        auto inv2 = function_manager->invokeFunction(registered_function_1, this->compute_service, input_1);
        auto inv3 = function_manager->invokeFunction(registered_function_1, this->compute_service, input_1);
        function_manager->wait_one(inv2);
        function_manager->wait_one(inv3);
        auto inv2_elapsed = inv2->getFunctionEndDate() - inv2->getSubmitDate();
        auto inv3_elapsed = inv3->getFunctionEndDate() - inv3->getSubmitDate();


        // std::cerr << "INV1: " << inv1_elapsed << std::endl;
        // std::cerr << "INV2: " << inv2_elapsed << std::endl;
        // std::cerr << "INV3: " << inv3_elapsed << std::endl;
        // std::cerr << "INV4: " << inv4_elapsed << std::endl;
        // std::cerr << "INV5: " << inv5_elapsed << std::endl;
        // std::cerr << "INV6: " << inv6_elapsed << std::endl;

        if (std::abs(inv2->getDispatchDate() - inv3->getDispatchDate()) > EPSILON) {
            throw std::runtime_error(
                "Unexpected dispatch times: inv2: " + std::to_string(inv2->getDispatchDate()) + "   inv3: " +
                std::to_string(inv3->getDispatchDate()));
        }
        if (std::abs(
            (inv2_elapsed + this->compute_service->getPropertyValueAsDouble(
                wrench::ServerlessComputeServiceProperty::CONTAINER_STARTUP_OVERHEAD)) - (inv3_elapsed)) > EPSILON) {
            throw std::runtime_error(
                "Unexpected elapsed times: inv2: " + std::to_string(inv2_elapsed) + "   inv3: dispatch: " +
                std::to_string(inv3_elapsed));
        }


        return 0;
    }
};

TEST_F(ServerlessTimingTest, OneIdleContainerTwoInvocations) {
    std::vector<std::shared_ptr<wrench::ServerlessScheduler>> schedulers = {
        std::make_shared<wrench::FCFSServerlessScheduler>(),
        // std::make_shared<wrench::RandomServerlessScheduler>(),
        // std::make_shared<wrench::WorkloadBalancingServerlessScheduler>(),
    };
    for (auto& scheduler : schedulers) {
        DO_TEST_WITH_FORK_ONE_ARG(do_OneIdleContainerTwoInvocations_test, scheduler);
    }
}

void ServerlessTimingTest::do_OneIdleContainerTwoInvocations_test(
    const std::shared_ptr<wrench::ServerlessScheduler>& scheduler) {
    int argc = 1;
    auto argv = (char**)calloc(argc, sizeof(char*));
    argv[0] = strdup("unit_test");
    // argv[1] = strdup("--wrench-full-log");


    auto simulation = wrench::Simulation::createSimulation();
    simulation->init(&argc, argv);

    simulation->instantiatePlatform(this->platform_file_path);

    auto storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
        "UserHost", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "0"}}, {}));

    std::vector<std::string> compute_nodes = {"ServerlessComputeNode1"};
    auto serverless_provider = simulation->add(new wrench::ServerlessComputeService(
        "ServerlessHeadNode", "/", compute_nodes, scheduler,
        {
            {wrench::ServerlessComputeServiceProperty::CONTAINER_STARTUP_OVERHEAD, "10.0"},
            {wrench::ServerlessComputeServiceProperty::CONTAINER_IDLE_TIMEOUT, "100.0"}
        },
        {}));

    std::string user_host = "UserHost";
    auto wms = simulation->add(
        new ServerlessOneIdleContainerTwoInvocationsController(this, user_host, serverless_provider, storage_service));

    simulation->launch();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  TMP STORAGE CLEARING TEST                                       **/
/**********************************************************************/

class ServerlessTmpStorageClearingController : public wrench::ExecutionController {
public:
    ServerlessTmpStorageClearingController(ServerlessTimingTest* test,
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
        auto function_manager = this->createFunctionManager();

        // Create a function
        std::function lambda = [](const std::shared_ptr<wrench::FunctionInput>& input,
                                  const std::shared_ptr<wrench::StorageService>& service) -> std::shared_ptr<
            wrench::FunctionOutput> {
            auto free_storage_space_begin = service->getTotalFreeSpace();
            auto real_input = std::dynamic_pointer_cast<MyFunctionInput>(input);
            wrench::StorageService::createFileAtLocation(
                wrench::FileLocation::LOCATION(service, wrench::Simulation::addTmpFile(1 * GB)));
            wrench::Simulation::sleep(10);
            auto free_storage_space_end = service->getTotalFreeSpace();
            return std::make_shared<MyFunctionOutput>(
                "FREE SPACE: " + std::to_string(free_storage_space_begin) + " " +
                std::to_string(free_storage_space_end));
        };

        // Register that function with a 32GB image file that takes %50 of the node disk space
        auto image_file_1 = wrench::Simulation::addFile("image_file_1", 50 * GB);
        auto image_location_1 = wrench::FileLocation::LOCATION(this->storage_service, image_file_1);
        wrench::StorageService::createFileAtLocation(image_location_1);
        auto image_1 = wrench::FunctionManager::createImage("my_image_1", image_location_1, image_file_1->getSize());

        auto function_1 = wrench::FunctionManager::createFunction("Function_1", lambda, image_1);
        auto input_1 = std::make_shared<MyFunctionInput>(1, 2);
        auto registered_function_1 = function_manager->registerFunction(function_1, this->compute_service, 100,
                                                                        30 * GB, 10 * MB, 10 * MB, 1 * MB);

        // Place one invocation, so that there will be one idle container to reuse
        auto inv1 = function_manager->invokeFunction(registered_function_1, this->compute_service, input_1);
        function_manager->wait_one(inv1);

        // Place another invocation that will reuse the idle container
        auto inv2 = function_manager->invokeFunction(registered_function_1, this->compute_service, input_1);
        function_manager->wait_one(inv2);

        // Both invocations should have the same output
        auto inv1_output = std::dynamic_pointer_cast<MyFunctionOutput>(inv1->getOutput());
        auto inv2_output = std::dynamic_pointer_cast<MyFunctionOutput>(inv2->getOutput());

        if (inv1_output->toString() != inv2_output->toString()) {
            throw std::runtime_error(
                "Both invocations should produce the same output, instead: " + inv1_output->toString() + " != " +
                inv2_output->toString());
        }

        return 0;
    }
};

TEST_F(ServerlessTimingTest, TmpStorageClearing) {
    std::vector<std::shared_ptr<wrench::ServerlessScheduler>> schedulers = {
        std::make_shared<wrench::FCFSServerlessScheduler>(),
        std::make_shared<wrench::RandomServerlessScheduler>(0),
        // std::make_shared<wrench::WorkloadBalancingServerlessScheduler>(),
    };
    for (auto& scheduler : schedulers) {
        DO_TEST_WITH_FORK_ONE_ARG(do_TmpStorageClearing_test, scheduler);
    }
}

void ServerlessTimingTest::do_TmpStorageClearing_test(
    const std::shared_ptr<wrench::ServerlessScheduler>& scheduler) {
    int argc = 1;
    auto argv = (char**)calloc(argc, sizeof(char*));
    argv[0] = strdup("unit_test");
    // argv[1] = strdup("--wrench-full-log");

    auto simulation = wrench::Simulation::createSimulation();
    simulation->init(&argc, argv);

    simulation->instantiatePlatform(this->platform_file_path);

    auto storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
        "UserHost", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "0"}}, {}));

    std::vector<std::string> compute_nodes = {"ServerlessComputeNode1"};
    auto serverless_provider = simulation->add(new wrench::ServerlessComputeService(
        "ServerlessHeadNode", "/", compute_nodes, scheduler,
        {
            {wrench::ServerlessComputeServiceProperty::CONTAINER_STARTUP_OVERHEAD, "10.0"},
            {wrench::ServerlessComputeServiceProperty::CONTAINER_IDLE_TIMEOUT, "100.0"}
        },
        {}));

    std::string user_host = "UserHost";
    auto wms = simulation->add(
        new ServerlessTmpStorageClearingController(this, user_host, serverless_provider, storage_service));

    simulation->launch();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  IDLE CONTAINER EVICTION TEST                                    **/
/**********************************************************************/

class ServerlessIdleContainerEvictionController : public wrench::ExecutionController {
public:
    ServerlessIdleContainerEvictionController(ServerlessTimingTest* test,
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
        auto function_manager = this->createFunctionManager();

        // Create a function lambda that sleeps 100 seconds
        double sleep_time = 100.0;
        std::function lambda = [sleep_time](const std::shared_ptr<wrench::FunctionInput>& input,
                                            const std::shared_ptr<wrench::StorageService>& service) -> std::shared_ptr<
            wrench::FunctionOutput> {
            auto real_input = std::dynamic_pointer_cast<MyFunctionInput>(input);
            wrench::Simulation::sleep(sleep_time);
            return std::make_shared<MyFunctionOutput>("output");
        };

        // Compute node has 64GB of RAM, create a 33GB image
        auto image_file = wrench::Simulation::addFile("image_file", 33 * GB);
        auto image_location = wrench::FileLocation::LOCATION(this->storage_service, image_file);
        wrench::StorageService::createFileAtLocation(image_location);
        auto image = wrench::FunctionManager::createImage("my_image", image_location, image_file->getSize());

        // Create 10 functions with these RAM sizes, which will fill up the remaining 31GB of RAM
        auto function = wrench::FunctionManager::createFunction("function_code", lambda, image);
        auto input = std::make_shared<MyFunctionInput>(1, 2);
        std::vector<sg_size_t> function_RAM_sizes = {1, 1, 1, 1, 2, 3, 4, 5, 6, 7};
        std::vector<std::shared_ptr<wrench::RegisteredFunction>> registered_functions;
        registered_functions.reserve(function_RAM_sizes.size());
        for (auto ram_size : function_RAM_sizes) {
            registered_functions.push_back(function_manager->registerFunction(
                function, this->compute_service, 100,
                1 * MB, ram_size * GB, 10 * MB, 1 * MB));
        }

        // Fill up memory by invoking these 10 functions, each once
        std::vector<std::shared_ptr<wrench::Invocation>> invocations;
        for (auto const& rf : registered_functions) {
            invocations.push_back(function_manager->invokeFunction(rf, this->compute_service, input));
            wrench::Simulation::sleep(1);
        }
        function_manager->wait_all(invocations);

        // std::cerr << "\n\n** AT THIS POINT MEMORY SHOULD BE FULL, AND WE PLACE ONE MORE INVOCATIONS **\n\n";

        // At this point, place one invocation for yet another function that needs 11 GB of RAM
        {
            auto rf = function_manager->registerFunction(
                function, this->compute_service, 100,
                1 * MB, 11 * GB, 10 * MB, 1 * MB);
            auto inv = function_manager->invokeFunction(rf, this->compute_service, input);
            function_manager->wait_one(inv);
        }


        // At this point, depending on the idle container eviction policy, different containers should have been evicted,
        std::set<int> indices_of_functions_that_should_have_been_evicted;
        if (this->compute_service->getPropertyValueAsString(
            wrench::ServerlessComputeServiceProperty::IDLE_CONTAINER_EVICTION_POLICY) == "RAM") {
            indices_of_functions_that_should_have_been_evicted = {6, 9};
        } else if (this->compute_service->getPropertyValueAsString(
            wrench::ServerlessComputeServiceProperty::IDLE_CONTAINER_EVICTION_POLICY) == "LRU") {
            indices_of_functions_that_should_have_been_evicted = {0, 1, 2, 3, 4, 5, 6};
        }

        // Double-check that containers that should not have been evicted haven't
        // std::cerr << " ** CHECKING NON-EVICTED CONTAINERS\n";
        for (int idx = 0; idx < 10; idx++) {
            if (indices_of_functions_that_should_have_been_evicted.count(idx) > 0) {
                continue;
            }
            auto inv = function_manager->invokeFunction(registered_functions.at(idx), this->compute_service, input);
            function_manager->wait_one(inv);
            auto elapsed = inv->getFunctionEndDate() - inv->getDispatchDate();
            if (std::abs(elapsed - sleep_time) > EPSILON) {
                throw std::runtime_error(
                    "Container for registered function index " + std::to_string(idx) + " should not have been evicted");
            }
        }
        // Double-check that other containers HAVE been evicted
        // std::cerr << " ** CHECKING EVICTED CONTAINERS\n";
        for (int idx = 0; idx < 10; idx++) {
            if (indices_of_functions_that_should_have_been_evicted.count(idx) == 0) {
                continue;
            }

            auto inv = function_manager->invokeFunction(registered_functions.at(idx), this->compute_service, input);
            function_manager->wait_one(inv);
            auto elapsed = inv->getFunctionEndDate() - inv->getDispatchDate();
            if (std::abs(
                elapsed - (sleep_time + this->compute_service->getPropertyValueAsDouble(
                    wrench::ServerlessComputeServiceProperty::CONTAINER_STARTUP_OVERHEAD))) > EPSILON) {
                throw std::runtime_error(
                    "Container for registered function index " + std::to_string(idx) + " should have been evicted");
            }
        }

        return 0;
    }
};

TEST_F(ServerlessTimingTest, IdleContainerEviction) {
    std::vector<std::shared_ptr<wrench::ServerlessScheduler>> schedulers = {
        std::make_shared<wrench::FCFSServerlessScheduler>(),
        // std::make_shared<wrench::RandomServerlessScheduler>(0),
        // std::make_shared<wrench::WorkloadBalancingServerlessScheduler>(),
    };
    for (auto& scheduler : schedulers) {
        DO_TEST_WITH_FORK_ONE_ARG(do_IdleContainerEviction_test, scheduler);
    }
}

void ServerlessTimingTest::do_IdleContainerEviction_test(
    const std::shared_ptr<wrench::ServerlessScheduler>& scheduler) {
    int argc = 2;
    auto argv = (char**)calloc(argc, sizeof(char*));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-full-log");

    auto simulation = wrench::Simulation::createSimulation();
    simulation->init(&argc, argv);

    simulation->instantiatePlatform(this->platform_file_path);

    auto storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
        "UserHost", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "0"}}, {}));

    std::vector<std::string> compute_nodes = {"ServerlessComputeNode1"};
    auto serverless_provider = simulation->add(new wrench::ServerlessComputeService(
        "ServerlessHeadNode", "/", compute_nodes, scheduler,
        {
            {wrench::ServerlessComputeServiceProperty::CONTAINER_STARTUP_OVERHEAD, "5.0"},
            {wrench::ServerlessComputeServiceProperty::CONTAINER_IDLE_TIMEOUT, "10000.0"},
            // {wrench::ServerlessComputeServiceProperty::IDLE_CONTAINER_EVICTION_POLICY, "RAM"},
            {wrench::ServerlessComputeServiceProperty::IDLE_CONTAINER_EVICTION_POLICY, "LRU"}
        },
        {}));

    std::string user_host = "UserHost";
    auto wms = simulation->add(
        new ServerlessIdleContainerEvictionController(this, user_host, serverless_provider, storage_service));

    simulation->launch();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  IMAGE DOWNLOAD SIMULATION TEST                                  **/
/**********************************************************************/

class ServerlessImageDownloadSimulationController : public wrench::ExecutionController {
public:
    ServerlessImageDownloadSimulationController(ServerlessTimingTest* test,
                                                const std::string& hostname,
                                                const std::shared_ptr<wrench::ServerlessComputeService>&
                                                compute_service,
                                                const std::shared_ptr<wrench::ServerlessComputeService>&
                                                other_compute_service,
                                                const std::shared_ptr<wrench::StorageService>& storage_service) :
        wrench::ExecutionController(hostname, "test") {
        this->test = test;
        this->compute_service = compute_service;
        this->other_compute_service = other_compute_service;
        this->storage_service = storage_service;
    }

private:
    ServerlessTimingTest* test;
    std::shared_ptr<wrench::ServerlessComputeService> compute_service;
    std::shared_ptr<wrench::ServerlessComputeService> other_compute_service;
    std::shared_ptr<wrench::StorageService> storage_service;

    int main() override {
        auto function_manager = this->createFunctionManager();

        // Create a function
        std::function lambda = [](const std::shared_ptr<wrench::FunctionInput>& input,
                                  const std::shared_ptr<wrench::StorageService>& service) -> std::shared_ptr<
            wrench::FunctionOutput> {
            auto real_input = std::dynamic_pointer_cast<MyFunctionInput>(input);
            wrench::Simulation::sleep(10);
            return std::make_shared<MyFunctionOutput>("output");
        };

        // Register a function_1 with a 30GB image file, and a 1GB container RAM space
        auto image_file_1 = wrench::Simulation::addFile("image_file_1", 30 * GB);
        auto image_location_1 = wrench::FileLocation::LOCATION(this->storage_service, image_file_1);
        wrench::StorageService::createFileAtLocation(image_location_1);
        auto image_1 = wrench::FunctionManager::createImage("my_image_1", image_location_1, image_file_1->getSize());

        auto function_1 = wrench::FunctionManager::createFunction("Function_1", lambda, image_1);
        auto input_1 = std::make_shared<MyFunctionInput>(1, 2);
        // 1 GB Container RAM SPACE
        auto registered_function_1 = function_manager->registerFunction(function_1, this->compute_service, 100,
                                                                        1 * GB, 1 * GB, 10 * MB, 1 * MB);

        // Register a function_2 with a 30GB image file, and a 1GB container RAM space
        auto image_file_2 = wrench::Simulation::addFile("image_file_2", 30 * GB);
        auto image_location_2 = wrench::FileLocation::LOCATION(this->storage_service, image_file_2);
        wrench::StorageService::createFileAtLocation(image_location_2);
        auto image_2 = wrench::FunctionManager::createImage("my_image_2", image_location_2, image_file_2->getSize());

        auto function_2 = wrench::FunctionManager::createFunction("Function_2", lambda, image_2);
        auto input_2 = std::make_shared<MyFunctionInput>(1, 2);
        // 1 GB Container RAM SPACE
        auto registered_function_2 = function_manager->registerFunction(function_2, this->other_compute_service, 100,
                                                                        1 * GB, 1 * GB, 10 * MB, 1 * MB);


        // Place three invocations to function_1 and one invocation to function_2
        auto inv1 = function_manager->invokeFunction(registered_function_1, this->compute_service, input_1);
        function_manager->wait_one(inv1);
        auto inv1_elapsed = inv1->getFunctionEndDate() - inv1->getSubmitDate();

        auto inv2 = function_manager->invokeFunction(registered_function_2, this->other_compute_service, input_2);
        function_manager->wait_one(inv2);
        auto inv2_elapsed = inv2->getFunctionEndDate() - inv2->getSubmitDate();

        if (std::abs(inv1_elapsed - inv2_elapsed) < 30.0 * GB / (20 * MB)) {
            throw std::runtime_error(
                "Unexpected elapsed times: inv1=" + std::to_string(inv1_elapsed) + " inv2=" + std::to_string(
                    inv2_elapsed));
        }


        return 0;
    }
};

TEST_F(ServerlessTimingTest, ImageDownloadSimulation) {
    std::vector<std::shared_ptr<wrench::ServerlessScheduler>> schedulers = {
        std::make_shared<wrench::FCFSServerlessScheduler>(),
        // std::make_shared<wrench::RandomServerlessScheduler>(),
        // std::make_shared<wrench::WorkloadBalancingServerlessScheduler>(),
    };
    for (auto& scheduler : schedulers) {
        DO_TEST_WITH_FORK_ONE_ARG(do_ImageDownloadSimulation_test, scheduler);
    }
}

void ServerlessTimingTest::do_ImageDownloadSimulation_test(
    const std::shared_ptr<wrench::ServerlessScheduler>& scheduler) {
    int argc = 1;
    auto argv = (char**)calloc(argc, sizeof(char*));
    argv[0] = strdup("unit_test");
    // argv[1] = strdup("--wrench-full-log");

    auto simulation = wrench::Simulation::createSimulation();
    simulation->init(&argc, argv);

    simulation->instantiatePlatform(this->platform_file_path);

    auto storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
        "UserHost", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "0"}}, {}));

    std::vector<std::string> compute_nodes = {"ServerlessComputeNode1"};
    auto serverless_provider = simulation->add(new wrench::ServerlessComputeService(
        "ServerlessHeadNode", "/", compute_nodes, scheduler,
        {
            {wrench::ServerlessComputeServiceProperty::CONTAINER_STARTUP_OVERHEAD, "5.0"},
            {wrench::ServerlessComputeServiceProperty::CONTAINER_IDLE_TIMEOUT, "10000.0"},
            {wrench::ServerlessComputeServiceProperty::SIMULATE_REMOTE_IMAGE_DOWNLOADS, "true"}
        },
        {}));

    auto other_serverless_provider = simulation->add(new wrench::ServerlessComputeService(
        "ServerlessHeadNode", "/", compute_nodes, scheduler,
        {
            {wrench::ServerlessComputeServiceProperty::CONTAINER_STARTUP_OVERHEAD, "5.0"},
            {wrench::ServerlessComputeServiceProperty::CONTAINER_IDLE_TIMEOUT, "10000.0"},
            {wrench::ServerlessComputeServiceProperty::SIMULATE_REMOTE_IMAGE_DOWNLOADS, "false"}
        },
        {}));

    std::string user_host = "UserHost";
    auto wms = simulation->add(
        new ServerlessImageDownloadSimulationController(this, user_host, serverless_provider, other_serverless_provider,
                                                        storage_service));

    simulation->launch();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
