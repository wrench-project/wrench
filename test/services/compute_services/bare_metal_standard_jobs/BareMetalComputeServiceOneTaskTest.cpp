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

WRENCH_LOG_CATEGORY(bare_metal_compute_service_test, "Log category for BareMetalComputeServiceOneTask test");

class BareMetalComputeServiceOneTaskTest : public ::testing::Test {
public:
    std::shared_ptr<wrench::Workflow> workflow;

    std::shared_ptr<wrench::DataFile> input_file;
    std::shared_ptr<wrench::DataFile> output_file;
    std::shared_ptr<wrench::WorkflowTask> task;
    std::shared_ptr<wrench::StorageService> storage_service1 = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service2 = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service3 = nullptr;
    std::shared_ptr<wrench::BareMetalComputeService> compute_service = nullptr;

    void do_BadSetup_test();

    void do_Noop_test();

    void do_StandardJobConstructor_test();

    void do_HostMemory_test();

    void do_ExecutionWithLocationMap_test();

    void do_ExecutionWithLocationMapMultiple_test();

    void do_ExecutionWithDefaultStorageService_test();

    void do_ExecutionWithPrePostCopiesTaskCleanup_test();

    void do_ExecutionWithPrePostCopiesNoTaskNoCleanup_test();

    void do_ExecutionWithPreNoPostCopiesNoTaskCleanup_test();

    void do_ExecutionWithMissingFile_test();

    void do_ExecutionWithNotEnoughCores_test();

    void do_ExecutionWithNotEnoughRAM_test();

    void do_ExecutionWithDownService_test();

    void do_ExecutionWithSuspendedService_test();


protected:
    ~BareMetalComputeServiceOneTaskTest() {
        workflow->clear();
    }

    BareMetalComputeServiceOneTaskTest() {

        // Create the simplest workflow
        workflow = wrench::Workflow::createWorkflow();

        // Create two files
        input_file = workflow->addFile("input_file", 10000.0);
        output_file = workflow->addFile("output_file", 20000.0);

        // Create one task1
        task = workflow->addTask("task1", 3600, 1, 1, 0);
        task->addInputFile(input_file);
        task->addOutputFile(output_file);

        // Create a platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"TwoCoreHost\" speed=\"1f\" core=\"2\"> "
                          "          <disk id=\"large_disk1\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"30000B\"/>"
                          "             <prop id=\"mount\" value=\"/disk1\"/>"
                          "          </disk>"
                          "          <disk id=\"large_disk2\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"30000B\"/>"
                          "             <prop id=\"mount\" value=\"/disk2\"/>"
                          "          </disk>"
                          "          <disk id=\"large_disk3\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"30000B\"/>"
                          "             <prop id=\"mount\" value=\"/disk3\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host> "
                          "       <host id=\"OneCoreHost\" speed=\"1f\" core=\"1\"> "
                          "          <disk id=\"large_disk1\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"30000B\"/>"
                          "             <prop id=\"mount\" value=\"/disk1\"/>"
                          "          </disk>"
                          "          <disk id=\"large_disk2\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"30000B\"/>"
                          "             <prop id=\"mount\" value=\"/disk2\"/>"
                          "          </disk>"
                          "          <disk id=\"large_disk3\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"30000B\"/>"
                          "             <prop id=\"mount\" value=\"/disk3\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host> "
                          "       <host id=\"RAMHost\" speed=\"1f\" core=\"1\" > "
                          "          <disk id=\"large_disk1\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"30000B\"/>"
                          "             <prop id=\"mount\" value=\"/disk1\"/>"
                          "          </disk>"
                          "          <disk id=\"large_disk2\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"30000B\"/>"
                          "             <prop id=\"mount\" value=\"/disk2\"/>"
                          "          </disk>"
                          "          <disk id=\"large_disk3\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"30000B\"/>"
                          "             <prop id=\"mount\" value=\"/disk3\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "         <prop id=\"ram\" value=\"1024B\" />"
                          "       </host> "
                          "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                          "       <route src=\"TwoCoreHost\" dst=\"OneCoreHost\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"RAMHost\" dst=\"OneCoreHost\"> <link_ctn id=\"1\"/> </route>"
                          "   </zone> "
                          "</platform>";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};

/**********************************************************************/
/**  BAD SETUP SIMULATION TEST                                       **/
/**********************************************************************/

class BadSetupTestWMS : public wrench::ExecutionController {
public:
    BadSetupTestWMS(BareMetalComputeServiceOneTaskTest *test,
                    std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BareMetalComputeServiceOneTaskTest *test;

    int main() {

        return 0;
    }
};

TEST_F(BareMetalComputeServiceOneTaskTest, BadSetup) {
    DO_TEST_WITH_FORK(do_BadSetup_test);
}

void BareMetalComputeServiceOneTaskTest::do_BadSetup_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

    int argc = 0;
    auto **argv = (char **) calloc(argc, sizeof(char *));

    ASSERT_THROW(simulation->init(&argc, argv), std::invalid_argument);
    free(argv);

    argc = 1;
    ASSERT_THROW(simulation->init(&argc, nullptr), std::invalid_argument);

    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

    ASSERT_THROW(simulation->launch(), std::runtime_error);

    argc = 1;
    argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("one_task_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Empty resource list
    ASSERT_THROW(compute_service = simulation->add(
                         new wrench::BareMetalComputeService("bogus",
                                                             (std::map<std::string, std::tuple<unsigned long, double>>){},
                                                             {})),
                 std::invalid_argument);

    // Bad hostname
    ASSERT_THROW(compute_service = simulation->add(
                         new wrench::BareMetalComputeService("bogus",
                                                             {std::make_pair("bogus",
                                                                             std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                             wrench::ComputeService::ALL_RAM))},
                                                             {})),
                 std::invalid_argument);

    // Get a hostname
    auto hostname = wrench::Simulation::getHostnameList()[0];

    // Bad resource hostname
    ASSERT_THROW(compute_service = simulation->add(
                         new wrench::BareMetalComputeService(hostname,
                                                             {std::make_pair("bogus",
                                                                             std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                             wrench::ComputeService::ALL_RAM))},
                                                             "",
                                                             {})),
                 std::invalid_argument);

    // Bad number of cores
    ASSERT_THROW(compute_service = simulation->add(
                         new wrench::BareMetalComputeService(hostname,
                                                             {std::make_pair(hostname,
                                                                             std::make_tuple(0,
                                                                                             wrench::ComputeService::ALL_RAM))},
                                                             "",
                                                             {})),
                 std::invalid_argument);

    // Bad number of cores
    ASSERT_THROW(compute_service = simulation->add(
                         new wrench::BareMetalComputeService(hostname,
                                                             {std::make_pair(hostname,
                                                                             std::make_tuple(100,
                                                                                             wrench::ComputeService::ALL_RAM))},
                                                             {})),
                 std::invalid_argument);


    // Bad RAM
    ASSERT_THROW(compute_service = simulation->add(
                         new wrench::BareMetalComputeService(hostname,
                                                             {std::make_pair("RAMHost",
                                                                             std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                             -1.0))},
                                                             {})),
                 std::invalid_argument);

    // Bad RAM
    ASSERT_THROW(compute_service = simulation->add(
                         new wrench::BareMetalComputeService(hostname,
                                                             {std::make_pair("RAMHost",
                                                                             std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                             100000.0))},
                                                             {})),
                 std::invalid_argument);

    // Bad PROPERTIES
    ASSERT_THROW(compute_service = simulation->add(
                         new wrench::BareMetalComputeService(hostname,
                                                             {std::make_pair("RAMHost",
                                                                             std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                             100000.0))},
                                                             "",
                                                             {std::make_pair(
                                                                     wrench::BareMetalComputeServiceProperty::THREAD_STARTUP_OVERHEAD,
                                                                     "-1.0")},
                                                             {})),
                 std::invalid_argument);

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(new BadSetupTestWMS(this, hostname)));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  NOOP SIMULATION TEST                                            **/
/**********************************************************************/

class NoopTestWMS : public wrench::ExecutionController {
public:
    NoopTestWMS(BareMetalComputeServiceOneTaskTest *test,
                std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BareMetalComputeServiceOneTaskTest *test;

    int main() {

        wrench::TerminalOutput::disableColor();// just for increasing stupid coverage

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Stop the Job Manager manually, just for kicks
        job_manager->stop();

        // Stop the Data Movement Manager manually, just for kicks
        data_movement_manager->stop();

        wrench::Simulation::sleep(1);

        // Stop the Compute service manually, for coverage
        this->test->compute_service->stop();

        return 0;
    }
};

TEST_F(BareMetalComputeServiceOneTaskTest, Noop) {
    DO_TEST_WITH_FORK(do_Noop_test);
}

void BareMetalComputeServiceOneTaskTest::do_Noop_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("one_task_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

    ASSERT_THROW(simulation->add((wrench::ComputeService *) nullptr), std::invalid_argument);

    // Create a Compute Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BareMetalComputeService("TwoCoreHost",
                                                                {std::make_pair("TwoCoreHost",
                                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                wrench::ComputeService::ALL_RAM))},
                                                                {"/scratch"},
                                                                {})));

    // Create a Storage Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            new wrench::SimpleStorageService("TwoCoreHost", {"/disk1"})));

    // Create a WMS
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    std::string hostname = "TwoCoreHost";
    ASSERT_NO_THROW(wms = simulation->add(
                            new NoopTestWMS(this, hostname)));

    ASSERT_THROW(simulation->stageFile(input_file, storage_service1), std::runtime_error);

    simulation->add(new wrench::FileRegistryService(hostname));

    ASSERT_THROW(simulation->stageFile(input_file, (std::shared_ptr<wrench::StorageService>) nullptr),
                 std::invalid_argument);
    ASSERT_THROW(simulation->stageFile(nullptr, storage_service1), std::invalid_argument);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "do nothing" simulation
    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  STANDARD JOB CONSTRUCTOR TEST                                   **/
/**********************************************************************/

class StandardJobConstructorTestWMS : public wrench::ExecutionController {
public:
    StandardJobConstructorTestWMS(
            BareMetalComputeServiceOneTaskTest *test,
            std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BareMetalComputeServiceOneTaskTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        std::shared_ptr<wrench::StandardJob> job = nullptr;

        // Create a job with nullptr task1 (and no file copies)
        try {
            job = job_manager->createStandardJob(nullptr,
                                                 {{test->input_file, wrench::FileLocation::LOCATION(
                                                                             (test->storage_service1))},
                                                  {test->output_file, wrench::FileLocation::LOCATION(
                                                                              (test->storage_service1))}});
            throw std::runtime_error("Should not be able to create a job with an empty task1");
        } catch (std::invalid_argument &e) {
        }

        // Create a job with an empty vector of tasks (and no file copies)
        try {
            std::vector<std::shared_ptr<wrench::WorkflowTask>> empty;
            job = job_manager->createStandardJob(empty,
                                                 {{test->input_file, wrench::FileLocation::LOCATION(
                                                                             (test->storage_service1))},
                                                  {test->output_file, wrench::FileLocation::LOCATION(
                                                                              (test->storage_service1))}});
            throw std::runtime_error("Should not be able to create a job with an empty task1 vector");
        } catch (std::invalid_argument &e) {
        }

        // Create a job with a vector of empty tasks (and no file copies)
        try {
            job = job_manager->createStandardJob((std::vector<std::shared_ptr<wrench::WorkflowTask>>){nullptr, nullptr},
                                                 {{test->input_file, wrench::FileLocation::LOCATION(
                                                                             (test->storage_service1))},
                                                  {test->output_file, wrench::FileLocation::LOCATION(
                                                                              (test->storage_service1))}});
            throw std::runtime_error("Should not be able to create a job with a vector of empty tasks");
        } catch (std::invalid_argument &e) {
        }

        // Create another task1
        std::shared_ptr<wrench::WorkflowTask> task_big = this->test->workflow->addTask("task2", 3600, 2, 2, 2048);

        // Create a job with nullptrs in file locations
        try {

            job = job_manager->createStandardJob({test->task, task_big},
                                                 {{nullptr, wrench::FileLocation::LOCATION(
                                                                    (test->storage_service1))},
                                                  {test->output_file, wrench::FileLocation::LOCATION(
                                                                              (test->storage_service1))}});
            throw std::runtime_error("Should not be able to create a job with an nullptr file in file locations");
        } catch (std::invalid_argument &e) {
        }

        // Create a job with nullptrs in file locations
        try {
            job = job_manager->createStandardJob({test->task, task_big},
                                                 {{test->input_file, nullptr},
                                                  {test->output_file, wrench::FileLocation::LOCATION(
                                                                              (test->storage_service1))}});
            throw std::runtime_error(
                    "Should not be able to create a job with an nullptr storage service in file locations");
        } catch (std::invalid_argument &e) {
        }

        // Create a job with nullptrs in pre file copies
        try {
            job = job_manager->createStandardJob({test->task},
                                                 {{test->input_file, wrench::FileLocation::LOCATION(
                                                                             (test->storage_service1))},
                                                  {test->output_file, wrench::FileLocation::LOCATION(
                                                                              (test->storage_service1))}},
                                                 {std::make_tuple(nullptr, wrench::FileLocation::LOCATION((test->storage_service1)), wrench::FileLocation::LOCATION(test->storage_service2))},
                                                 {},
                                                 {});
            throw std::runtime_error("Should not be able to create a job with a nullptr file in pre file copies");
        } catch (std::invalid_argument &e) {
        }

        // Create a job with nullptrs in pre file copies
        try {
            job = job_manager->createStandardJob({test->task},
                                                 {{test->input_file, wrench::FileLocation::LOCATION(
                                                                             test->storage_service1)},
                                                  {test->output_file, wrench::FileLocation::LOCATION(
                                                                              (test->storage_service1))}},
                                                 {std::make_tuple(test->output_file, nullptr,
                                                                  wrench::FileLocation::LOCATION(
                                                                          test->storage_service2))},
                                                 {},
                                                 {});
            throw std::runtime_error(
                    "Should not be able to create a job with a nullptr src storage service in pre file copies");
        } catch (std::invalid_argument &e) {
        }

        // Create a job with nullptrs in pre file copies
        try {
            job = job_manager->createStandardJob({test->task},
                                                 {{test->input_file, wrench::FileLocation::LOCATION(
                                                                             (test->storage_service1))},
                                                  {test->output_file, wrench::FileLocation::LOCATION(
                                                                              (test->storage_service1))}},
                                                 {std::make_tuple(test->output_file, wrench::FileLocation::LOCATION((test->storage_service1)), nullptr)},
                                                 {},
                                                 {});
            throw std::runtime_error(
                    "Should not be able to create a job with a nullptr dst storage service in pre file copies");
        } catch (std::invalid_argument &e) {
        }

        // Create a job with nullptrs in post file copies
        try {
            job = job_manager->createStandardJob({test->task},
                                                 {{test->input_file, wrench::FileLocation::LOCATION(
                                                                             (test->storage_service1))},
                                                  {test->output_file, wrench::FileLocation::LOCATION(
                                                                              (test->storage_service1))}},
                                                 {},
                                                 {std::make_tuple(nullptr, wrench::FileLocation::LOCATION(test->storage_service1), wrench::FileLocation::LOCATION(test->storage_service2))},
                                                 {});
            throw std::runtime_error("Should not be able to create a job with a nullptr file in post file copies");
        } catch (std::invalid_argument &e) {
        }

        // Create a job with nullptrs in post file copies
        try {
            job = job_manager->createStandardJob({test->task},
                                                 {{test->input_file, wrench::FileLocation::LOCATION(
                                                                             (test->storage_service1))},
                                                  {test->output_file, wrench::FileLocation::LOCATION(
                                                                              (test->storage_service1))}},
                                                 {},
                                                 {std::make_tuple(test->output_file, nullptr,
                                                                  wrench::FileLocation::LOCATION(
                                                                          test->storage_service2))},
                                                 {});
            throw std::runtime_error(
                    "Should not be able to create a job with a nullptr src storage service in post file copies");
        } catch (std::invalid_argument &e) {
        }

        // Create a job with nullptrs in post file copies
        try {
            job = job_manager->createStandardJob({test->task},
                                                 {{test->input_file, wrench::FileLocation::LOCATION(
                                                                             (test->storage_service1))},
                                                  {test->output_file, wrench::FileLocation::LOCATION(
                                                                              (test->storage_service1))}},
                                                 {},
                                                 {std::make_tuple(test->output_file, wrench::FileLocation::LOCATION((test->storage_service1)), nullptr)},
                                                 {});
            throw std::runtime_error(
                    "Should not be able to create a job with a nullptr dst storage service in post file copies");
        } catch (std::invalid_argument &e) {
        }

        // Create a job with nullptrs in post file deletions
        try {
            job = job_manager->createStandardJob({test->task},
                                                 {{test->input_file, wrench::FileLocation::LOCATION(
                                                                             (test->storage_service1))},
                                                  {test->output_file, wrench::FileLocation::LOCATION(
                                                                              (test->storage_service1))}},
                                                 {},
                                                 {},
                                                 {std::make_tuple(nullptr, wrench::FileLocation::LOCATION(
                                                                                   (test->storage_service1)))});
            throw std::runtime_error("Should not be able to create a job with a nullptr file in file deletions");
        } catch (std::invalid_argument &e) {
        }


        // Create a job with nullptrs in post file deletions
        try {
            job = job_manager->createStandardJob({test->task},
                                                 {{test->input_file, wrench::FileLocation::LOCATION(
                                                                             (test->storage_service1))},
                                                  {test->output_file, wrench::FileLocation::LOCATION(
                                                                              (test->storage_service1))}},
                                                 {},
                                                 {},
                                                 {std::make_tuple(test->input_file, nullptr)});
            throw std::runtime_error(
                    "Should not be able to create a job with a nullptr storage service in file deletions");
        } catch (std::invalid_argument &e) {
        }

        if (job) {
            // use job to avoid 'unused-but-set-variable' warning
        }

        // Stop the Job Manager manually, just for kicks
        job_manager->stop();

        return 0;
    }
};

TEST_F(BareMetalComputeServiceOneTaskTest, StandardJobConstructor) {
    DO_TEST_WITH_FORK(do_StandardJobConstructor_test);
}

void BareMetalComputeServiceOneTaskTest::do_StandardJobConstructor_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("one_task_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    simulation->instantiatePlatform(platform_file_path);

    // Get a hostname
    std::string hostname1 = "TwoCoreHost";

    // Create a Compute Service
    compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname1,
                                                {std::make_pair(hostname1,
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                "/scratch",
                                                {}));

    // Create a Storage Service
    storage_service1 = simulation->add(
            new wrench::SimpleStorageService(hostname1, {"/disk1"}));

    // Start a file registry service
    simulation->add(new wrench::FileRegistryService(hostname1));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = simulation->add(
            new StandardJobConstructorTestWMS(
                    this, hostname1));

    // Staging the input_file on the storage service
    simulation->stageFile(input_file, storage_service1);

    // Running a "do nothing" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  HOST MEMORY SIMULATION TEST                                     **/
/**********************************************************************/

class HostMemoryTestWMS : public wrench::ExecutionController {
public:
    HostMemoryTestWMS(BareMetalComputeServiceOneTaskTest *test, std::string hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BareMetalComputeServiceOneTaskTest *test;

    int main() {
        double ram_capacity;

        ram_capacity = wrench::Simulation::getHostMemoryCapacity("TwoCoreHost");
        if (ram_capacity != wrench::S4U_Simulation::DEFAULT_RAM) {
            throw std::runtime_error("RAM Capacity of TwoCoreHost should be the default");
        }

        ram_capacity = wrench::Simulation::getMemoryCapacity();
        if (ram_capacity != wrench::S4U_Simulation::DEFAULT_RAM) {
            throw std::runtime_error("RAM Capacity of TwoCoreHost should be the default");
        }

        ram_capacity = wrench::Simulation::getHostMemoryCapacity("RAMHost");
        if (ram_capacity == wrench::S4U_Simulation::DEFAULT_RAM) {
            throw std::runtime_error("RAM Capacity of RAMHost should not be the default");
        }
        if (std::abs(ram_capacity - 1024) > 0.01) {
            throw std::runtime_error("RAM Capacity of RAMHost should  be 1024");
        }

        return 0;
    }
};

TEST_F(BareMetalComputeServiceOneTaskTest, HostMemory) {
    DO_TEST_WITH_FORK(do_HostMemory_test);
}

void BareMetalComputeServiceOneTaskTest::do_HostMemory_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("one_task_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    simulation->instantiatePlatform(platform_file_path);

    // Get a hostname
    std::string hostname1 = "TwoCoreHost";
    std::string hostname2 = "RAMHost";

    // Create a Compute Service
    compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname1,
                                                {std::make_pair(hostname1,
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                "/scratch",
                                                {}));

    // Create a Storage Service
    storage_service1 = simulation->add(
            new wrench::SimpleStorageService(hostname1, {"/disk1"}));

    // Start a file registry service
    simulation->add(new wrench::FileRegistryService(hostname1));

    // Create a WMS
    auto wms = simulation->add(
            new HostMemoryTestWMS(this, "TwoCoreHost"));

    // Staging the input_file on the storage service
    simulation->stageFile(input_file, storage_service1);

    // Running a "do nothing" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/** EXECUTION WITH LOCATION_MAP SIMULATION TEST                      **/
/**********************************************************************/

class ExecutionWithLocationMapTestWMS : public wrench::ExecutionController {
public:
    ExecutionWithLocationMapTestWMS(BareMetalComputeServiceOneTaskTest *test,
                                    std::string &hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:
    BareMetalComputeServiceOneTaskTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a job
        auto job = job_manager->createStandardJob(test->task,
                                                  {{test->input_file, wrench::FileLocation::LOCATION(
                                                                              test->storage_service1, "/disk1")},
                                                   {test->output_file, wrench::FileLocation::LOCATION(
                                                                               test->storage_service1, "/disk1")}});


        // Submit the job
        job_manager->submitJob(job, test->compute_service);

        // Wait for the workflow execution event
        std::shared_ptr<wrench::ExecutionEvent> event = this->waitForNextEvent();
        if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        // bogus lookup #1
        try {
            wrench::StorageService::lookupFile(nullptr, wrench::FileLocation::LOCATION(this->test->storage_service1));
            throw std::runtime_error("Should not have been able to lookup a nullptr file");
        } catch (std::invalid_argument &e) {
        }

        // bogus lookup #2
        try {
            wrench::StorageService::lookupFile(this->test->output_file, nullptr);
            throw std::runtime_error("Should not have been able to lookup a nullptr location");
        } catch (std::invalid_argument &e) {
        }


        if (!wrench::StorageService::lookupFile(this->test->output_file,
                                                wrench::FileLocation::LOCATION(this->test->storage_service1))) {
            throw std::runtime_error("Output file not written to storage service");
        }


        return 0;
    }
};

TEST_F(BareMetalComputeServiceOneTaskTest, ExecutionWithLocationMap) {
    DO_TEST_WITH_FORK(do_ExecutionWithLocationMap_test);
}

void BareMetalComputeServiceOneTaskTest::do_ExecutionWithLocationMap_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("one_task_test");
    //    argv[1] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Compute Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BareMetalComputeService(hostname,
                                                                {std::make_pair(hostname,
                                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                wrench::ComputeService::ALL_RAM))},
                                                                "", {})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            new wrench::SimpleStorageService("OneCoreHost", {"/disk1"})));

    // Create a File Registry Service
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ;
    ASSERT_NO_THROW(wms = simulation->add(
                            new ExecutionWithLocationMapTestWMS(
                                    this, hostname)));

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    simulation->getOutput().enableWorkflowTaskTimestamps(true);

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());

    // Check that the output trace makes sense
    ASSERT_EQ(task->getState(), wrench::WorkflowTask::COMPLETED);
    ASSERT_EQ(task->getFailureCount(), 0);
    ASSERT_GT(task->getStartDate(), 0.0);
    ASSERT_GT(task->getEndDate(), 0.0);
    ASSERT_GT(task->getEndDate(), task->getStartDate());
    ASSERT_LT(std::abs(workflow->getCompletionDate() - task->getEndDate()), 0.01);

    std::vector<wrench::SimulationTimestamp<wrench::SimulationTimestampTaskCompletion> *> task_completion_trace =
            simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>();
    ASSERT_EQ(simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>().size(), 1);
    ASSERT_LT(std::abs(
                      simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>()[0]->getContent()->getDate() -
                      task->getEndDate()),
              0.01);
    ASSERT_EQ(simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>()[0]->getContent()->getTask(),
              task);


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/** EXECUTION WITH LOCATION_MAP MULTIPLE SIMULATION TEST             **/
/**********************************************************************/

class ExecutionWithLocationMapMultipleTestWMS : public wrench::ExecutionController {
public:
    ExecutionWithLocationMapMultipleTestWMS(BareMetalComputeServiceOneTaskTest *test,
                                            std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BareMetalComputeServiceOneTaskTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a job
        std::map<std::shared_ptr<wrench::DataFile>, std::vector<std::shared_ptr<wrench::FileLocation>>> file_locations;
        file_locations[test->input_file] = {};
        // First location won't work, but second will
        file_locations[test->input_file].push_back(wrench::FileLocation::LOCATION(test->storage_service2, "/disk2"));
        file_locations[test->input_file].push_back(wrench::FileLocation::LOCATION(test->storage_service1, "/disk1"));
        file_locations[test->output_file] = {};
        file_locations[test->output_file].push_back(wrench::FileLocation::LOCATION(test->storage_service1, "/disk1"));

        auto job = job_manager->createStandardJob(test->task, file_locations);

        // Submit the job
        job_manager->submitJob(job, test->compute_service);

        // Wait for the workflow execution event
        std::shared_ptr<wrench::ExecutionEvent> event = this->waitForNextEvent();
        if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        // bogus lookup #1
        try {
            wrench::StorageService::lookupFile(nullptr, wrench::FileLocation::LOCATION(this->test->storage_service1));
            throw std::runtime_error("Should not have been able to lookup a nullptr file");
        } catch (std::invalid_argument &e) {
        }

        // bogus lookup #2
        try {
            wrench::StorageService::lookupFile(this->test->output_file, nullptr);
            throw std::runtime_error("Should not have been able to lookup a nullptr location");
        } catch (std::invalid_argument &e) {
        }


        if (!wrench::StorageService::lookupFile(this->test->output_file,
                                                wrench::FileLocation::LOCATION(this->test->storage_service1))) {
            throw std::runtime_error("Output file not written to storage service");
        }

        return 0;
    }
};

TEST_F(BareMetalComputeServiceOneTaskTest, ExecutionWithLocationMapMultiple) {
    DO_TEST_WITH_FORK(do_ExecutionWithLocationMapMultiple_test);
}

void BareMetalComputeServiceOneTaskTest::do_ExecutionWithLocationMapMultiple_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("one_task_test");
    //    argv[1] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Compute Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BareMetalComputeService(hostname,
                                                                {std::make_pair(hostname,
                                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                wrench::ComputeService::ALL_RAM))},
                                                                "", {})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            new wrench::SimpleStorageService("OneCoreHost", {"/disk1"})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
                            new wrench::SimpleStorageService("OneCoreHost", {"/disk2"})));

    // Create a File Registry Service
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ;
    ASSERT_NO_THROW(wms = simulation->add(
                            new ExecutionWithLocationMapMultipleTestWMS(
                                    this, hostname)));

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    simulation->getOutput().enableWorkflowTaskTimestamps(true);

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());

    // Check that the output trace makes sense
    ASSERT_EQ(task->getState(), wrench::WorkflowTask::COMPLETED);
    ASSERT_EQ(task->getFailureCount(), 0);
    ASSERT_GT(task->getStartDate(), 0.0);
    ASSERT_GT(task->getEndDate(), 0.0);
    ASSERT_GT(task->getEndDate(), task->getStartDate());
    ASSERT_LT(std::abs(workflow->getCompletionDate() - task->getEndDate()), 0.01);

    std::vector<wrench::SimulationTimestamp<wrench::SimulationTimestampTaskCompletion> *> task_completion_trace =
            simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>();
    ASSERT_EQ(simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>().size(), 1);
    ASSERT_LT(std::abs(
                      simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>()[0]->getContent()->getDate() -
                      task->getEndDate()),
              0.01);
    ASSERT_EQ(simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>()[0]->getContent()->getTask(),
              task);

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/** EXECUTION WITH DEFAULT STORAGE SERVICE SIMULATION TEST           **/
/**********************************************************************/

class ExecutionWithDefaultStorageServiceTestWMS : public wrench::ExecutionController {
public:
    ExecutionWithDefaultStorageServiceTestWMS(BareMetalComputeServiceOneTaskTest *test,
                                              std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BareMetalComputeServiceOneTaskTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a job
        auto job = job_manager->createStandardJob(test->task);

        // Submit the job
        job_manager->submitJob(job, test->compute_service);

        // Wait for the workflow execution event
        std::shared_ptr<wrench::ExecutionEvent> event = this->waitForNextEvent();
        if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event!");
        }

        if (not wrench::StorageService::lookupFile(this->test->output_file,
                                                   wrench::FileLocation::LOCATION(this->test->storage_service1,
                                                                                  "/scratch/" + job->getName()))) {
            throw std::runtime_error("Output file not written to storage service");
        }

        return 0;
    }
};

TEST_F(BareMetalComputeServiceOneTaskTest, DISABLED_ExecutionWithDefaultStorageService) {
    DO_TEST_WITH_FORK(do_ExecutionWithDefaultStorageService_test);
}

void BareMetalComputeServiceOneTaskTest::do_ExecutionWithDefaultStorageService_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("one_task_test");

    ASSERT_THROW(simulation->launch(), std::runtime_error);

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            new wrench::SimpleStorageService(hostname, {"/disk1"})));

    // Create a Compute Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BareMetalComputeService(hostname,
                                                                {std::make_pair(hostname,
                                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                wrench::ComputeService::ALL_RAM))},
                                                                {})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ;
    ASSERT_NO_THROW(wms = simulation->add(
                            new ExecutionWithDefaultStorageServiceTestWMS(this, hostname)));

    // Create a File Registry Service
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());

    // Check that the output trace makes sense
    ASSERT_EQ(task->getState(), wrench::WorkflowTask::COMPLETED);
    ASSERT_EQ(task->getFailureCount(), 0);
    ASSERT_GT(task->getStartDate(), 0.0);
    ASSERT_GT(task->getEndDate(), 0.0);
    ASSERT_GT(task->getEndDate(), task->getStartDate());

    std::vector<wrench::SimulationTimestamp<wrench::SimulationTimestampTaskCompletion> *> task_completion_trace =
            simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>();
    ASSERT_EQ(simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>().size(), 1);
    ASSERT_EQ(simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>()[0]->getContent()->getDate(),
              task->getEndDate());
    ASSERT_EQ(simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>()[0]->getContent()->getTask(),
              task);


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/** EXECUTION WITH PRE/POST COPIES AND CLEANUP SIMULATION TEST       **/
/**********************************************************************/

class ExecutionWithPrePostCopiesAndCleanupTestWMS : public wrench::ExecutionController {
public:
    ExecutionWithPrePostCopiesAndCleanupTestWMS(
            BareMetalComputeServiceOneTaskTest *test,
            std::string &hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:
    BareMetalComputeServiceOneTaskTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a job
        auto job = job_manager->createStandardJob(
                {test->task},
                {{test->input_file, wrench::FileLocation::LOCATION((test->storage_service1))},
                 {test->output_file, wrench::FileLocation::LOCATION(
                                             test->storage_service2)}},//changed this since we don't have default storage now
                {std::make_tuple(test->input_file, wrench::FileLocation::LOCATION((test->storage_service1)),
                                 wrench::FileLocation::LOCATION(test->storage_service2))},
                {std::make_tuple(test->output_file, wrench::FileLocation::LOCATION(test->storage_service2),
                                 wrench::FileLocation::LOCATION((test->storage_service1)))},
                {std::make_tuple(test->input_file, wrench::FileLocation::LOCATION(test->storage_service2)),
                 std::make_tuple(test->output_file, wrench::FileLocation::LOCATION(test->storage_service2))});
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

        // Test file locations
        if (not wrench::StorageService::lookupFile(
                    this->test->input_file,
                    wrench::FileLocation::LOCATION(this->test->storage_service1))) {
            throw std::runtime_error("Input file should be on Storage Service #1");
        }
        if (not wrench::StorageService::lookupFile(
                    this->test->output_file,
                    wrench::FileLocation::LOCATION(this->test->storage_service1))) {
            throw std::runtime_error("Output file should be on Storage Service #1");
        }
        if (wrench::StorageService::lookupFile(
                    this->test->input_file,
                    wrench::FileLocation::LOCATION(this->test->storage_service2))) {
            throw std::runtime_error("Input file should not be on Storage Service #2");
        }
        if (wrench::StorageService::lookupFile(
                    this->test->output_file,
                    wrench::FileLocation::LOCATION(this->test->storage_service2))) {
            throw std::runtime_error("Output file should not be on Storage Service #2");
        }

        return 0;
    }
};

TEST_F(BareMetalComputeServiceOneTaskTest, ExecutionWithPrePostCopiesTaskCleanup) {
    DO_TEST_WITH_FORK(do_ExecutionWithPrePostCopiesTaskCleanup_test)
}

void BareMetalComputeServiceOneTaskTest::do_ExecutionWithPrePostCopiesTaskCleanup_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("one_task_test");
    //    argv[1] = strdup("--wrench-full-log");

    ASSERT_THROW(simulation->launch(), std::runtime_error);

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

    // Get a hostname
    std::string hostname = "TwoCoreHost";

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            new wrench::SimpleStorageService(hostname, {"/disk1"})));

    // Create another Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
                            new wrench::SimpleStorageService(hostname, {"/disk2"})));


    // Create a Compute Service with default Storage Service #2
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BareMetalComputeService(hostname,
                                                                {std::make_pair(hostname,
                                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                wrench::ComputeService::ALL_RAM))},
                                                                "", {})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ;
    ASSERT_NO_THROW(wms = simulation->add(
                            new ExecutionWithPrePostCopiesAndCleanupTestWMS(this, hostname)));

    // Create a File Registry Service
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

    // Staging the input_file on storage service #1
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    simulation->getOutput().enableWorkflowTaskTimestamps(true);

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());

    // Check that the output trace makes sense
    ASSERT_EQ(task->getState(), wrench::WorkflowTask::COMPLETED);
    ASSERT_EQ(task->getFailureCount(), 0);
    ASSERT_GT(task->getStartDate(), 0.0);
    ASSERT_GT(task->getEndDate(), 0.0);
    ASSERT_GT(task->getEndDate(), task->getStartDate());

    std::vector<wrench::SimulationTimestamp<wrench::SimulationTimestampTaskCompletion> *> task_completion_trace =
            simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>();
    ASSERT_EQ(simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>().size(), 1);
    ASSERT_LT(std::abs(
                      simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>()[0]->getContent()->getDate() -
                      task->getEndDate()),
              0.01);
    ASSERT_EQ(simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>()[0]->getContent()->getTask(),
              task);


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/*************************************************************************/
/** EXECUTION WITH PRE/POST COPIES NO TASK  NO CLEANUP SIMULATION TEST  **/
/*************************************************************************/

class ExecutionWithPrePostCopiesNoTaskNoCleanupTestWMS : public wrench::ExecutionController {
public:
    ExecutionWithPrePostCopiesNoTaskNoCleanupTestWMS(
            BareMetalComputeServiceOneTaskTest *test,
            std::string &hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:
    BareMetalComputeServiceOneTaskTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a job
        auto job = job_manager->createStandardJob({},
                                                  (std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>>){},
                                                  {std::make_tuple(test->input_file,
                                                                   wrench::FileLocation::LOCATION(
                                                                           test->storage_service1),
                                                                   wrench::FileLocation::LOCATION(
                                                                           test->storage_service2))},
                                                  {std::make_tuple(test->input_file,
                                                                   wrench::FileLocation::LOCATION(
                                                                           test->storage_service2),
                                                                   wrench::FileLocation::LOCATION(
                                                                           test->storage_service3))},
                                                  {});
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

        // Test file locations
        if (not wrench::StorageService::lookupFile(this->test->input_file,
                                                   wrench::FileLocation::LOCATION(this->test->storage_service2))) {
            throw std::runtime_error("Input file should be on Storage Service #2");
        }
        if (not wrench::StorageService::lookupFile(this->test->input_file,
                                                   wrench::FileLocation::LOCATION(this->test->storage_service3))) {
            throw std::runtime_error("Input file should be on Storage Service #3");
        }

        return 0;
    }
};

TEST_F(BareMetalComputeServiceOneTaskTest, ExecutionWithPrePostCopiesNoTaskNoCleanup) {
    DO_TEST_WITH_FORK(do_ExecutionWithPrePostCopiesNoTaskNoCleanup_test)
}

void BareMetalComputeServiceOneTaskTest::do_ExecutionWithPrePostCopiesNoTaskNoCleanup_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("one_task_test");

    ASSERT_THROW(simulation->launch(), std::runtime_error);

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            new wrench::SimpleStorageService(hostname, {"/disk1"})));

    // Create another Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
                            new wrench::SimpleStorageService(hostname, {"/disk2"})));

    // Create another Storage Service
    ASSERT_NO_THROW(storage_service3 = simulation->add(
                            new wrench::SimpleStorageService(hostname, {"/disk3"})));


    // Create a Compute Service with default Storage Service #2
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BareMetalComputeService(hostname,
                                                                {std::make_pair(hostname,
                                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                wrench::ComputeService::ALL_RAM))},
                                                                "/scratch",
                                                                {})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ;
    ASSERT_NO_THROW(wms = simulation->add(
                            new ExecutionWithPrePostCopiesNoTaskNoCleanupTestWMS(this, hostname)));

    // Create a File Registry Service
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

    // Staging the input_file on storage service #1
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/*************************************************************************/
/** EXECUTION WITH PRE, NO POST COPIES NO TASK CLEANUP SIMULATION TEST  **/
/*************************************************************************/

class ExecutionWithPreNoPostCopiesNoTaskCleanupTestWMS : public wrench::ExecutionController {
public:
    ExecutionWithPreNoPostCopiesNoTaskCleanupTestWMS(BareMetalComputeServiceOneTaskTest *test,
                                                     std::string &hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:
    BareMetalComputeServiceOneTaskTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a job
        auto job = job_manager->createStandardJob({},
                                                  (std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>>){},
                                                  {std::make_tuple(test->input_file,
                                                                   wrench::FileLocation::LOCATION(
                                                                           test->storage_service1),
                                                                   wrench::FileLocation::LOCATION(
                                                                           test->storage_service2))},
                                                  {},
                                                  {std::make_tuple(test->input_file,
                                                                   wrench::FileLocation::LOCATION(
                                                                           test->storage_service2))});
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

        // Test file locations
        if (wrench::StorageService::lookupFile(this->test->input_file,
                                               wrench::FileLocation::LOCATION(this->test->storage_service2))) {
            throw std::runtime_error("Input file should not be on Storage Service #2");
        }

        return 0;
    }
};

TEST_F(BareMetalComputeServiceOneTaskTest, ExecutionWithPreNoPostCopiesNoTaskCleanup) {
    DO_TEST_WITH_FORK(do_ExecutionWithPreNoPostCopiesNoTaskCleanup_test)
}

void BareMetalComputeServiceOneTaskTest::do_ExecutionWithPreNoPostCopiesNoTaskCleanup_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("one_task_test");

    ASSERT_THROW(simulation->launch(), std::runtime_error);

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            new wrench::SimpleStorageService(hostname, {"/disk1"})));

    // Create another Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
                            new wrench::SimpleStorageService(hostname, {"/disk2"})));


    // Create a Compute Service with default Storage Service #2
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BareMetalComputeService(hostname,
                                                                {std::make_pair(hostname,
                                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                wrench::ComputeService::ALL_RAM))},
                                                                "/scratch",
                                                                {})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ;
    ASSERT_NO_THROW(wms = simulation->add(
                            new ExecutionWithPreNoPostCopiesNoTaskCleanupTestWMS(this, hostname)));

    // Create a File Registry Service
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

    // Staging the input_file on storage service #1
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/********************************************************/
/** EXECUTION WITH MISSING FILE  SIMULATION TEST       **/
/********************************************************/

class ExecutionWithMissingFileTestWMS : public wrench::ExecutionController {
public:
    ExecutionWithMissingFileTestWMS(BareMetalComputeServiceOneTaskTest *test,
                                    std::string &hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:
    BareMetalComputeServiceOneTaskTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        // Remove the staged file!
        wrench::StorageService::deleteFile(test->input_file,
                                           wrench::FileLocation::LOCATION(test->storage_service1));

        // Create a job ubmit the job
        std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>> file_locations;
        file_locations[test->input_file] = wrench::FileLocation::LOCATION(test->storage_service2);
        file_locations[test->output_file] = wrench::FileLocation::LOCATION(test->storage_service2);
        auto job = job_manager->createStandardJob(test->task, file_locations);
        job_manager->submitJob(job, test->compute_service);

        // Wait for the workflow execution event
        std::shared_ptr<wrench::ExecutionEvent> event = this->waitForNextEvent();
        auto real_event = std::dynamic_pointer_cast<wrench::StandardJobFailedEvent>(event);
        if (real_event) {
            auto cause = std::dynamic_pointer_cast<wrench::FileNotFound>(real_event->failure_cause);
            if (not cause) {
                throw std::runtime_error(
                        "Got an Standard Job Failure as expected, but unexpected failure cause: " +
                        real_event->failure_cause->toString() + " (expected: FileNotFound");
            }
            std::string error_msg = cause->toString();
            if (cause->getFile() != test->input_file) {
                throw std::runtime_error(
                        "Got the expected failure, but the failure cause does not point to the right file");
            }
        } else {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }


        return 0;
    }
};

TEST_F(BareMetalComputeServiceOneTaskTest, ExecutionWithMissingFile) {
    DO_TEST_WITH_FORK(do_ExecutionWithMissingFile_test)
}

void BareMetalComputeServiceOneTaskTest::do_ExecutionWithMissingFile_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("one_task_test");
    //    argv[1] = strdup("--wrench-full-log");

    ASSERT_THROW(simulation->launch(), std::runtime_error);

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            new wrench::SimpleStorageService(hostname, {"/disk1"})));

    // Create another Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
                            new wrench::SimpleStorageService(hostname, {"/disk2"})));


    // Create a Compute Service with no default Storage Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BareMetalComputeService(hostname,
                                                                {std::make_pair(hostname,
                                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                wrench::ComputeService::ALL_RAM))},
                                                                "/scratch",
                                                                {})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ;
    ASSERT_NO_THROW(wms = simulation->add(
                            new ExecutionWithMissingFileTestWMS(this, hostname)));

    // Create a File Registry Service
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

    // Staging the input_file on storage service #1
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());

    ASSERT_EQ(this->task->getFailureCount(), 1);
    ASSERT_GT(this->task->getFailureDate(), 0.0);
    ASSERT_LT(this->task->getFailureDate(), wrench::Simulation::getCurrentSimulatedDate());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/********************************************************/
/** EXECUTION WITH NOT ENOUGH CORES SIMULATION TEST    **/
/********************************************************/

class ExecutionWithNotEnoughCoresTestWMS : public wrench::ExecutionController {
public:
    ExecutionWithNotEnoughCoresTestWMS(BareMetalComputeServiceOneTaskTest *test,
                                       std::string &hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:
    BareMetalComputeServiceOneTaskTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create another task1
        std::shared_ptr<wrench::WorkflowTask> task_big = this->test->workflow->addTask("task2", 3600, 2, 2, 2048);

        // Create a job
        auto job = job_manager->createStandardJob(task_big);
        // Submit the job
        try {
            job_manager->submitJob(job, test->compute_service, {{"task2", "OneCoreHost:2"}});
            throw std::runtime_error("Should not be able to submit a job to a service without enough cores");
        } catch (wrench::ExecutionException &e) {
            auto cause = std::dynamic_pointer_cast<wrench::NotEnoughResources>(e.getCause());
            if (not cause) {
                throw std::runtime_error("Received the expected exception, but unexpected failure cause: " +
                                         e.getCause()->toString() + " (expected: NotEnoughResources)");
            }
            std::string error_msg = cause->toString();
            if (cause->getJob() != job) {
                throw std::runtime_error(
                        "Got the expected failure, but the failure cause does not point to the right job");
            }
            if (cause->getService() != test->compute_service) {
                throw std::runtime_error(
                        "Got the expected failure, but the failure cause does not point to the right compute service");
            }
        }

        return 0;
    }
};

TEST_F(BareMetalComputeServiceOneTaskTest, ExecutionWithNotEnoughCores) {
    DO_TEST_WITH_FORK(do_ExecutionWithNotEnoughCores_test)
}

void BareMetalComputeServiceOneTaskTest::do_ExecutionWithNotEnoughCores_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("one_task_test");
    //    argv[1] = strdup("--wrench-full-log");

    ASSERT_THROW(simulation->launch(), std::runtime_error);

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            new wrench::SimpleStorageService(hostname, {"/disk1"})));

    // Create another Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
                            new wrench::SimpleStorageService(hostname, {"/disk3"})));


    // Create a Compute Service with no default Storage Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BareMetalComputeService("OneCoreHost",
                                                                {std::make_pair("OneCoreHost",
                                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                wrench::ComputeService::ALL_RAM))},
                                                                "/scratch",
                                                                {})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ;
    ASSERT_NO_THROW(wms = simulation->add(
                            new ExecutionWithNotEnoughCoresTestWMS(this, hostname)));

    // Create a File Registry Service
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

    // Staging the input_file on storage service #1
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/********************************************************/
/** EXECUTION WITH NOT ENOUGH RAM SIMULATION TEST      **/
/********************************************************/

class ExecutionWithNotEnoughRAMTestWMS : public wrench::ExecutionController {
public:
    ExecutionWithNotEnoughRAMTestWMS(BareMetalComputeServiceOneTaskTest *test,
                                     std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BareMetalComputeServiceOneTaskTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        std::shared_ptr<wrench::WorkflowTask> task_big = this->test->workflow->addTask("task2", 3600, 2, 2, 2048);

        // Create a job
        auto job = job_manager->createStandardJob(task_big);
        // Submit the job
        try {
            job_manager->submitJob(job, test->compute_service);
            throw std::runtime_error("Should not be able to submit a job to a service without enough RAM");
        } catch (wrench::ExecutionException &e) {
            auto cause = std::dynamic_pointer_cast<wrench::NotEnoughResources>(e.getCause());
            if (not cause) {
                throw std::runtime_error("Received the expected exception, but unexpected failure cause: " +
                                         e.getCause()->toString() + " (expected: NotEnoughResources)");
            }
            std::string error_msg = cause->toString();
            if (cause->getJob() != job) {
                throw std::runtime_error(
                        "Got the expected failure, but the failure cause does not point to the right job");
            }
            if (cause->getService() != test->compute_service) {
                throw std::runtime_error(
                        "Got the expected failure, but the failure cause does not point to the right compute service");
            }
        }

        return 0;
    }
};

TEST_F(BareMetalComputeServiceOneTaskTest, ExecutionWithNotEnoughRAM) {
    DO_TEST_WITH_FORK(do_ExecutionWithNotEnoughRAM_test)
}

void BareMetalComputeServiceOneTaskTest::do_ExecutionWithNotEnoughRAM_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("one_task_test");
    //    argv[1] = strdup("--wrench-full-log");

    ASSERT_THROW(simulation->launch(), std::runtime_error);

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            new wrench::SimpleStorageService(hostname, {"/disk1"})));

    // Create another Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
                            new wrench::SimpleStorageService(hostname, {"/disk2"})));


    // Create a Compute Service with no default Storage Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BareMetalComputeService("RAMHost",
                                                                {std::make_pair("RAMHost",
                                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                wrench::ComputeService::ALL_RAM))},
                                                                "/scratch",
                                                                {})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ;
    ASSERT_NO_THROW(wms = simulation->add(
                            new ExecutionWithNotEnoughRAMTestWMS(this, hostname)));

    // Create a File Registry Service
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

    // Staging the input_file on storage service #1
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/** EXECUTION WITH DOWN SERVICE SIMULATION TEST                      **/
/**********************************************************************/

class ExecutionWithDownServiceTestWMS : public wrench::ExecutionController {
public:
    ExecutionWithDownServiceTestWMS(BareMetalComputeServiceOneTaskTest *test,
                                    std::string &hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:
    BareMetalComputeServiceOneTaskTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        std::shared_ptr<wrench::StandardJob> job = nullptr;

        // Shutdown the service
        test->compute_service->stop();

        // Create a job
        job = job_manager->createStandardJob(test->task,
                                             {{test->input_file, wrench::FileLocation::LOCATION(
                                                                         test->storage_service1)},
                                              {test->output_file, wrench::FileLocation::LOCATION(
                                                                          test->storage_service1)}});

        // Submit the job
        try {
            job_manager->submitJob(job, test->compute_service);
            throw std::runtime_error("Should not be able to submit a job to a service that is down");
        } catch (wrench::ExecutionException &e) {
            auto cause = std::dynamic_pointer_cast<wrench::ServiceIsDown>(e.getCause());
            if (not cause) {
                throw std::runtime_error("Got the expected exception, but an expected failure cause: " +
                                         e.getCause()->toString() + "(expected: ServiceIsDown)");
            }
            std::string error_msg = cause->toString();
            if (cause->getService() != test->compute_service) {
                throw std::runtime_error(
                        "Got the expected failure, but the failure cause does not point to the right compute service");
            }
        }

        return 0;
    }
};

TEST_F(BareMetalComputeServiceOneTaskTest, ExecutionWithDownService) {
    DO_TEST_WITH_FORK(do_ExecutionWithDownService_test);
}

void BareMetalComputeServiceOneTaskTest::do_ExecutionWithDownService_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("one_task_test");
    //    argv[1] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Compute Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BareMetalComputeService(hostname,
                                                                {std::make_pair(hostname,
                                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                wrench::ComputeService::ALL_RAM))},
                                                                "/scratch",
                                                                {})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            new wrench::SimpleStorageService(hostname, {"/disk1"})));

    // Create a File Registry Service
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ;
    ASSERT_NO_THROW(wms = simulation->add(
                            new ExecutionWithDownServiceTestWMS(
                                    this, hostname)));

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/** EXECUTION WITH SUSPENDED SERVICE SIMULATION TEST                 **/
/**********************************************************************/

class ExecutionWithSuspendedServiceTestWMS : public wrench::ExecutionController {
public:
    ExecutionWithSuspendedServiceTestWMS(BareMetalComputeServiceOneTaskTest *test,
                                         std::string &hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:
    BareMetalComputeServiceOneTaskTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        std::shared_ptr<wrench::StandardJob> job = nullptr;

        // Suspend the service
        test->compute_service->suspend();


        // Create a job
        job = job_manager->createStandardJob(test->task,
                                             {{test->input_file, wrench::FileLocation::LOCATION(
                                                                         test->storage_service1)},
                                              {test->output_file, wrench::FileLocation::LOCATION(
                                                                          test->storage_service1)}});

        // Submit the job
        try {
            job_manager->submitJob(job, test->compute_service);
            throw std::runtime_error("Should not be able to submit a job to a service that is down");
        } catch (wrench::ExecutionException &e) {
            auto cause = std::dynamic_pointer_cast<wrench::ServiceIsSuspended>(e.getCause());
            if (not cause) {
                throw std::runtime_error("Got the expected exception, but an expected failure cause: " +
                                         e.getCause()->toString() + "(expected: ServiceIsSuspended)");
            }
            std::string error_msg = cause->toString();
            if (cause->getService() != test->compute_service) {
                throw std::runtime_error(
                        "Got the expected failure, but the failure cause does not point to the right compute service");
            }
        }

        // Sleep for 1 sec
        wrench::Simulation::sleep(1);

        // Resume the service
        test->compute_service->resume();

        // Submit the job again
        try {
            job_manager->submitJob(job, test->compute_service);
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Should  be able to submit a job to a service that has been resumed");
        }

        return 0;
    }
};

TEST_F(BareMetalComputeServiceOneTaskTest, ExecutionWithSuspendedService) {
    DO_TEST_WITH_FORK(do_ExecutionWithSuspendedService_test);
}

void BareMetalComputeServiceOneTaskTest::do_ExecutionWithSuspendedService_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("one_task_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Compute Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BareMetalComputeService(hostname,
                                                                {std::make_pair(hostname,
                                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                wrench::ComputeService::ALL_RAM))},
                                                                "/scratch",
                                                                {})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            new wrench::SimpleStorageService(hostname, {"/disk1"})));

    // Create a File Registry Service
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ;
    ASSERT_NO_THROW(wms = simulation->add(
                            new ExecutionWithSuspendedServiceTestWMS(
                                    this, hostname)));

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
