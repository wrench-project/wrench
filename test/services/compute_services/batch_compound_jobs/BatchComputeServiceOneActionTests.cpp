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

WRENCH_LOG_CATEGORY(batch_compute_service_one_action_test, "Log category for BatchComputeServiceOneAction test");

class BatchComputeServiceOneActionTest : public ::testing::Test {
public:
    std::shared_ptr<wrench::DataFile> input_file;
    std::shared_ptr<wrench::DataFile> output_file;
    std::shared_ptr<wrench::WorkflowTask> task;
    std::shared_ptr<wrench::StorageService> storage_service1 = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service2 = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service3 = nullptr;
    std::shared_ptr<wrench::BatchComputeService> compute_service = nullptr;

    void do_BadSetup_test();
    void do_OneSleepAction_test();
    void do_OneComputeActionNotEnoughResources_test();
    void do_OneComputeActionBogusServiceSpecificArgs_test();
    void do_OneSleepActionServiceCrashed_test();
    void do_OneSleepJobTermination_test();
    void do_OneSleepJobExpiration_test();
    void do_OneFileReadActionFileNotThere_test();
    void do_OneSleepActionServiceDown_test();
    void do_OneSleepActionServiceSuspended_test();
    void do_OneSleepActionBadScratch_test();

    std::shared_ptr<wrench::Workflow> workflow;

protected:
    ~BatchComputeServiceOneActionTest() {
        workflow->clear();
    }

    BatchComputeServiceOneActionTest() {

        // Create the simplest workflow
        workflow = wrench::Workflow::createWorkflow();

        // Create two files
        input_file = workflow->addFile("input_file", 10000.0);
        output_file = workflow->addFile("output_file", 20000.0);

        // Create a platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
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
};

/**********************************************************************/
/**  BAD SETUP SIMULATION TEST                                       **/
/**********************************************************************/

class BatchBadSetupTestWMS : public wrench::ExecutionController {
public:
    BatchBadSetupTestWMS(BatchComputeServiceOneActionTest *test,
                         std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BatchComputeServiceOneActionTest *test;

    int main() {

        return 0;
    }
};

TEST_F(BatchComputeServiceOneActionTest, BadSetup) {
    DO_TEST_WITH_FORK(do_BadSetup_test);
}

void BatchComputeServiceOneActionTest::do_BadSetup_test() {
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
    argv[0] = strdup("one_action_test");
    //    argv[1] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Empty resource list
    ASSERT_THROW(compute_service = simulation->add(
                         new wrench::BatchComputeService("Host1",
                                                         {},
                                                         {})),
                 std::invalid_argument);

    // Bad hostname
    ASSERT_THROW(compute_service = simulation->add(
                         new wrench::BatchComputeService("bogus",
                                                         {""},
                                                         {})),
                 std::invalid_argument);

    // Get a hostname
    std::string hostname = "Host1";

    // Bad resource hostname
    ASSERT_THROW(compute_service = simulation->add(
                         new wrench::BatchComputeService(hostname,
                                                         {"bogus"}, "",
                                                         {})),
                 std::invalid_argument);

    // Bad PROPERTIES
    ASSERT_THROW(compute_service = simulation->add(
                         new wrench::BatchComputeService(hostname,
                                                         {"RAMHost"},
                                                         "",
                                                         {std::make_pair(
                                                                 wrench::BatchComputeServiceProperty::TASK_STARTUP_OVERHEAD,
                                                                 "-1.0")},
                                                         {})),
                 std::invalid_argument);

    // Create a WMS
    auto wms = simulation->add(new BatchBadSetupTestWMS(this, hostname));

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  ONE SLEEP ACTION TEST                                           **/
/**********************************************************************/

class BatchOneSleepActionTestWMS : public wrench::ExecutionController {
public:
    BatchOneSleepActionTestWMS(BatchComputeServiceOneActionTest *test,
                               std::shared_ptr<wrench::BatchComputeService> batch_compute_service,
                               std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test), batch_compute_service(batch_compute_service) {
    }

private:
    BatchComputeServiceOneActionTest *test;
    std::shared_ptr<wrench::BatchComputeService> batch_compute_service;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a compound job and submit it
        auto job = job_manager->createCompoundJob("my_job");
        auto action = job->addSleepAction("my_sleep", 10.0);

        // No service-specific arguments
        try {
            job_manager->submitJob(job, this->test->compute_service, {});
            throw std::runtime_error("Should not be able to submit job with empty service-specific arguments");
        } catch (std::invalid_argument &ignore) {
        }

        // Missing service-specific arguments
        std::map<std::string, std::string> service_specific_args;
        service_specific_args["-N"] = "1";
        service_specific_args["-c"] = "1";
        try {
            job_manager->submitJob(job, this->test->compute_service, service_specific_args);
            throw std::runtime_error("Should not be able to submit job with missing service-specific arguments");
        } catch (std::invalid_argument &ignore) {
        }


        service_specific_args["-t"] = "60";
        job_manager->submitJob(job, this->test->compute_service, service_specific_args);


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
        if (real_event->compute_service != this->test->compute_service) {
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
        this->batch_compute_service->stop();

        return 0;
    }
};

TEST_F(BatchComputeServiceOneActionTest, OneSleepAction) {
    DO_TEST_WITH_FORK(do_OneSleepAction_test);
}

void BatchComputeServiceOneActionTest::do_OneSleepAction_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

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
                            new wrench::BatchComputeService("Host3",
                                                            {"Host4"},
                                                            {"/scratch"},
                                                            {})));

    // Create a Storage Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            new wrench::SimpleStorageService("Host2", {"/"})));

    // Create a WMS
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
                            new BatchOneSleepActionTestWMS(
                                    this, compute_service,
                                    hostname)));

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
/**  ONE COMPUTE ACTION NOT ENOUGH RESOURCES TEST                    **/
/**********************************************************************/

class BatchOneComputeActionNotEnoughResourcesTestWMS : public wrench::ExecutionController {
public:
    BatchOneComputeActionNotEnoughResourcesTestWMS(BatchComputeServiceOneActionTest *test,
                                                   std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BatchComputeServiceOneActionTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job that requires 1 node / core
        auto job1 = job_manager->createCompoundJob("my_job1");
        auto action1 = job1->addComputeAction("my_computation", 1000.0, 0.0, 1, 1,
                                              wrench::ParallelModel::AMDAHL(1.0));

        // Submit it but ask for too many resources
        std::map<std::string, std::string> service_specific_arguments;
        service_specific_arguments["-t"] = "60";
        service_specific_arguments["-N"] = "2";
        service_specific_arguments["-c"] = "1";
        try {
            job_manager->submitJob(job1, this->test->compute_service, service_specific_arguments);
            throw std::runtime_error("Shouldn't be able to submit a job that asks for too many resources");
        } catch (wrench::ExecutionException &e) {
            auto failure_cause = std::dynamic_pointer_cast<wrench::NotEnoughResources>(e.getCause());
            if (not failure_cause) {
                throw std::runtime_error("Unexpected failure cause " + e.getCause()->toString());
            }
        }

        // Submit it but ask for too many resources
        service_specific_arguments["-t"] = "60";
        service_specific_arguments["-N"] = "1";
        service_specific_arguments["-c"] = "100";
        try {
            job_manager->submitJob(job1, this->test->compute_service, service_specific_arguments);
            throw std::runtime_error("Shouldn't be able to submit a job that asks for too many resources");
        } catch (wrench::ExecutionException &e) {
            auto failure_cause = std::dynamic_pointer_cast<wrench::NotEnoughResources>(e.getCause());
            if (not failure_cause) {
                throw std::runtime_error("Unexpected failure cause " + e.getCause()->toString());
            }
        }

        // Create a job that requires 1 node / core, but with a an action that requires 2 cores
        auto job2 = job_manager->createCompoundJob("my_job2");
        auto action2 = job2->addComputeAction("my_computation_2cores", 1000.0, 0.0, 2, 2,
                                              wrench::ParallelModel::AMDAHL(1.0));

        // Submit it but ask for too many resources
        service_specific_arguments["-t"] = "60";
        service_specific_arguments["-N"] = "1";
        service_specific_arguments["-c"] = "1";
        try {
            job_manager->submitJob(job2, this->test->compute_service, service_specific_arguments);
            throw std::runtime_error("Shouldn't be able to submit a job that asks for too many resources");
        } catch (wrench::ExecutionException &e) {
            auto failure_cause = std::dynamic_pointer_cast<wrench::NotEnoughResources>(e.getCause());
            if (not failure_cause) {
                throw std::runtime_error("Unexpected failure cause " + e.getCause()->toString());
            }
        }

        // Create a job that requires 1 node / core, but with a an action that requires too much RAM
        auto job3 = job_manager->createCompoundJob("my_job3");
        auto action3 = job3->addComputeAction("my_computation_too_much_ram", 1000.0, 100000000.0, 1, 1,
                                              wrench::ParallelModel::AMDAHL(1.0));

        // Submit it but ask for too many resources
        service_specific_arguments["-t"] = "60";
        service_specific_arguments["-N"] = "1";
        service_specific_arguments["-c"] = "1";
        try {
            job_manager->submitJob(job3, this->test->compute_service, service_specific_arguments);
            throw std::runtime_error("Shouldn't be able to submit a job that asks for too many resources");
        } catch (wrench::ExecutionException &e) {
            auto failure_cause = std::dynamic_pointer_cast<wrench::NotEnoughResources>(e.getCause());
            if (not failure_cause) {
                throw std::runtime_error("Unexpected failure cause " + e.getCause()->toString());
            }
        }

        return 0;
    }
};

TEST_F(BatchComputeServiceOneActionTest, OneComputeActionNotEnoughResources) {
    DO_TEST_WITH_FORK(do_OneComputeActionNotEnoughResources_test);
}

void BatchComputeServiceOneActionTest::do_OneComputeActionNotEnoughResources_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

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
                            new wrench::BatchComputeService("Host3",
                                                            {"Host4"},
                                                            {"/scratch"},
                                                            {})));

    // Create a Storage Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            new wrench::SimpleStorageService("Host2", {"/"})));

    // Create a WMS
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
                            new BatchOneComputeActionNotEnoughResourcesTestWMS(
                                    this, hostname)));


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
/**  ONE COMPUTE ACTION BOGUS SERVICE-SPECIFIC ARGS TEST             **/
/**********************************************************************/

class BatchOneComputeActionBogusServiceSpecificArgsTestWMS : public wrench::ExecutionController {
public:
    BatchOneComputeActionBogusServiceSpecificArgsTestWMS(BatchComputeServiceOneActionTest *test,
                                                         std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BatchComputeServiceOneActionTest *test;

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

        for (auto const &args: bogus_args) {
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
        this->test->compute_service->stop();

        return 0;
    }
};

TEST_F(BatchComputeServiceOneActionTest, OneComputeActionBogusServiceSpecificArgs) {
    DO_TEST_WITH_FORK(do_OneComputeActionBogusServiceSpecificArgs_test);
}

void BatchComputeServiceOneActionTest::do_OneComputeActionBogusServiceSpecificArgs_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

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
                            new wrench::BatchComputeService("Host3",
                                                            {"Host4"},
                                                            {"/scratch"},
                                                            {})));

    // Create a Storage Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            new wrench::SimpleStorageService("Host2", {"/"})));

    // Create a WMS
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
                            new BatchOneComputeActionBogusServiceSpecificArgsTestWMS(
                                    this, hostname)));


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
/**  ONE COMPUTE ACTION SERVICE CRASHED TEST                         **/
/**********************************************************************/

class BatchServiceCrashedTestWMS : public wrench::ExecutionController {
public:
    BatchServiceCrashedTestWMS(BatchComputeServiceOneActionTest *test,
                               std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BatchComputeServiceOneActionTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a compound job and submit it
        auto job = job_manager->createCompoundJob("my_job");
        auto action = job->addSleepAction("my_sleep", 10.0);

        std::map<std::string, std::string> service_specific_arguments;
        service_specific_arguments["-t"] = "60";
        service_specific_arguments["-N"] = "1";
        service_specific_arguments["-c"] = "3";
        job_manager->submitJob(job, this->test->compute_service, service_specific_arguments);

        // Sleep 1 sec
        wrench::Simulation::sleep(1.0);

        // Kill the BatchComputeService
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
        if (real_event->compute_service != this->test->compute_service) {
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
        this->test->compute_service->stop();

        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchComputeServiceOneActionTest, ServiceCrashed) {
#else
TEST_F(BatchComputeServiceOneActionTest, ServiceCrashed) {
#endif
    DO_TEST_WITH_FORK(do_OneSleepActionServiceCrashed_test);
}

void BatchComputeServiceOneActionTest::do_OneSleepActionServiceCrashed_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

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
                            new wrench::BatchComputeService("Host3",
                                                            {"Host4"},
                                                            {"/scratch"},
                                                            {}, {})));

    // Create a Storage Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            new wrench::SimpleStorageService("Host2", {"/"})));

    // Create a WMS
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
                            new BatchServiceCrashedTestWMS(
                                    this, hostname)));


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
/**  ONE COMPUTE ACTION TERMINATION TEST                             **/
/**********************************************************************/

class BatchJobTerminationTestWMS : public wrench::ExecutionController {
public:
    BatchJobTerminationTestWMS(BatchComputeServiceOneActionTest *test,
                               std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BatchComputeServiceOneActionTest *test;

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

        std::map<std::string, std::string> service_specific_arguments;
        service_specific_arguments["-t"] = "60";
        service_specific_arguments["-N"] = "1";
        service_specific_arguments["-c"] = "1";

        job_manager->submitJob(job, this->test->compute_service, service_specific_arguments);

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

TEST_F(BatchComputeServiceOneActionTest, JobTermination) {
    DO_TEST_WITH_FORK(do_OneSleepJobTermination_test);
}

void BatchComputeServiceOneActionTest::do_OneSleepJobTermination_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

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
                            new wrench::BatchComputeService("Host3",
                                                            {"Host4"},
                                                            {"/scratch"},
                                                            {}, {})));

    // Create a Storage Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            new wrench::SimpleStorageService("Host2", {"/"})));

    // Create a WMS
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
                            new BatchJobTerminationTestWMS(
                                    this, hostname)));


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
/**  ONE COMPUTE ACTION EXPIRATION TEST                             **/
/**********************************************************************/

class BatchJobExpirationTestWMS : public wrench::ExecutionController {
public:
    BatchJobExpirationTestWMS(BatchComputeServiceOneActionTest *test,
                              std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BatchComputeServiceOneActionTest *test;

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
        auto action = job->addSleepAction("my_sleep", 100.0);

        std::map<std::string, std::string> service_specific_arguments;
        service_specific_arguments["-t"] = "1";
        service_specific_arguments["-N"] = "1";
        service_specific_arguments["-c"] = "1";

        job_manager->submitJob(job, this->test->compute_service, service_specific_arguments);

        // Check job state
        if (job->getState() != wrench::CompoundJob::State::SUBMITTED) {
            throw std::runtime_error("Unexpected job state: " + job->getStateAsString());
        }

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
        if (real_event->compute_service != this->test->compute_service) {
            throw std::runtime_error("Event's compute service isn't the right compute service!");
        }

        // Check job state
        if (job->getState() != wrench::CompoundJob::State::DISCONTINUED) {
            throw std::runtime_error("Unexpected job state: " + job->getStateAsString());
        }

        // Chek action stuff
        if (action->getState() != wrench::Action::State::KILLED) {
            throw std::runtime_error("Unexpected action state " + action->getStateAsString());
        }
        if (not std::dynamic_pointer_cast<wrench::JobTimeout>(action->getFailureCause())) {
            throw std::runtime_error("Unexpected action failure cause " + action->getFailureCause()->toString());
        }

        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchComputeServiceOneActionTest, JobExpiration) {
#else
TEST_F(BatchComputeServiceOneActionTest, JobExpiration) {
#endif
    DO_TEST_WITH_FORK(do_OneSleepJobExpiration_test);
}

void BatchComputeServiceOneActionTest::do_OneSleepJobExpiration_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

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
                            new wrench::BatchComputeService("Host3",
                                                            {"Host4"},
                                                            {"/scratch"},
                                                            {}, {})));

    // Create a Storage Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            new wrench::SimpleStorageService("Host2", {"/"})));

    // Create a WMS
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
                            new BatchJobExpirationTestWMS(
                                    this, hostname)));

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
/**  ONE FILE READ ACTION SERVICE FILE NOT THERE TEST               **/
/**********************************************************************/

class BatchServiceFileNotThereTestWMS : public wrench::ExecutionController {
public:
    BatchServiceFileNotThereTestWMS(BatchComputeServiceOneActionTest *test,
                                    std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BatchComputeServiceOneActionTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a compound job and submit it
        auto job = job_manager->createCompoundJob("my_job");
        auto action = job->addFileReadAction("my_file_read", this->test->input_file,
                                             wrench::FileLocation::LOCATION(this->test->storage_service1));

        std::map<std::string, std::string> service_specific_args;
        service_specific_args["-t"] = "60";
        service_specific_args["-N"] = "1";
        service_specific_args["-c"] = "1";

        job_manager->submitJob(job, this->test->compute_service, service_specific_args);

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
        if (real_event->compute_service != this->test->compute_service) {
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
        this->test->compute_service->stop();

        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchComputeServiceOneActionTest, FileNotThere) {
#else
TEST_F(BatchComputeServiceOneActionTest, FileNotThere) {
#endif
    DO_TEST_WITH_FORK(do_OneFileReadActionFileNotThere_test);
}

void BatchComputeServiceOneActionTest::do_OneFileReadActionFileNotThere_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

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
                            new wrench::BatchComputeService("Host3",
                                                            {"Host4"},
                                                            {"/scratch"},
                                                            {}, {})));

    // Create a Storage Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            new wrench::SimpleStorageService("Host2", {"/"})));

    // Create a WMS
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
                            new BatchServiceFileNotThereTestWMS(
                                    this, hostname)));

    // Running a "do nothing" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  ONE SLEEP ACTION SERVICE DOWN TEST                              **/
/**********************************************************************/

class BatchServiceServiceDownTestWMS : public wrench::ExecutionController {
public:
    BatchServiceServiceDownTestWMS(BatchComputeServiceOneActionTest *test,
                                   std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BatchComputeServiceOneActionTest *test;

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

TEST_F(BatchComputeServiceOneActionTest, ServiceDown) {
    DO_TEST_WITH_FORK(do_OneSleepActionServiceDown_test);
}

void BatchComputeServiceOneActionTest::do_OneSleepActionServiceDown_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

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
                            new wrench::BatchComputeService("Host3",
                                                            {"Host4"},
                                                            {"/scratch"},
                                                            {}, {})));

    // Create a Storage Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            new wrench::SimpleStorageService("Host2", {"/"})));

    // Create a WMS
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
                            new BatchServiceServiceDownTestWMS(
                                    this, hostname)));

    // Running a "do nothing" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  ONE SLEEP ACTION SERVICE SUSPENDED TEST                         **/
/**********************************************************************/

class BatchServiceServiceSuspendedTestWMS : public wrench::ExecutionController {
public:
    BatchServiceServiceSuspendedTestWMS(BatchComputeServiceOneActionTest *test,
                                        std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BatchComputeServiceOneActionTest *test;

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

TEST_F(BatchComputeServiceOneActionTest, ServiceSuspended) {
    DO_TEST_WITH_FORK(do_OneSleepActionServiceSuspended_test);
}

void BatchComputeServiceOneActionTest::do_OneSleepActionServiceSuspended_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

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
                            new wrench::BatchComputeService("Host3",
                                                            {"Host4"},
                                                            {"/scratch"},
                                                            {}, {})));

    // Create a Storage Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            new wrench::SimpleStorageService("Host2", {"/"})));

    // Create a WMS
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
                            new BatchServiceServiceSuspendedTestWMS(
                                    this, hostname)));

    // Running a "do nothing" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  ONE SLEEP ACTION BAD SCRATCH TEST                               **/
/**********************************************************************/

class BatchServiceBadScratchTestWMS : public wrench::ExecutionController {
public:
    BatchServiceBadScratchTestWMS(BatchComputeServiceOneActionTest *test,
                                  std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BatchComputeServiceOneActionTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a compound job
        auto job = job_manager->createCompoundJob("my_job");
        auto action = job->addFileCopyAction("my_file_copy", this->test->input_file,
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

TEST_F(BatchComputeServiceOneActionTest, BadScratch) {
    DO_TEST_WITH_FORK(do_OneSleepActionBadScratch_test);
}

void BatchComputeServiceOneActionTest::do_OneSleepActionBadScratch_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

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
                            new wrench::BatchComputeService("Host3",
                                                            {"Host4"},
                                                            {""},
                                                            {}, {})));

    // Create a Storage Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            new wrench::SimpleStorageService("Host2", {"/"})));

    // Create a WMS
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
                            new BatchServiceBadScratchTestWMS(
                                    this, hostname)));

    // Running a "do nothing" simulation
    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
