/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <cmath>

#include <gtest/gtest.h>

#include <wrench-dev.h>

#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(simple_storage_service_performance_test, "Log category for SimpleStorageServicePerformanceTest");


#define FILE_SIZE (10*1000*1000*1000.00) // 10 GB
#define MBPS_BANDWIDTH 100               // 100 MB
#define STORAGE_SIZE (100.0 * FILE_SIZE)

class SimpleStorageServicePerformanceTest : public ::testing::Test {

public:
    wrench::WorkflowFile *file_1;
    wrench::WorkflowFile *file_2;
    wrench::WorkflowFile *file_3;
    std::shared_ptr<wrench::StorageService> storage_service_1 = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service_2 = nullptr;

    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;

    void do_FileRead_test(double buffer_size);
    void do_ConcurrentFileCopies_test(double buffer_size);

    static double computeExpectedTwoStagePipelineTime(double bw_stage1, double bw_stage2, double total_size,
                                                      double buffer_size) {
        double expected_elapsed = 0;
        if (buffer_size >= total_size) {
            return (total_size / bw_stage1) + (total_size / bw_stage2);
        } else {
            double bottleneck_bandwidth = std::min<double>(bw_stage1, bw_stage2);
            double num_full_buffers = std::floor(total_size / buffer_size);
            return buffer_size / bw_stage1 +
                   (num_full_buffers - 1) * (buffer_size / bottleneck_bandwidth) +
                   buffer_size / bw_stage2 +
                   (total_size - num_full_buffers * buffer_size) / bw_stage2;
        }
    }

    static double computeExpectedThreeStagePipelineTime(double bw_stage1, double bw_stage2, double bw_stage3,
                                                        double total_size, double buffer_size) {
        double expected_elapsed = 0;
        if (buffer_size >= total_size) {
            return (total_size / bw_stage1) + (total_size / bw_stage2) + (total_size / bw_stage3);
        } else {
            double bottleneck_bandwidth = std::min<double>(std::min<double>(bw_stage1, bw_stage2), bw_stage3);
            double num_full_buffers = std::floor(total_size / buffer_size);
            double last_nonfull_buffer_size = total_size - num_full_buffers * buffer_size;

            return buffer_size / bw_stage1 +
                   buffer_size / std::min<double>(bw_stage1, bw_stage2) +
                   (num_full_buffers - 2) * (buffer_size / bottleneck_bandwidth) +
                   std::max<double>(last_nonfull_buffer_size / bw_stage1,
                                    buffer_size / std::min<double>(bw_stage2, bw_stage3)) +
                   std::max<double>(last_nonfull_buffer_size / bw_stage2, buffer_size / bw_stage3) +
                   last_nonfull_buffer_size / bw_stage3;
        }
    }


protected:
    SimpleStorageServicePerformanceTest() {

        // Create the simplest workflow
        workflow_unique_ptr = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());
        workflow = workflow_unique_ptr.get();

        // Create the files
        file_1 = workflow->addFile("file_1", FILE_SIZE);
        file_2 = workflow->addFile("file_2", FILE_SIZE);
        file_3 = workflow->addFile("file_3", FILE_SIZE);

        // Create a 3-host platform file (network bandwidth == disk bandwidth)
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"SrcHost\" speed=\"1f\"> "
                          "          <disk id=\"large_disk\" read_bw=\"" + std::to_string(MBPS_BANDWIDTH) +
                          "MBps\" write_bw=\"" + std::to_string(MBPS_BANDWIDTH) +
                          "MBps\">"
                          "             <prop id=\"size\" value=\"" + std::to_string(STORAGE_SIZE) +
                          "B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"DstHost\" speed=\"1f\"> "
                          "          <disk id=\"large_disk\" read_bw=\"" + std::to_string(MBPS_BANDWIDTH) +
                          "MBps\" write_bw=\"" + std::to_string(MBPS_BANDWIDTH) +
                          "MBps\">"
                          "             <prop id=\"size\" value=\"" + std::to_string(STORAGE_SIZE) +
                          "B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"WMSHost\" speed=\"1f\"/> "
                          "       <link id=\"link\" bandwidth=\"" + std::to_string(MBPS_BANDWIDTH) +
                          "MBps\" latency=\"1us\"/>"
                          "       <route src=\"SrcHost\" dst=\"DstHost\">"
                          "         <link_ctn id=\"link\"/>"
                          "       </route>"
                          "       <route src=\"WMSHost\" dst=\"SrcHost\">"
                          "         <link_ctn id=\"link\"/>"
                          "       </route>"
                          "       <route src=\"WMSHost\" dst=\"DstHost\">"
                          "         <link_ctn id=\"link\"/>"
                          "       </route>"
                          "   </zone> "
                          "</platform>";

        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);

    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
    std::unique_ptr<wrench::Workflow> workflow_unique_ptr;
    wrench::Workflow *workflow;
};


/**********************************************************************/
/**  CONCURRENT FILE COPIES TEST                                     **/
/**********************************************************************/

class SimpleStorageServiceConcurrentFileCopiesTestWMS : public wrench::WMS {

public:
    SimpleStorageServiceConcurrentFileCopiesTestWMS(SimpleStorageServicePerformanceTest *test,
                                                    const std::set<std::shared_ptr<wrench::ComputeService>> compute_services,
                                                    const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                                    std::string hostname,
                                                    double buffer_size) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
        this->buffer_size = buffer_size;
    }

private:

    SimpleStorageServicePerformanceTest *test;
    double buffer_size;

    int main() {

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        auto file_registry_service = this->getAvailableFileRegistryService();

        // Time the time it takes to transfer a file from Src to Dst
        double copy1_start = wrench::Simulation::getCurrentSimulatedDate();
        data_movement_manager->initiateAsynchronousFileCopy(this->test->file_1,
                                                            wrench::FileLocation::LOCATION(this->test->storage_service_1),
                                                            wrench::FileLocation::LOCATION(this->test->storage_service_2));

        std::shared_ptr<wrench::WorkflowExecutionEvent> event1 = this->getWorkflow()->waitForNextExecutionEvent();
        double event1_arrival = wrench::Simulation::getCurrentSimulatedDate();

        // Now do 2 of them in parallel
        double copy2_start = wrench::Simulation::getCurrentSimulatedDate();
        data_movement_manager->initiateAsynchronousFileCopy(this->test->file_2,
                                                            wrench::FileLocation::LOCATION(this->test->storage_service_1),
                                                            wrench::FileLocation::LOCATION(this->test->storage_service_2));

        double copy3_start = wrench::Simulation::getCurrentSimulatedDate();
        data_movement_manager->initiateAsynchronousFileCopy(this->test->file_3,
                                                            wrench::FileLocation::LOCATION(this->test->storage_service_1),
                                                            wrench::FileLocation::LOCATION(this->test->storage_service_2));


        std::shared_ptr<wrench::WorkflowExecutionEvent> event2 = this->getWorkflow()->waitForNextExecutionEvent();
        double event2_arrival = wrench::Simulation::getCurrentSimulatedDate();

        std::shared_ptr<wrench::WorkflowExecutionEvent> event3 = this->getWorkflow()->waitForNextExecutionEvent();
        double event3_arrival = wrench::Simulation::getCurrentSimulatedDate();


        double transfer_time_1 = event1_arrival - copy1_start;
        double transfer_time_2 = event2_arrival - copy2_start;
        double transfer_time_3 = event3_arrival - copy3_start;

        // Do relative checks
        if (std::abs(copy2_start - copy3_start) > 0.1) {
            throw std::runtime_error("Time between two asynchronous operations is too big");
        }

        if ((std::abs(transfer_time_2 / transfer_time_1 - 2.0) > 0.001) ||
            (std::abs(transfer_time_3 / transfer_time_1 - 2.0) > 0.001)) {
            throw std::runtime_error("Concurrent transfers should be roughly twice slower");
        }

        if (std::abs(event2_arrival - event3_arrival) > 0.1) {
            throw std::runtime_error("Time between two asynchronous operation completions is too big");
        }

        // Do absolute checksThree
        double expected_transfer_time_1 =
                SimpleStorageServicePerformanceTest::computeExpectedThreeStagePipelineTime(
                        MBPS_BANDWIDTH * 1000 * 1000,
                        0.92 * MBPS_BANDWIDTH * 1000 * 1000,
                        MBPS_BANDWIDTH * 1000 * 1000,
                        FILE_SIZE, buffer_size
                );

        if (std::abs(transfer_time_1 - expected_transfer_time_1) > 1.0) {
            throw std::runtime_error("Unexpected transfer time #1 " + std::to_string(transfer_time_1) +
                                     " (should be around " + std::to_string(expected_transfer_time_1) + ")");
        }


        double expected_transfer_time_2 =
                SimpleStorageServicePerformanceTest::computeExpectedThreeStagePipelineTime(
                        MBPS_BANDWIDTH * 1000 * 1000 / 2,
                        0.92 * MBPS_BANDWIDTH * 1000 * 1000 / 2,
                        MBPS_BANDWIDTH * 1000 * 1000 / 2,
                        FILE_SIZE, buffer_size
                );

        if (std::abs(transfer_time_2 - expected_transfer_time_2) > 1.0) {
            throw std::runtime_error("Unexpected transfer time #2 " + std::to_string(transfer_time_2) +
                                     " (should be around " + std::to_string(expected_transfer_time_2) + ")");
        }

        return 0;
    }
};

TEST_F(SimpleStorageServicePerformanceTest, ConcurrentFileCopies) {
    DO_TEST_WITH_FORK_ONE_ARG(do_ConcurrentFileCopies_test, DBL_MAX)
    DO_TEST_WITH_FORK_ONE_ARG(do_ConcurrentFileCopies_test, FILE_SIZE/2);
    DO_TEST_WITH_FORK_ONE_ARG(do_ConcurrentFileCopies_test, FILE_SIZE/10);
    DO_TEST_WITH_FORK_ONE_ARG(do_ConcurrentFileCopies_test, FILE_SIZE/10 + FILE_SIZE / 3);
    DO_TEST_WITH_FORK_ONE_ARG(do_ConcurrentFileCopies_test, FILE_SIZE/20);
    DO_TEST_WITH_FORK_ONE_ARG(do_ConcurrentFileCopies_test, FILE_SIZE/50);
    DO_TEST_WITH_FORK_ONE_ARG(do_ConcurrentFileCopies_test, FILE_SIZE/100);
}

void SimpleStorageServicePerformanceTest::do_ConcurrentFileCopies_test(double buffer_size) {

    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a (unused) Compute Service
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService("WMSHost",
                                                {std::make_pair("WMSHost", std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))}, {})));

    // Create Two Storage Services
    ASSERT_NO_THROW(storage_service_1 = simulation->add(
            new wrench::SimpleStorageService("SrcHost", {"/"},
                                             {{wrench::StorageServiceProperty::BUFFER_SIZE, std::to_string(buffer_size)}})));
    ASSERT_NO_THROW(storage_service_2 = simulation->add(
            new wrench::SimpleStorageService("DstHost", {"/"},
                                             {{wrench::StorageServiceProperty::BUFFER_SIZE, std::to_string(buffer_size)}})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new SimpleStorageServiceConcurrentFileCopiesTestWMS(
                    this, {compute_service}, {storage_service_1, storage_service_2},
                    "WMSHost", buffer_size)));

    wms->addWorkflow(this->workflow);

    // Create a file registry
    simulation->add(new wrench::FileRegistryService("WMSHost"));

    // Staging all files on the Src storage service
    ASSERT_NO_THROW(simulation->stageFile(file_1, storage_service_1));
    ASSERT_NO_THROW(simulation->stageFile(file_2, storage_service_1));
    ASSERT_NO_THROW(simulation->stageFile(file_3, storage_service_1));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    free(argv[0]);
    free(argv);
}



/**********************************************************************/
/**  FILE READ  TEST                                                 **/
/**********************************************************************/

class SimpleStorageServiceFileReadTestWMS : public wrench::WMS {

public:
    SimpleStorageServiceFileReadTestWMS(SimpleStorageServicePerformanceTest *test,
                                        const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                        std::string hostname, double buffer_size) :
            wrench::WMS(nullptr, nullptr, {}, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
        this->buffer_size = buffer_size;
    }

private:

    SimpleStorageServicePerformanceTest *test;
    double buffer_size;

    int main() {

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        auto file_registry_service = this->getAvailableFileRegistryService();

        double before_read = wrench::Simulation::getCurrentSimulatedDate();
        try {
            wrench::StorageService::readFile(this->test->file_1, wrench::FileLocation::LOCATION(this->test->storage_service_1));
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error(e.what());
        }

        double elapsed = wrench::Simulation::getCurrentSimulatedDate() - before_read;

        double effective_network_bandwidth = (MBPS_BANDWIDTH * 1000 * 1000) * 0.92;
        double effective_disk_bandwidth = (MBPS_BANDWIDTH * 1000 * 1000);

        double expected_elapsed = SimpleStorageServicePerformanceTest::computeExpectedTwoStagePipelineTime(
                effective_disk_bandwidth, effective_network_bandwidth, FILE_SIZE, buffer_size);

        if (std::abs(elapsed - expected_elapsed) > 1.0) {
            throw std::runtime_error("Incorrect file read time " + std::to_string(elapsed) +
                                     " (expected: " + std::to_string(expected_elapsed) +
                                     "; buffer size = " +
                                     (buffer_size == DBL_MAX ? "infty" : std::to_string(buffer_size)) + ")");
        }

        return 0;
    }
};

TEST_F(SimpleStorageServicePerformanceTest, FileRead) {
    DO_TEST_WITH_FORK_ONE_ARG(do_FileRead_test, DBL_MAX);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileRead_test, FILE_SIZE/2);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileRead_test, FILE_SIZE/10);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileRead_test, FILE_SIZE/10 + FILE_SIZE / 3);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileRead_test, FILE_SIZE/20);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileRead_test, FILE_SIZE/50);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileRead_test, FILE_SIZE/100);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileRead_test, FILE_SIZE/200);
}


void SimpleStorageServicePerformanceTest::do_FileRead_test(double buffer_size) {

    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create A Storage Service
    ASSERT_NO_THROW(storage_service_1 = simulation->add(
            new wrench::SimpleStorageService("SrcHost", {"/"}, {
                    {wrench::StorageServiceProperty::BUFFER_SIZE, std::to_string(buffer_size)}
            })));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new SimpleStorageServiceFileReadTestWMS(
                    this, {storage_service_1},
                    "WMSHost", buffer_size)));

    wms->addWorkflow(this->workflow);

    // Create a file registry
    simulation->add(new wrench::FileRegistryService("WMSHost"));

    // Staging all files on the  storage service
    ASSERT_NO_THROW(simulation->stageFile(file_1, storage_service_1));
    ASSERT_NO_THROW(simulation->stageFile(file_2, storage_service_1));
    ASSERT_NO_THROW(simulation->stageFile(file_3, storage_service_1));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    free(argv[0]);
    free(argv);
}
