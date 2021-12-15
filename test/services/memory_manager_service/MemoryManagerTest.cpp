
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

#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(memory_manager_test, "Log category for MemoryManager test");

#define GB (1000.0*1000.0*1000.0)
#define PAGE_CACHE_RAM_SIZE  32
#define FILE_SIZE 20

class MemoryManagerTest : public ::testing::Test {
public:
    std::shared_ptr<wrench::StorageService> storage_service1 = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service2 = nullptr;
    std::shared_ptr<wrench::BareMetalComputeService> compute_service = nullptr;

    void do_MemoryManagerBadSetupTest_test();
    void do_MemoryManagerChainOfTasksTest_test();

protected:

    ~MemoryManagerTest() {
        workflow->clear();
    }

    MemoryManagerTest() {

        // Create the simplest workflow
        workflow = wrench::Workflow::createWorkflow();

        // Create a platform file
        std::string bad_xml = "<?xml version='1.0'?>"
                              "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                              "<platform version=\"4.1\"> "
                              "   <zone id=\"AS0\" routing=\"Full\"> "
                              "       <host id=\"TwoCoreHost\" speed=\"100f\" core=\"2\"> "
                              "          <disk id=\"large_disk1\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                              "             <prop id=\"size\" value=\"1.5GB\"/>"
                              "             <prop id=\"mount\" value=\"/\"/>"
                              "          </disk>"
                              "       </host> "
                              "       <host id=\"OneCoreHost\" speed=\"100f\" core=\"1\"> "
                              "          <disk id=\"large_disk1\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                              "             <prop id=\"size\" value=\"1.5GB\"/>"
                              "             <prop id=\"mount\" value=\"/\"/>"
                              "          </disk>"
                              "       </host> "
                              "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                              "       <route src=\"TwoCoreHost\" dst=\"OneCoreHost\"> <link_ctn id=\"1\"/> </route>"
                              "   </zone> "
                              "</platform>";
        FILE *bad_platform_file = fopen(bad_platform_file_path.c_str(), "w");
        fprintf(bad_platform_file, "%s", bad_xml.c_str());
        fclose(bad_platform_file);


        // Create a VALID platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"TwoCoreHost\" speed=\"1000f\" core=\"2\"> "
                          "          <disk id=\"large_disk1\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"30000GB\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"memory\" read_bw=\"1000MBps\" write_bw=\"1000MBps\">"
                          "             <prop id=\"size\" value=\"" + std::to_string(PAGE_CACHE_RAM_SIZE) + "GB\"/>"
                                                                                                            "             <prop id=\"mount\" value=\"/memory\"/>"
                                                                                                            "          </disk>"
                                                                                                            "       </host> "
                                                                                                            "       <host id=\"OneCoreHost\" speed=\"100f\" core=\"1\"> "
                                                                                                            "          <disk id=\"memory\" read_bw=\"1000MBps\" write_bw=\"1000MBps\">"
                                                                                                            "             <prop id=\"size\" value=\"" + std::to_string(PAGE_CACHE_RAM_SIZE) + "GB\"/>"
                                                                                                                                                                                              "             <prop id=\"mount\" value=\"/memory\"/>"
                                                                                                                                                                                              "          </disk>"
                                                                                                                                                                                              "          <disk id=\"large_disk1\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                                                                                                                                                                                              "             <prop id=\"size\" value=\"30000GB\"/>"
                                                                                                                                                                                              "             <prop id=\"mount\" value=\"/\"/>"
                                                                                                                                                                                              "          </disk>"
                                                                                                                                                                                              "       </host> "
                                                                                                                                                                                              "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                                                                                                                                                                                              "       <route src=\"TwoCoreHost\" dst=\"OneCoreHost\"> <link_ctn id=\"1\"/> </route>"
                                                                                                                                                                                              "   </zone> "
                                                                                                                                                                                              "</platform>";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);

    }

    std::string bad_platform_file_path = UNIQUE_TMP_PATH_PREFIX + "bas_platform.xml";
    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
    std::shared_ptr<wrench::Workflow> workflow;

};


/**********************************************************************/
/** BAD SETUP TEST                                                   **/
/**********************************************************************/

TEST_F(MemoryManagerTest, BadSetup) {
    DO_TEST_WITH_FORK(do_MemoryManagerBadSetupTest_test)
}

void MemoryManagerTest::do_MemoryManagerBadSetupTest_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-pagecache-simulation");
//    argv[2] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_THROW(simulation->instantiatePlatform(bad_platform_file_path), std::invalid_argument);

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/** EXECUTION WITH PRE/POST COPIES AND CLEANUP SIMULATION TEST       **/
/**********************************************************************/

class MemoryManagerChainOfTasksTestWMS : public wrench::ExecutionController {
public:
    MemoryManagerChainOfTasksTestWMS(
            MemoryManagerTest *test,
            std::shared_ptr<wrench::Workflow> workflow,
            std::string &hostname) :
            wrench::ExecutionController(hostname, "test"),
            test(test), workflow(workflow){
    }

private:
    MemoryManagerTest *test;
    std::shared_ptr<wrench::Workflow> workflow;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        while (not this->workflow->isDone()) {

            // Create job
            auto task = *(this->workflow->getReadyTasks().begin());
            auto job = job_manager->createStandardJob(
                    task,
                    {{*(task->getInputFiles().begin()), wrench::FileLocation::LOCATION((test->storage_service1))},
                     {*(task->getOutputFiles().begin()), wrench::FileLocation::LOCATION(test->storage_service1)}});
            // Submit the job
            job_manager->submitJob(job, test->compute_service);

            // Wait for the workflow execution event
            std::shared_ptr<wrench::ExecutionEvent> event = this->waitForNextEvent();
            if (std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
                // do nothing
            } else if (auto real_event = std::dynamic_pointer_cast<wrench::StandardJobFailedEvent>(event)) {
                throw std::runtime_error("Unexpected job failure: " +
                                         real_event->failure_cause->toString());
            } else {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }
        }

        return 0;
    }
};

TEST_F(MemoryManagerTest, MemoryManagerChainOfTask) {
    DO_TEST_WITH_FORK(do_MemoryManagerChainOfTasksTest_test)
}

void MemoryManagerTest::do_MemoryManagerChainOfTasksTest_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc =2;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-pagecache-simulation");

    ASSERT_THROW(simulation->launch(), std::runtime_error);

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "TwoCoreHost";

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/"},
                                             {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "100000000.0"}},
                                             {})));

    // Create a Compute Service
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair(hostname,
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                "", {})));

    // Create a Workflow
    auto workflow = wrench::Workflow::createWorkflow();
    auto previous_output_file = workflow->addFile("task0_input", FILE_SIZE * GB);
    int num_tasks = 10;
    for (int i=0; i < num_tasks; i++) {
        auto task = workflow->addTask("task1" + std::to_string(i), 100.0, 1, 1, 0.0);
        auto output_file = workflow->addFile("task1" + std::to_string(i) + "_output", FILE_SIZE * GB);
        task->addOutputFile(output_file);
        task->addInputFile(previous_output_file);
        previous_output_file = output_file;
    }

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new MemoryManagerChainOfTasksTestWMS(this, workflow, hostname)));
    // Create a File Registry Service
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

    // Staging the input_file on storage service #1
    ASSERT_NO_THROW(simulation->stageFile(workflow->getFileByID("task0_input"), storage_service1));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());

    workflow->clear();

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


