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

WRENCH_LOG_CATEGORY(bare_metal_compute_service_one_action_test, "Log category for BareMetalComputeServiceOneAction test");

class BareMetalComputeServiceOneActionTest : public ::testing::Test {
public:
    wrench::WorkflowFile *input_file;
    wrench::WorkflowFile *output_file;
    wrench::WorkflowTask *task;
    std::shared_ptr<wrench::StorageService> storage_service1 = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service2 = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service3 = nullptr;
    std::shared_ptr<wrench::BareMetalComputeService> compute_service = nullptr;

    void do_BadSetup_test();
    void do_Noop_test();
    void do_OneSleepAction_test();
    void do_OneComputeActionNotEnoughResources_test();
    void do_OneComputeActionBogusServiceSpecificArgs_test();
    void do_OneSleepActionServiceCrashed_test();
    void do_OneSleepJobTermination_test();
    void do_OneSleepActionServiceCrashedRestarted_test();
    void do_OneFileReadActionFileNotThere_test();
    void do_OneSleepActionServiceDown_test();
    void do_OneSleepActionServiceSuspended_test();
    void do_OneSleepActionBadScratch_test();

protected:
    BareMetalComputeServiceOneActionTest() {

        // Create the simplest workflow
        workflow_unique_ptr = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());
        workflow = workflow_unique_ptr.get();

        // Create two files
        input_file = workflow->addFile("input_file", 10000.0);
        output_file = workflow->addFile("output_file", 20000.0);

        // Create a platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk1\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/disk1/\"/>"
                          "          </disk>"
                          "          <disk id=\"large_disk2\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/disk2/\"/>"
                          "          </disk>"

                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Host2\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Host3\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Host4\" speed=\"1f\" core=\"10\">  "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "         <prop id=\"ram\" value=\"1024B\"/> "
                          "       </host>  "
                          "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                          "       <link id=\"2\" bandwidth=\"100MBps\" latency=\"0us\"/>"
                          "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host2\" dst=\"Host3\"> <link_ctn id=\"2\"/> </route>"
                          "       <route src=\"Host2\" dst=\"Host4\"> <link_ctn id=\"2\"/> </route>"
                          "       <route src=\"Host3\" dst=\"Host4\"> <link_ctn id=\"2\"/> </route>"
                          "       <route src=\"Host1\" dst=\"Host4\"> <link_ctn id=\"2\"/> </route>"
                          "       <route src=\"Host1\" dst=\"Host3\"> <link_ctn id=\"2\"/> </route>"
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
/**  BAD SETUP SIMULATION TEST                                       **/
/**********************************************************************/

class BadSetupTestWMS : public wrench::WMS {
public:
    BadSetupTestWMS(BareMetalComputeServiceOneActionTest *test,
                    const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                    const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                    std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    BareMetalComputeServiceOneActionTest *test;

    int main() {

        return 0;
    }
};

TEST_F(BareMetalComputeServiceOneActionTest, BadSetup) {
    DO_TEST_WITH_FORK(do_BadSetup_test);
}

void BareMetalComputeServiceOneActionTest::do_BadSetup_test() {
    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();

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
    argv[0] = strdup("one_action_test");
//    argv[1] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Empty resource list
    ASSERT_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService("bogus",
                                                (std::map<std::string, std::tuple<unsigned long, double>>) {},
                                                {})), std::invalid_argument);

    // Bad hostname
    ASSERT_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService("bogus",
                                                {std::make_pair("bogus",
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                {})), std::invalid_argument);

    // Get a hostname
    auto hostname = wrench::Simulation::getHostnameList()[0];

    // Bad resource hostname
    ASSERT_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair("bogus",
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))}, "",
                                                {})), std::invalid_argument);

    // Bad number of cores
    ASSERT_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair(hostname,
                                                                std::make_tuple(0,
                                                                                wrench::ComputeService::ALL_RAM))}, "",
                                                {})), std::invalid_argument);

    // Bad number of cores
    ASSERT_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair(hostname,
                                                                std::make_tuple(100,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                {})), std::invalid_argument);


    // Bad RAM
    ASSERT_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair("RAMHost",
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                -1.0))},
                                                {})), std::invalid_argument);

    // Bad RAM
    ASSERT_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair("RAMHost",
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                100000.0))},
                                                {})), std::invalid_argument);

    // Bad PROPERTIES
    ASSERT_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair("RAMHost",
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                100000.0))},
                                                "",
                                                {
                                                        std::make_pair(
                                                                wrench::BareMetalComputeServiceProperty::TASK_STARTUP_OVERHEAD,
                                                                "-1.0")
                                                },
                                                {})), std::invalid_argument);

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(new BadSetupTestWMS(this,
                                                              {}, {}, hostname)));

    ASSERT_THROW(wms->addWorkflow(nullptr), std::invalid_argument);
    ASSERT_NO_THROW(wms->addWorkflow(this->workflow));
    ASSERT_THROW(wms->addWorkflow(this->workflow), std::invalid_argument);

    // Running a "run a single task1" simulation
    ASSERT_THROW(simulation->launch(), std::runtime_error);

    delete simulation;

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  NOOP SIMULATION TEST                                            **/
/**********************************************************************/

class NoopTestWMS : public wrench::WMS {
public:
    NoopTestWMS(BareMetalComputeServiceOneActionTest *test,
                const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    BareMetalComputeServiceOneActionTest *test;

    int main() {

        wrench::TerminalOutput::disableColor(); // just for increasing stupid coverage

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
        (*(this->getAvailableComputeServices<wrench::BareMetalComputeService>().begin()))->stop();

        return 0;
    }
};

TEST_F(BareMetalComputeServiceOneActionTest, Noop) {
    DO_TEST_WITH_FORK(do_Noop_test);
}

void BareMetalComputeServiceOneActionTest::do_Noop_test() {
    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();

    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("one_action_test");
//    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

    ASSERT_THROW(simulation->add((wrench::ComputeService *) nullptr), std::invalid_argument);

    // Create a Compute Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService("Host3",
                                                {std::make_pair("Host4",
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                {"/scratch"},
                                                {})));

    // Create a Storage Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService("Host2", {"/"})));

    // Create a WMS
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::WMS> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
            new NoopTestWMS(
                    this,
                    {compute_service}, {
                            storage_service1
                    }, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Running a "do nothing" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  ONE SLEEP ACTION TEST                                           **/
/**********************************************************************/

class OneSleepActionTestWMS : public wrench::WMS {
public:
    OneSleepActionTestWMS(BareMetalComputeServiceOneActionTest *test,
                          const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                          const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                          std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    BareMetalComputeServiceOneActionTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a compound job and submit it
        auto job = job_manager->createCompoundJob("my_job");
        auto action = job->addSleepAction("my_sleep", 10.0);
        job_manager->submitJob(job, this->test->compute_service, {});

        // Wait for the workflow execution event
        std::shared_ptr<wrench::ExecutionEvent> event = this->waitForNextEvent();
        if (not std::dynamic_pointer_cast<wrench::CompoundJobCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        // Check event content
        auto real_event = std::dynamic_pointer_cast<wrench::CompoundJobCompletedEvent>(event);
        if (real_event->job != job) {
            throw std::runtime_error("Event's job isn't the right job!");
        }
        if (real_event->compute_service != this->test->compute_service)  {
            throw std::runtime_error("Event's compute service isn't the right compute service!");
        }

        // Check job state
        if (job->getState() != wrench::CompoundJob::State::COMPLETED) {
            throw std::runtime_error("Unexpected job state: " + job->getStateAsString());
        }

        // Chek action stuff
        if (action->getState() != wrench::Action::State::COMPLETED) {
            throw std::runtime_error("Unexpected action state " + action->getStateAsString());
        }

        if ((action->getStartDate() > 0.0001) or (fabs(action->getEndDate() - 10.0) > 0.0001)) {
            throw std::runtime_error("Unexpected action stard/end dates");
        }

//        std::cerr << action->getStartDate() << "\n";
//        std::cerr << action->getEndDate() << "\n";

        // Stop the Job Manager manually, just for kicks
        job_manager->stop();

        // Stop the Data Movement Manager manually, just for kicks
        data_movement_manager->stop();

        wrench::Simulation::sleep(1);

        // Stop the Compute service manually, for coverage
        (*(this->getAvailableComputeServices<wrench::BareMetalComputeService>().begin()))->stop();

        return 0;
    }
};

TEST_F(BareMetalComputeServiceOneActionTest, OneSleepAction) {
    DO_TEST_WITH_FORK(do_OneSleepAction_test);
}

void BareMetalComputeServiceOneActionTest::do_OneSleepAction_test() {
    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();

    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("one_action_test");
//    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

    ASSERT_THROW(simulation->add((wrench::ComputeService *) nullptr), std::invalid_argument);

    // Create a Compute Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService("Host3",
                                                {std::make_pair("Host4",
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                {"/scratch"},
                                                {})));

    // Create a Storage Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService("Host2", {"/"})));

    // Create a WMS
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::WMS> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
            new OneSleepActionTestWMS(
                    this,
                    {compute_service}, {
                            storage_service1
                    }, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    simulation->add(new wrench::FileRegistryService(hostname));

    ASSERT_THROW(simulation->stageFile(input_file, (std::shared_ptr<wrench::StorageService>) nullptr),
                 std::invalid_argument);
    ASSERT_THROW(simulation->stageFile(nullptr, storage_service1), std::invalid_argument);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "do nothing" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  ONE COMPUTE ACTION NOT ENOUGH RESOURCES TEST                    **/
/**********************************************************************/

class OneComputeActionNotEnoughResourcesTestWMS : public wrench::WMS {
public:
    OneComputeActionNotEnoughResourcesTestWMS(BareMetalComputeServiceOneActionTest *test,
                                              const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                              const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                              std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    BareMetalComputeServiceOneActionTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a compound job that asks for too many cores
        auto job1 = job_manager->createCompoundJob("my_job1");
        // Create a compute action that asks for too many cores
        auto action1 = job1->addComputeAction("my_computation", 1000.0, 0.0, 100, 100,
                                              wrench::ParallelModel::AMDAHL(1.0));
        // Create a compound job that asks for too much RAM
        auto job2 = job_manager->createCompoundJob("my_job2");
        // Create a compute action that asks for too many cores
        auto action2 = job2->addComputeAction("my_computation", 1000.0, 1000.0, 1, 1,
                                              wrench::ParallelModel::AMDAHL(1.0));

        std::vector<std::shared_ptr<wrench::CompoundJob>> jobs = {job1, job2};

        for (auto const &job : jobs) {

            // Submit the job
            try {
                job_manager->submitJob(job, this->test->compute_service, {});
                throw std::runtime_error("Shouldn't be able to submit a job that asks for too many resources");
            } catch (wrench::ExecutionException &e) {
                auto cause = std::dynamic_pointer_cast<wrench::NotEnoughResources>(e.getCause());
                if (not cause) {
                    throw std::runtime_error("Unexpected failure cause");
                }
                if (cause->getJob() != job) {
                    throw std::runtime_error("Unexpected job in failure cause");
                }
                if (cause->getService() != this->test->compute_service) {
                    throw std::runtime_error("Unexpected service in failure cause");
                }
            }
        }


        // Stop the Job Manager manually, just for kicks
        job_manager->stop();

        // Stop the Data Movement Manager manually, just for kicks
        data_movement_manager->stop();

        wrench::Simulation::sleep(1);

        // Stop the Compute service manually, for coverage
        (*(this->getAvailableComputeServices<wrench::BareMetalComputeService>().begin()))->stop();

        return 0;
    }
};

TEST_F(BareMetalComputeServiceOneActionTest, OneComputeActionNotEnoughResources) {
    DO_TEST_WITH_FORK(do_OneComputeActionNotEnoughResources_test);
}

void BareMetalComputeServiceOneActionTest::do_OneComputeActionNotEnoughResources_test() {
    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();

    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("one_action_test");
//    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

    ASSERT_THROW(simulation->add((wrench::ComputeService *) nullptr), std::invalid_argument);

    // Create a Compute Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService("Host3",
                                                {std::make_pair("Host4",
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                100.0))},
                                                {"/scratch"},
                                                {})));

    // Create a Storage Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService("Host2", {"/"})));

    // Create a WMS
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::WMS> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
            new OneComputeActionNotEnoughResourcesTestWMS(
                    this,
                    {compute_service}, {
                            storage_service1
                    }, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));


    simulation->add(new wrench::FileRegistryService(hostname));

    ASSERT_THROW(simulation->stageFile(input_file, (std::shared_ptr<wrench::StorageService>) nullptr),
                 std::invalid_argument);
    ASSERT_THROW(simulation->stageFile(nullptr, storage_service1), std::invalid_argument);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "do nothing" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}



/**********************************************************************/
/**  ONE COMPUTE ACTION BOGUS SERVICE-SPECIFIC ARGS TEST             **/
/**********************************************************************/

class OneComputeActionBogusServiceSpecificArgsTestWMS : public wrench::WMS {
public:
    OneComputeActionBogusServiceSpecificArgsTestWMS(BareMetalComputeServiceOneActionTest *test,
                                                    const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                                    const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                                    std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    BareMetalComputeServiceOneActionTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a compound job that asks for too many cores
        auto job = job_manager->createCompoundJob("my_job");
        // Create a compute action
        auto action = job->addComputeAction("my_computation", 1000.0, 0.0, 2, 15,
                                            wrench::ParallelModel::AMDAHL(1.0));

        // Submit the job with bogus args
        std::vector<std::map<std::string, std::string>> bogus_args;
        bogus_args.push_back({{"bogus_action", "somehost"}});
        bogus_args.push_back({{"my_computation", "Host4:30:123:12:ba"}});
        bogus_args.push_back({{"my_computation", "Host4:1"}});
        bogus_args.push_back({{"my_computation", "Host4:12"}});
        bogus_args.push_back({{"my_computation", "Host4:100"}});
        bogus_args.push_back({{"my_computation", "1"}});
        bogus_args.push_back({{"my_computation", "12"}});
        bogus_args.push_back({{"my_computation", "20"}});

        for (auto const &args : bogus_args) {
            try {
                job_manager->submitJob(job, this->test->compute_service, args);
                throw std::runtime_error("Shouldn't have been able to submit job (" + args.begin()->first + ":" + args.begin()->second + ")");
            } catch (std::invalid_argument &ignore) {
//                std::cerr << "Expected exception: " << e.what() << "\n";
            } catch (wrench::ExecutionException &ignore) {

            }
        }

        // Stop the Job Manager manually, just for kicks
        job_manager->stop();

        // Stop the Data Movement Manager manually, just for kicks
        data_movement_manager->stop();

        wrench::Simulation::sleep(1);

        // Stop the Compute service manually, for coverage
        (*(this->getAvailableComputeServices<wrench::BareMetalComputeService>().begin()))->stop();

        return 0;
    }
};

TEST_F(BareMetalComputeServiceOneActionTest, OneComputeActionBogusServiceSpecificArgs) {
    DO_TEST_WITH_FORK(do_OneComputeActionBogusServiceSpecificArgs_test);
}

void BareMetalComputeServiceOneActionTest::do_OneComputeActionBogusServiceSpecificArgs_test() {
    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();

    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("one_action_test");
//    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

    ASSERT_THROW(simulation->add((wrench::ComputeService *) nullptr), std::invalid_argument);

    // Create a Compute Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService("Host3",
                                                {std::make_pair("Host4",
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                100.0))},
                                                {"/scratch"},
                                                {})));

    // Create a Storage Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService("Host2", {"/"})));

    // Create a WMS
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::WMS> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
            new OneComputeActionBogusServiceSpecificArgsTestWMS(
                    this,
                    {compute_service}, {
                            storage_service1
                    }, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));


    simulation->add(new wrench::FileRegistryService(hostname));

    ASSERT_THROW(simulation->stageFile(input_file, (std::shared_ptr<wrench::StorageService>) nullptr),
                 std::invalid_argument);
    ASSERT_THROW(simulation->stageFile(nullptr, storage_service1), std::invalid_argument);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "do nothing" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}



/**********************************************************************/
/**  ONE COMPUTE ACTION SERVICE CRASHED TEST                         **/
/**********************************************************************/

class ServiceCrashedTestWMS : public wrench::WMS {
public:
    ServiceCrashedTestWMS(BareMetalComputeServiceOneActionTest *test,
                          const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                          const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                          std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    BareMetalComputeServiceOneActionTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a compound job and submit it
        auto job = job_manager->createCompoundJob("my_job");
        auto action = job->addSleepAction("my_sleep", 10.0);
        job_manager->submitJob(job, this->test->compute_service, {});

        // Sleep 1 sec
        wrench::Simulation::sleep(1.0);

        // Kill the BareMetalComputeService
        wrench::Simulation::turnOffHost("Host4");

        // Wait for the workflow execution event
        std::shared_ptr<wrench::ExecutionEvent> event = this->waitForNextEvent();
        if (not std::dynamic_pointer_cast<wrench::CompoundJobFailedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        // Check event content
        auto real_event = std::dynamic_pointer_cast<wrench::CompoundJobFailedEvent>(event);
        if (real_event->job != job) {
            throw std::runtime_error("Event's job isn't the right job!");
        }
        if (real_event->compute_service != this->test->compute_service)  {
            throw std::runtime_error("Event's compute service isn't the right compute service!");
        }

        if (not std::dynamic_pointer_cast<wrench::SomeActionsHaveFailed>(real_event->failure_cause)) {
            throw std::runtime_error("Unexpected event-level failure cause");
        }

        // Chek action stuff
        if (action->getState() != wrench::Action::State::FAILED) {
            throw std::runtime_error("Unexpected action state " + action->getStateAsString());
        }

        if (not std::dynamic_pointer_cast<wrench::HostError>(action->getFailureCause())) {
            throw std::runtime_error("Unexpected action failure cause " + action->getFailureCause()->toString());
        }

        if ((action->getStartDate() > 0.0001) or (std::abs<double>(action->getEndDate() - 1.0) > 0)) {
            throw std::runtime_error("Unexpected action start/end dates");
        }


        // Stop the Job Manager manually, just for kicks
        job_manager->stop();

        // Stop the Data Movement Manager manually, just for kicks
        data_movement_manager->stop();

        wrench::Simulation::sleep(1);

        // Stop the Compute service manually, for coverage
        (*(this->getAvailableComputeServices<wrench::BareMetalComputeService>().begin()))->stop();

        return 0;
    }
};

TEST_F(BareMetalComputeServiceOneActionTest, ServiceCrashed) {
    DO_TEST_WITH_FORK(do_OneSleepActionServiceCrashed_test);
}

void BareMetalComputeServiceOneActionTest::do_OneSleepActionServiceCrashed_test() {
    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();

    int argc = 2;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("one_action_test");
    argv[1] = strdup("--wrench-host-shutdown-simulation");
//    argv[2] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

    ASSERT_THROW(simulation->add((wrench::ComputeService *) nullptr), std::invalid_argument);

    // Create a Compute Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService("Host3",
                                                {std::make_pair("Host4",
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                {"/scratch"},
                                                {{wrench::BareMetalComputeServiceProperty::FAIL_ACTION_AFTER_ACTION_EXECUTOR_CRASH, "true"}}, {})));

    // Create a Storage Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService("Host2", {"/"})));

    // Create a WMS
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::WMS> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
            new ServiceCrashedTestWMS(
                    this,
                    {compute_service}, {
                            storage_service1
                    }, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));


    simulation->add(new wrench::FileRegistryService(hostname));

    ASSERT_THROW(simulation->stageFile(input_file, (std::shared_ptr<wrench::StorageService>) nullptr),
                 std::invalid_argument);
    ASSERT_THROW(simulation->stageFile(nullptr, storage_service1), std::invalid_argument);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "do nothing" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  ONE COMPUTE ACTION TERMINATION TEST                             **/
/**********************************************************************/

class JobTerminationTestWMS : public wrench::WMS {
public:
    JobTerminationTestWMS(BareMetalComputeServiceOneActionTest *test,
                          const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                          const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                          std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    BareMetalComputeServiceOneActionTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a compound job and submit it
        auto job = job_manager->createCompoundJob("my_job");
        if (job->getState() != wrench::CompoundJob::State::NOT_SUBMITTED) {
            throw std::runtime_error("Unexpected job state: " + job->getStateAsString());
        }
        auto action = job->addSleepAction("my_sleep", 10.0);
        job_manager->submitJob(job, this->test->compute_service, {});

        // Check job state
        if (job->getState() != wrench::CompoundJob::State::SUBMITTED) {
            throw std::runtime_error("Unexpected job state: " + job->getStateAsString());
        }

        // Sleep 1 sec
        wrench::Simulation::sleep(1.0);

        // Terminate the job
        job_manager->terminateJob(job);

        // Check job state
        if (job->getState() != wrench::CompoundJob::State::DISCONTINUED) {
            throw std::runtime_error("Unexpected job state: " + job->getStateAsString());
        }

        // Chek action stuff
        if (action->getState() != wrench::Action::State::KILLED) {
            throw std::runtime_error("Unexpected action state " + action->getStateAsString());
        }
        if (not std::dynamic_pointer_cast<wrench::JobKilled>(action->getFailureCause())) {
            throw std::runtime_error("Unexpected action failure cause " + action->getFailureCause()->toString());
        }

        return 0;
    }
};

TEST_F(BareMetalComputeServiceOneActionTest, JobTermination) {
    DO_TEST_WITH_FORK(do_OneSleepJobTermination_test);
}

void BareMetalComputeServiceOneActionTest::do_OneSleepJobTermination_test() {
    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();

    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("one_action_test");
//    argv[1] = strdup("--wrench-host-shutdown-simulation");
//    argv[2] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

    ASSERT_THROW(simulation->add((wrench::ComputeService *) nullptr), std::invalid_argument);

    // Create a Compute Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService("Host3",
                                                {std::make_pair("Host4",
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                {"/scratch"},
                                                {{wrench::BareMetalComputeServiceProperty::FAIL_ACTION_AFTER_ACTION_EXECUTOR_CRASH, "true"}}, {})));

    // Create a Storage Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService("Host2", {"/"})));

    // Create a WMS
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::WMS> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
            new JobTerminationTestWMS(
                    this,
                    {compute_service}, {
                            storage_service1
                    }, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));


    simulation->add(new wrench::FileRegistryService(hostname));

    ASSERT_THROW(simulation->stageFile(input_file, (std::shared_ptr<wrench::StorageService>) nullptr),
                 std::invalid_argument);
    ASSERT_THROW(simulation->stageFile(nullptr, storage_service1), std::invalid_argument);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "do nothing" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  ONE SLEEP ACTION SERVICE CRASHED RESTARTED TEST               **/
/**********************************************************************/

class ServiceCrashedRestartedTestWMS : public wrench::WMS {
public:
    ServiceCrashedRestartedTestWMS(BareMetalComputeServiceOneActionTest *test,
                                   const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                   const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                   std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    BareMetalComputeServiceOneActionTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a compound job and submit it
        auto job = job_manager->createCompoundJob("my_job");
        auto action = job->addSleepAction("my_sleep", 10.0);
        job_manager->submitJob(job, this->test->compute_service, {});

        // Sleep 1 sec
        wrench::Simulation::sleep(1.0);

        // Kill the compute host
        wrench::Simulation::turnOffHost("Host4");

        // Sleep 1 sec
        wrench::Simulation::sleep(1.0);

        // Restart the compute host
        wrench::Simulation::turnOnHost("Host4");

        // Wait for the workflow execution event
        std::shared_ptr<wrench::ExecutionEvent> event = this->waitForNextEvent();
        if (not std::dynamic_pointer_cast<wrench::CompoundJobCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        // Check event content
        auto real_event = std::dynamic_pointer_cast<wrench::CompoundJobCompletedEvent>(event);
        if (real_event->job != job) {
            throw std::runtime_error("Event's job isn't the right job!");
        }
        if (real_event->compute_service != this->test->compute_service)  {
            throw std::runtime_error("Event's compute service isn't the right compute service!");
        }

        // Chek action stuff
        if (action->getState() != wrench::Action::State::COMPLETED) {
            throw std::runtime_error("Unexpected action state " + action->getStateAsString());
        }

        if ((std::abs<double>(action->getStartDate() - 2.0) > 0.0001) or (std::abs<double>(action->getEndDate() - 12.0) > 0.0001)) {
            throw std::runtime_error("Unexpected action start/end dates");
        }

        // Check action history
        auto history = action->getExecutionHistory();
        if (history.size() != 2) {
            throw std::runtime_error("Unexpected execution history size");
        }
        auto top = history.top();
        if ((top.failure_cause != nullptr) or
            (std::abs<double>(top.start_date - 2.0) > 0.0001) or
            (std::abs<double>(top.end_date - 12.0) > 0.0001) or
            (top.execution_host != "Host4") or
            (top.num_cores_allocated != 0) or
            (top.state != wrench::Action::State::COMPLETED) or
            (top.ram_allocated != 0.0) or
            (top.physical_execution_host != "Host4")) {
            throw std::runtime_error("Unexpected last history");
        }
        history.pop();
        top = history.top();
        if ((top.failure_cause == nullptr) or
            (std::dynamic_pointer_cast<wrench::HostError>(top.failure_cause) == nullptr) or
            (std::abs<double>(top.start_date - 0.0) > 0.0001) or
            (std::abs<double>(top.end_date - 1.0) > 0.0001) or
            (top.execution_host != "Host4") or
            (top.num_cores_allocated != 0) or
            (top.state != wrench::Action::State::FAILED) or
            (top.ram_allocated != 0.0) or
            (top.physical_execution_host != "Host4")) {
            throw std::runtime_error("Unexpected last history");
        }

        // Stop the Job Manager manually, just for kicks
        job_manager->stop();

        // Stop the Data Movement Manager manually, just for kicks
        data_movement_manager->stop();

        wrench::Simulation::sleep(1);

        // Stop the Compute service manually, for coverage
        (*(this->getAvailableComputeServices<wrench::BareMetalComputeService>().begin()))->stop();

        return 0;
    }
};

TEST_F(BareMetalComputeServiceOneActionTest, ServiceCrashedRestarted) {
    DO_TEST_WITH_FORK(do_OneSleepActionServiceCrashedRestarted_test);
}

void BareMetalComputeServiceOneActionTest::do_OneSleepActionServiceCrashedRestarted_test() {
    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();

    int argc = 2;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("one_action_test");
    argv[1] = strdup("--wrench-host-shutdown-simulation");
//    argv[2] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

    ASSERT_THROW(simulation->add((wrench::ComputeService *) nullptr), std::invalid_argument);

    // Create a Compute Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService("Host3",
                                                {std::make_pair("Host4",
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                {"/scratch"},
                                                {{wrench::BareMetalComputeServiceProperty::FAIL_ACTION_AFTER_ACTION_EXECUTOR_CRASH, "false"}}, {})));

    // Create a Storage Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService("Host2", {"/"})));

    // Create a WMS
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::WMS> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
            new ServiceCrashedRestartedTestWMS(
                    this,
                    {compute_service}, {
                            storage_service1
                    }, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    simulation->add(new wrench::FileRegistryService(hostname));

    ASSERT_THROW(simulation->stageFile(input_file, (std::shared_ptr<wrench::StorageService>) nullptr),
                 std::invalid_argument);
    ASSERT_THROW(simulation->stageFile(nullptr, storage_service1), std::invalid_argument);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "do nothing" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  ONE FILE READ ACTION SERVICE FILE NOT THERE TEST               **/
/**********************************************************************/

class ServiceFileNotThereTestWMS : public wrench::WMS {
public:
    ServiceFileNotThereTestWMS(BareMetalComputeServiceOneActionTest *test,
                               const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                               const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                               std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    BareMetalComputeServiceOneActionTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a compound job and submit it
        auto job = job_manager->createCompoundJob("my_job");
        auto action = job->addFileReadAction("my_file_read", this->test->input_file,
                                             wrench::FileLocation::LOCATION(this->test->storage_service1));
        job_manager->submitJob(job, this->test->compute_service, {});

        // Wait for the workflow execution event
        std::shared_ptr<wrench::ExecutionEvent> event = this->waitForNextEvent();
        if (not std::dynamic_pointer_cast<wrench::CompoundJobFailedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        // Check event content
        auto real_event = std::dynamic_pointer_cast<wrench::CompoundJobFailedEvent>(event);
        if (real_event->job != job) {
            throw std::runtime_error("Event's job isn't the right job!");
        }
        if (real_event->compute_service != this->test->compute_service)  {
            throw std::runtime_error("Event's compute service isn't the right compute service!");
        }
        if (not std::dynamic_pointer_cast<wrench::SomeActionsHaveFailed>(real_event->failure_cause)) {
            throw std::runtime_error("Unexpected event-level failure cause");
        }

        // Check job state
        if (job->getState() != wrench::CompoundJob::State::DISCONTINUED) {
            throw std::runtime_error("Unexpected job state: " + job->getStateAsString());
        }

        // Chek action stuff
        if (action->getState() != wrench::Action::State::FAILED) {
            throw std::runtime_error("Unexpected action state " + action->getStateAsString());
        }

        auto real_failure = std::dynamic_pointer_cast<wrench::FileNotFound>(action->getFailureCause());
        if (not real_failure) {
            throw std::runtime_error("Unexpected action failure cause");
        }
        if (real_failure->getFile() != this->test->input_file) {
            throw std::runtime_error("Unexpected file in the action failure cause");
        }
        if (real_failure->getLocation()->getStorageService() != this->test->storage_service1) {
            throw std::runtime_error("Unexpected storage service in the action failure cause");
        }

        // Stop the Job Manager manually, just for kicks
        job_manager->stop();

        // Stop the Data Movement Manager manually, just for kicks
        data_movement_manager->stop();

        wrench::Simulation::sleep(1);

        // Stop the Compute service manually, for coverage
        (*(this->getAvailableComputeServices<wrench::BareMetalComputeService>().begin()))->stop();

        return 0;
    }
};

TEST_F(BareMetalComputeServiceOneActionTest, FileNotThere) {
    DO_TEST_WITH_FORK(do_OneFileReadActionFileNotThere_test);
}

void BareMetalComputeServiceOneActionTest::do_OneFileReadActionFileNotThere_test() {
    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();

    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("one_action_test");
//    argv[1] = strdup("--wrench-host-shutdown-simulation");
//    argv[2] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

    ASSERT_THROW(simulation->add((wrench::ComputeService *) nullptr), std::invalid_argument);

    // Create a Compute Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService("Host3",
                                                {std::make_pair("Host4",
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                {"/scratch"},
                                                {{wrench::BareMetalComputeServiceProperty::FAIL_ACTION_AFTER_ACTION_EXECUTOR_CRASH, "false"}}, {})));

    // Create a Storage Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService("Host2", {"/"})));

    // Create a WMS
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::WMS> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
            new ServiceFileNotThereTestWMS(
                    this,
                    {compute_service}, {
                            storage_service1
                    }, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Running a "do nothing" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  ONE SLEEP ACTION SERVICE DOWN TEST                              **/
/**********************************************************************/

class ServiceServiceDownTestWMS : public wrench::WMS {
public:
    ServiceServiceDownTestWMS(BareMetalComputeServiceOneActionTest *test,
                              const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                              const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                              std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    BareMetalComputeServiceOneActionTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();


        // Create a compound job
        auto job = job_manager->createCompoundJob("my_job");
        auto action = job->addSleepAction("my_sleep", 10.0);

        // Take down the compute service
        this->test->compute_service->stop();

        // Submit the job
        try {
            job_manager->submitJob(job, this->test->compute_service, {});
            throw std::runtime_error("Shouldn't be able to submit a job to a down service");
        } catch (wrench::ExecutionException &e) {
            auto cause = std::dynamic_pointer_cast<wrench::ServiceIsDown>(e.getCause());
            if (not cause) {
                throw std::runtime_error("Unexpected failure cause");
            }
            if (cause->getService() != this->test->compute_service) {
                throw std::runtime_error("Unexpected service in failure cause");
            }
            // Check job state
            if (job->getState() != wrench::CompoundJob::State::NOT_SUBMITTED) {
                throw std::runtime_error("Unexpected job state: " + job->getStateAsString());
            }
        }

        return 0;
    }
};

TEST_F(BareMetalComputeServiceOneActionTest, ServiceDown) {
    DO_TEST_WITH_FORK(do_OneSleepActionServiceDown_test);
}

void BareMetalComputeServiceOneActionTest::do_OneSleepActionServiceDown_test() {
    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();

    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("one_action_test");
//    argv[1] = strdup("--wrench-host-shutdown-simulation");
//    argv[2] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

    ASSERT_THROW(simulation->add((wrench::ComputeService *) nullptr), std::invalid_argument);

    // Create a Compute Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService("Host3",
                                                {std::make_pair("Host4",
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                {"/scratch"},
                                                {{wrench::BareMetalComputeServiceProperty::FAIL_ACTION_AFTER_ACTION_EXECUTOR_CRASH, "false"}}, {})));

    // Create a Storage Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService("Host2", {"/"})));

    // Create a WMS
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::WMS> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
            new ServiceServiceDownTestWMS(
                    this,
                    {compute_service}, {
                            storage_service1
                    }, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Running a "do nothing" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  ONE SLEEP ACTION SERVICE SUSPENDED TEST                         **/
/**********************************************************************/

class ServiceServiceSuspendedTestWMS : public wrench::WMS {
public:
    ServiceServiceSuspendedTestWMS(BareMetalComputeServiceOneActionTest *test,
                                   const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                   const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                   std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    BareMetalComputeServiceOneActionTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();


        // Create a compound job
        auto job = job_manager->createCompoundJob("my_job");
        auto action = job->addSleepAction("my_sleep", 10.0);

        // Suspend the compute service
        this->test->compute_service->suspend();

        // Submit the job
        try {
            job_manager->submitJob(job, this->test->compute_service, {});
            throw std::runtime_error("Shouldn't be able to submit a job to a suspended service");
        } catch (wrench::ExecutionException &e) {
            auto cause = std::dynamic_pointer_cast<wrench::ServiceIsSuspended>(e.getCause());
            if (not cause) {
                throw std::runtime_error("Unexpected failure cause");
            }
            if (cause->getService() != this->test->compute_service) {
                throw std::runtime_error("Unexpected service in failure cause");
            }
            // Check job state
            if (job->getState() != wrench::CompoundJob::State::NOT_SUBMITTED) {
                throw std::runtime_error("Unexpected job state: " + job->getStateAsString());
            }
        }

        return 0;
    }
};

TEST_F(BareMetalComputeServiceOneActionTest, ServiceSuspended) {
    DO_TEST_WITH_FORK(do_OneSleepActionServiceSuspended_test);
}

void BareMetalComputeServiceOneActionTest::do_OneSleepActionServiceSuspended_test() {
    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();

    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("one_action_test");
//    argv[1] = strdup("--wrench-host-shutdown-simulation");
//    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

    ASSERT_THROW(simulation->add((wrench::ComputeService *) nullptr), std::invalid_argument);

    // Create a Compute Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService("Host3",
                                                {std::make_pair("Host4",
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                {"/scratch"},
                                                {{wrench::BareMetalComputeServiceProperty::FAIL_ACTION_AFTER_ACTION_EXECUTOR_CRASH, "false"}}, {})));

    // Create a Storage Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService("Host2", {"/"})));

    // Create a WMS
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::WMS> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
            new ServiceServiceSuspendedTestWMS(
                    this,
                    {compute_service}, {
                            storage_service1
                    }, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Running a "do nothing" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  ONE SLEEP ACTION BAD SCRATCH TEST                               **/
/**********************************************************************/

class ServiceBadScratchTestWMS : public wrench::WMS {
public:
    ServiceBadScratchTestWMS(BareMetalComputeServiceOneActionTest *test,
                                   const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                   const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                   std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    BareMetalComputeServiceOneActionTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a compound job
        auto job = job_manager->createCompoundJob("my_job");
        auto action = job->addFileCopyAction("my_file_copy",this->test->input_file,
                                             wrench::FileLocation::LOCATION(this->test->storage_service1),
                                             wrench::FileLocation::SCRATCH);

        // Submit the job
        try {
            job_manager->submitJob(job, this->test->compute_service, {});
            throw std::runtime_error("Shouldn't be able to submit that job to a service that does not have scratch");
        } catch (std::invalid_argument &ignore) {
        }

        return 0;
    }
};

TEST_F(BareMetalComputeServiceOneActionTest, BadScratch) {
    DO_TEST_WITH_FORK(do_OneSleepActionBadScratch_test);
}

void BareMetalComputeServiceOneActionTest::do_OneSleepActionBadScratch_test() {
    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();

    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("one_action_test");
//    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

    ASSERT_THROW(simulation->add((wrench::ComputeService *) nullptr), std::invalid_argument);

    // Create a Compute Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService("Host3",
                                                {std::make_pair("Host4",
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                {""},
                                                {{wrench::BareMetalComputeServiceProperty::FAIL_ACTION_AFTER_ACTION_EXECUTOR_CRASH, "false"}}, {})));

    // Create a Storage Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService("Host2", {"/"})));

    // Create a WMS
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::WMS> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
            new ServiceBadScratchTestWMS(
                    this,
                    {compute_service}, {
                            storage_service1
                    }, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Running a "do nothing" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
