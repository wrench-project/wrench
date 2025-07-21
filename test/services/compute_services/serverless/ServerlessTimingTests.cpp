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

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"
#include "wrench/services/compute/serverless/schedulers/FCFSServerlessScheduler.h"
#include "wrench/services/compute/serverless/schedulers/RandomServerlessScheduler.h"
#include "wrench/services/compute/serverless/schedulers/WorkloadBalancingServerlessScheduler.h"

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
        auto function = wrench::FunctionManager::createFunction("Function", lambda, image_location);
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
            double compute = 5; // estimate (bottleneck = sleep)
            double expected_elapsed = remote_download + copy_to_compute_node + local_image_read + compute;

            if (fabs(elapsed - expected_elapsed) > 0.05) {
                throw std::runtime_error(
                    "1) Unexpected elapsed time " + std::to_string(elapsed) + " (expected: " + std::to_string(
                        expected_elapsed) + ")");
            }
        }

        // Place another invocation (which saves on some stuff)
        {
            auto now = wrench::Simulation::getCurrentSimulatedDate();
            auto invocation = function_manager->invokeFunction(registered_function, this->compute_service, input);
            function_manager->wait_one(invocation);
            auto elapsed = wrench::Simulation::getCurrentSimulatedDate() - now;
            double remote_download = 0; // cached
            double local_copy = 0; // ALREADY ON DISK!
            double local_image_read = 0; // ALREADY IN RAM!
            double compute = 5; // extimate (bottleneck = sleep)
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
        std::make_shared<wrench::RandomServerlessScheduler>(),
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
        "ServerlessHeadNode",  "/", compute_nodes, scheduler, {}, {}));

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
        auto function_1 = wrench::FunctionManager::createFunction("Function_1", lambda, image_location_1);
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
        //     std::cerr << "INVOCATION #" << i << ": START TIME - COMPLETION TIME: " << invocations.at(i)->getSubmitDate() << ": " << invocations.at(i)->getStartDate() << " -> " << invocations.at(i)->getEndDate() << std::endl;
        // }
        for (unsigned long i = 0; i < num_invocations; i += 10) {
            double start_date = invocations.at(i)->getStartDate();
            double end_date = invocations.at(i)->getEndDate();
            for (unsigned long j = i + 1; j < std::min<unsigned long>(i + 10, num_invocations); j++) {
                if (fabs(start_date - invocations.at(j)->getStartDate()) > 0.1) {
                    throw std::runtime_error("Unexpected execution pattern");
                }
                if (fabs(end_date - invocations.at(j)->getEndDate()) > 0.1) {
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
        "ServerlessHeadNode", "/", compute_nodes,  scheduler, {}, {}));

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
        auto function_1 = wrench::FunctionManager::createFunction("Function_1", lambda, image_location_1);
        auto input_1 = std::make_shared<MyFunctionInput>(1, 2);
        auto registered_function_1 = function_manager->registerFunction(function_1, this->compute_service, 100,
                                                                        2000 * MB, 1 * MB, 10 * MB, 1 * MB);

        // Place an invocation
        auto invocation_1 = function_manager->invokeFunction(registered_function_1, this->compute_service, input_1);

        // Register another function with an image file that will not fit in RAM
        auto image_file_2 = wrench::Simulation::addFile("image_file_2", 60 * GB);
        auto image_location_2 = wrench::FileLocation::LOCATION(this->storage_service, image_file_2);
        wrench::StorageService::createFileAtLocation(image_location_2);
        auto function_2 = wrench::FunctionManager::createFunction("Function_2", lambda, image_location_2);
        auto input_2 = std::make_shared<MyFunctionInput>(1, 2);
        auto registered_function_2 = function_manager->registerFunction(function_2, this->compute_service, 100,
                                                                        2000 * MB, 1 * MB, 10 * MB, 1 * MB);

        auto invocation_2 = function_manager->invokeFunction(registered_function_2, this->compute_service, input_2);

        function_manager->wait_one(invocation_1);
        function_manager->wait_one(invocation_2);

        double image_download = 6494.84;
        double image_copy = 2 * 60 * GB / (100 * MB);
        double image_load = 1 * 60 * GB / (100 * MB);
        double compute_time = 50;

        // std::cerr << "IMAGE DOWNLOAD = " << image_download << std::endl;
        // std::cerr << "IMAGE COPY = " << image_copy << std::endl;
        // std::cerr << "IMAGE LOAD = " << image_load << std::endl;

        double expected_invocation_1_start = image_download + image_copy + image_load;
        double expected_invocation_1_end = expected_invocation_1_start + compute_time;

        double expected_invocation_2_start = expected_invocation_1_end + image_load;
        double expected_invocation_2_end = expected_invocation_2_start + compute_time;

        if (fabs(expected_invocation_1_start - invocation_1->getStartDate()) > EPSILON) {
            throw std::runtime_error("Unexpected invocation_1 start date " +
                std::to_string(invocation_1->getStartDate()) + " instead of " + std::to_string(
                    expected_invocation_1_start));
        }
        if (fabs(expected_invocation_1_end - invocation_1->getEndDate()) > EPSILON) {
            throw std::runtime_error("Unexpected invocation_1 end date " +
                std::to_string(invocation_1->getEndDate()) + " instead of " + std::to_string(
                    expected_invocation_1_end));
        }
        if (fabs(expected_invocation_2_start - invocation_2->getStartDate()) > EPSILON) {
            throw std::runtime_error("Unexpected invocation_2 start date " +
                std::to_string(invocation_2->getStartDate()) + " instead of " + std::to_string(
                    expected_invocation_2_start));
        }
        if (fabs(expected_invocation_2_end - invocation_2->getEndDate()) > EPSILON) {
            throw std::runtime_error("Unexpected invocation_2 end date " +
                std::to_string(invocation_2->getEndDate()) + " instead of " + std::to_string(
                    expected_invocation_2_end));
        }


        return 0;
    }
};

TEST_F(ServerlessTimingTest, RAMPressureDueToImages) {
    std::vector<std::shared_ptr<wrench::ServerlessScheduler>> schedulers = {
        std::make_shared<wrench::FCFSServerlessScheduler>(),
        std::make_shared<wrench::RandomServerlessScheduler>(),
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
        "ServerlessHeadNode", "/", compute_nodes,  scheduler, {}, {}));

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
        auto function_1 = wrench::FunctionManager::createFunction("Function_1", lambda, image_location_1);
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
            double start_date = invocations.at(i)->getStartDate();
            double end_date = invocations.at(i)->getEndDate();
            for (unsigned long j = i + 1; j < std::min<unsigned long>(i + 4, num_invocations); j++) {
                if (fabs(start_date - invocations.at(j)->getStartDate()) > 0.1) {
                    throw std::runtime_error("Unexpected execution pattern");
                }
                if (fabs(end_date - invocations.at(j)->getEndDate()) > 0.1) {
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
        "ServerlessHeadNode", "/", compute_nodes,  scheduler, {}, {}));

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
        auto function_1 = wrench::FunctionManager::createFunction("Function_1", lambda, image_location_1);
        auto input_1 = std::make_shared<MyFunctionInput>(1, 2);
        auto registered_function_1 = function_manager->registerFunction(function_1, this->compute_service, 100,
                                                                        2000 * MB, 1 * MB, 10 * MB, 1 * MB);

        // Place an invocation
        auto invocation_1 = function_manager->invokeFunction(registered_function_1, this->compute_service, input_1);

        // Register another function with an image file that will not fit on disk
        auto image_file_2 = wrench::Simulation::addFile("image_file_2", 61 * GB);
        auto image_location_2 = wrench::FileLocation::LOCATION(this->storage_service, image_file_2);
        wrench::StorageService::createFileAtLocation(image_location_2);
        auto function_2 = wrench::FunctionManager::createFunction("Function_2", lambda, image_location_2);
        auto input_2 = std::make_shared<MyFunctionInput>(1, 2);
        auto registered_function_2 = function_manager->registerFunction(function_2, this->compute_service, 100,
                                                                        2000 * MB, 1 * MB, 10 * MB, 1 * MB);

        auto invocation_2 = function_manager->invokeFunction(registered_function_2, this->compute_service, input_2);

        function_manager->wait_one(invocation_1);
        function_manager->wait_one(invocation_2);

        // We expect that as soo as invocation_1 has started, then invocation_2 can finally do the copy and load.
        double expected_invocation_2_start_date = invocation_1->getStartDate() + 2 * (61 * GB / (100 * MB));

        if (fabs(expected_invocation_2_start_date - invocation_2->getStartDate()) > 0.1) {
            throw std::runtime_error("Unexpected start date for invocation_2 " + std::to_string(invocation_2->getStartDate()) +
                " (expected: " +std::to_string(expected_invocation_2_start_date) + ")");
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
        "ServerlessHeadNode", "/", compute_nodes,  scheduler, {}, {}));

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
        auto function_1 = wrench::FunctionManager::createFunction("Function_1", lambda, image_location_1);
        auto input_1 = std::make_shared<MyFunctionInput>(1, 2);
        auto registered_function_1 = function_manager->registerFunction(function_1, this->compute_service, 100,
                                                                        30 * GB, 1 * MB, 10 * MB, 1 * MB);

        // Place invocations, but only 3 should be able to run at a time (since the 60GB image will be evicted!)
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
        //     std::cerr << "INVOCATION #" << i << ": START TIME - COMPLETION TIME: " << invocations.at(i)->getSubmitDate() << ": " << invocations.at(i)->getStartDate() << " -> " << invocations.at(i)->getEndDate() << std::endl;
        // }
        for (unsigned long i = 0; i < num_invocations; i += 3) {
            double start_date = invocations.at(i)->getStartDate();
            double end_date = invocations.at(i)->getEndDate();
            for (unsigned long j = i + 1; j < std::min<unsigned long>(i + 3, num_invocations); j++) {
                if (fabs(start_date - invocations.at(j)->getStartDate()) > 0.1) {
                    throw std::runtime_error("Unexpected execution pattern");
                }
                if (fabs(end_date - invocations.at(j)->getEndDate()) > 0.1) {
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
        "ServerlessHeadNode", "/", compute_nodes,  scheduler, {}, {}));

    std::string user_host = "UserHost";
    auto wms = simulation->add(
        new ServerlessDiskPressureDueToInvocationsController(this, user_host, serverless_provider, storage_service));

    simulation->launch();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
