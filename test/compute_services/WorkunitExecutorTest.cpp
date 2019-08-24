/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>
#include <random>
#include <wrench-dev.h>

#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/services/compute/workunit_executor/Workunit.h"
#include "wrench/services/compute/workunit_executor/WorkunitExecutor.h"

#include "../../src/wrench/services/compute/work_unit_executor/ComputeThread.h"


#include "../include/TestWithFork.h"
#include "../include/UniqueTmpPathPrefix.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(workunit_executor_test, "Log category for Simple WorkunitExecutorTest");

#define EPSILON 0.05

class WorkunitExecutorTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::StorageService> storage_service1 = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service2 = nullptr;
    wrench::Simulation *simulation;


    void do_WorkunitExecutorConstructor_test();
    void do_WorkunitExecutorBadScratchSpace_test();


protected:
    WorkunitExecutorTest() {

        // Create the simplest workflow
        workflow = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());

        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"10\"/> "
                          "       <host id=\"Host2\" speed=\"1f\" core=\"10\"/> "
                          "       <host id=\"Host3\" speed=\"1f\" core=\"10\"/> "
                          "       <host id=\"Host4\" speed=\"1f\" core=\"10\">  "
                          "         <prop id=\"ram\" value=\"1024\"/> "
                          "       </host>  "
                          "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                          "       <link id=\"2\" bandwidth=\"0.1MBps\" latency=\"10us\"/>"
                          "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host3\" dst=\"Host4\"> <link_ctn id=\"2\"/> </route>"
                          "       <route src=\"Host1\" dst=\"Host4\"> <link_ctn id=\"2\"/> </route>"
                          "   </zone> "
                          "</platform>";
        // Create a four-host 10-core platform file
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);

    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
    std::unique_ptr<wrench::Workflow> workflow;

};


/**********************************************************************/
/**  DO CONSTRUCTOR TEST                                             **/
/**********************************************************************/

class WorkunitExecutorConstructorTestWMS : public wrench::WMS {

public:
    WorkunitExecutorConstructorTestWMS(WorkunitExecutorTest *test,
                                       const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                       const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                       std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }


private:

    WorkunitExecutorTest *test;

    int main() {
        auto input_file = this->getWorkflow()->getFileByID("input_file");
        auto output_file = this->getWorkflow()->getFileByID("output_file");

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create some  standard job
        auto task = this->getWorkflow()->addTask("task", 3600, 1, 1, 1.0, 0);
        task->addInputFile(input_file);
        task->addOutputFile(output_file);
        auto job = job_manager->createStandardJob(
                {task},
                {},
                {},
                {},
                {});


        std::set<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::StorageService>, std::shared_ptr<wrench::StorageService>>> pre_file_copies;
        std::map<wrench::WorkflowFile *, std::shared_ptr<wrench::StorageService>> file_locations;
        std::set<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::StorageService>, std::shared_ptr<wrench::StorageService> >> post_file_copies;
        std::set<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::StorageService> >> cleanup_file_deletions;

        pre_file_copies.insert(std::make_tuple(input_file, this->test->storage_service1, this->test->storage_service2));
        file_locations[input_file] = this->test->storage_service2;
        post_file_copies.insert(std::make_tuple(output_file, this->test->storage_service2, this->test->storage_service1));
        cleanup_file_deletions.insert(std::make_tuple(input_file, this->test->storage_service2));

        auto wu = std::shared_ptr<wrench::Workunit>(new wrench::Workunit(
                job,
                pre_file_copies,
                task,
                file_locations,
                post_file_copies,
                cleanup_file_deletions
        ));


        try {
            auto wue = new wrench::WorkunitExecutor(this->hostname, 0, 1, this->mailbox_name,
                                                    wu, nullptr, job, 0, true);
            throw std::runtime_error("Shouldn't be able to create a WorkunitExecutor with <=0 num cores");
        } catch (std::invalid_argument &e) {}

        try {
            auto wue = new wrench::WorkunitExecutor(this->hostname, 1, -1, this->mailbox_name,
                                                    wu, nullptr, job, 0, true);
            throw std::runtime_error("Shouldn't be able to create a WorkunitExecutor with <0 ram");
        } catch (std::invalid_argument &e) {}

        try {
            auto wue = new wrench::WorkunitExecutor(this->hostname, 1, 1, this->mailbox_name,
                                                    nullptr, nullptr, job, 0, true);
            throw std::runtime_error("Shouldn't be able to create a WorkunitExecutor with nullptr Workunit");
        } catch (std::invalid_argument &e) {}

        try {
            auto wue = new wrench::WorkunitExecutor(this->hostname, 1, 1, this->mailbox_name,
                                                    wu, nullptr, job, -1, true);
            throw std::runtime_error("Shouldn't be able to create a WorkunitExecutor with <0 thread startup overhead");
        } catch (std::invalid_argument &e) {}


        return 0;
    }
};

TEST_F(WorkunitExecutorTest, ConstructorTest) {
    DO_TEST_WITH_FORK(do_WorkunitExecutorConstructor_test);
}

void WorkunitExecutorTest::do_WorkunitExecutorConstructor_test() {

    // Create and initialize a simulation
    simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = simulation->getHostnameList()[0];

    // Create a Compute Service
    std::shared_ptr<wrench::ComputeService> compute_service;
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair(hostname, std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                                                {})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService(hostname, 10000000000000.0)));

    // Create another Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
            new wrench::SimpleStorageService(hostname, 10000000000000.0)));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new WorkunitExecutorConstructorTestWMS(
                    this,  {compute_service}, {storage_service1, storage_service2}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow.get()));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create two workflow files
    wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000000.0);
    wrench::WorkflowFile *output_file = this->workflow->addFile("output_file", 20000.0);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "run a single task" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}



/**********************************************************************/
/**  BAD SCRATCH TEST                                                **/
/**********************************************************************/

class WorkunitExecutorBadScratchSpaceTestWMS : public wrench::WMS {

public:
    WorkunitExecutorBadScratchSpaceTestWMS(WorkunitExecutorTest *test,
                                           const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                           const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                           std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }


private:

    WorkunitExecutorTest *test;

    int main() {
        auto input_file = this->getWorkflow()->getFileByID("input_file");
        auto output_file = this->getWorkflow()->getFileByID("output_file");

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create some  standard job
        auto task = this->getWorkflow()->addTask("task", 3600, 1, 1, 1.0, 0);
        task->addInputFile(input_file);
        task->addOutputFile(output_file);
        auto job = job_manager->createStandardJob(
                {task},
                {},
                {},
                {},
                {});

        std::set<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::StorageService>, std::shared_ptr<wrench::StorageService>>> pre_file_copies;
        std::map<wrench::WorkflowFile *, std::shared_ptr<wrench::StorageService>> file_locations;
        std::set<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::StorageService>, std::shared_ptr<wrench::StorageService> >> post_file_copies;
        std::set<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::StorageService> >> cleanup_file_deletions;

        /** BOGUS PRE **/
        {
            pre_file_copies.insert(
                    std::make_tuple(input_file, this->test->storage_service1, wrench::ComputeService::SCRATCH));
            file_locations[input_file] = this->test->storage_service2;
            file_locations[output_file] = this->test->storage_service2;
            post_file_copies.insert(
                    std::make_tuple(output_file, this->test->storage_service2, this->test->storage_service1));
            cleanup_file_deletions.insert(std::make_tuple(input_file, this->test->storage_service2));

            auto wu = std::shared_ptr<wrench::Workunit>(new wrench::Workunit(
                    job,
                    pre_file_copies,
                    task,
                    file_locations,
                    post_file_copies,
                    cleanup_file_deletions
            ));

            auto wue = std::shared_ptr<wrench::WorkunitExecutor>(
                    new wrench::WorkunitExecutor(this->hostname, 1, 1, this->mailbox_name,
                                                 wu, nullptr, job, 1, true));
            wue->simulation = this->simulation;
            wue->start(wue, true, false);

            auto msg = wrench::S4U_Mailbox::getMessage(this->mailbox_name);
            if (not std::dynamic_pointer_cast<wrench::WorkunitExecutorFailedMessage>(msg)) {
                throw std::runtime_error("Was expecting a WorkunitExecutorFailedMessage message!");
            } else {
                auto real_msg = std::dynamic_pointer_cast<wrench::WorkunitExecutorFailedMessage>(msg);
                if (not std::dynamic_pointer_cast<wrench::NoScratchSpace>(real_msg->cause)) {
                    throw std::runtime_error(
                            "Got the expected WorkunitExecutorFailedMessage message but not the expected NoScratchSpace failure cause");
                }
            }
        }

        /** BOGUS MAP **/
        {
            pre_file_copies.insert(
                    std::make_tuple(input_file, this->test->storage_service1, this->test->storage_service2));
            file_locations[input_file] = this->test->storage_service2;
            file_locations[output_file] = wrench::ComputeService::SCRATCH;
            post_file_copies.insert(
                    std::make_tuple(output_file, this->test->storage_service2, this->test->storage_service1));
            cleanup_file_deletions.insert(std::make_tuple(input_file, this->test->storage_service2));

            auto wu = std::shared_ptr<wrench::Workunit>(new wrench::Workunit(
                    job,
                    pre_file_copies,
                    task,
                    file_locations,
                    post_file_copies,
                    cleanup_file_deletions
            ));

            auto wue = std::shared_ptr<wrench::WorkunitExecutor>(
                    new wrench::WorkunitExecutor(this->hostname, 1, 1, this->mailbox_name,
                                                 wu, nullptr, job, 1, true));
            wue->simulation = this->simulation;
            wue->start(wue, true, false);

            auto msg = wrench::S4U_Mailbox::getMessage(this->mailbox_name);
            if (not std::dynamic_pointer_cast<wrench::WorkunitExecutorFailedMessage>(msg)) {
                throw std::runtime_error("Was expecting a WorkunitExecutorFailedMessage message!");
            } else {
                auto real_msg = std::dynamic_pointer_cast<wrench::WorkunitExecutorFailedMessage>(msg);
                if (not std::dynamic_pointer_cast<wrench::NoScratchSpace>(real_msg->cause)) {
                    throw std::runtime_error(
                            "Got the expected WorkunitExecutorFailedMessage message but not the expected NoScratchSpace failure cause");
                }
            }
        }

        /** BOGUS POST **/
        {
            pre_file_copies.insert(
                    std::make_tuple(input_file, this->test->storage_service1, this->test->storage_service2));
            file_locations[input_file] = this->test->storage_service2;
            file_locations[output_file] = this->test->storage_service2;
            post_file_copies.insert(
                    std::make_tuple(output_file, wrench::ComputeService::SCRATCH, this->test->storage_service1));
            cleanup_file_deletions.insert(std::make_tuple(input_file, this->test->storage_service2));

            auto wu = std::shared_ptr<wrench::Workunit>(new wrench::Workunit(
                    job,
                    pre_file_copies,
                    task,
                    file_locations,
                    post_file_copies,
                    cleanup_file_deletions
            ));

            auto wue = std::shared_ptr<wrench::WorkunitExecutor>(
                    new wrench::WorkunitExecutor(this->hostname, 1, 1, this->mailbox_name,
                                                 wu, nullptr, job, 1, true));
            wue->simulation = this->simulation;
            wue->start(wue, true, false);

            auto msg = wrench::S4U_Mailbox::getMessage(this->mailbox_name);
            if (not std::dynamic_pointer_cast<wrench::WorkunitExecutorFailedMessage>(msg)) {
                throw std::runtime_error("Was expecting a WorkunitExecutorFailedMessage message!");
            } else {
                auto real_msg = std::dynamic_pointer_cast<wrench::WorkunitExecutorFailedMessage>(msg);
                if (not std::dynamic_pointer_cast<wrench::NoScratchSpace>(real_msg->cause)) {
                    throw std::runtime_error(
                            "Got the expected WorkunitExecutorFailedMessage message but not the expected NoScratchSpace failure cause");
                }
            }
        }

        /** BOGUS CLEANUP **/
        {
            pre_file_copies.insert(
                    std::make_tuple(input_file, this->test->storage_service1, this->test->storage_service2));
            file_locations[input_file] = this->test->storage_service2;
            file_locations[output_file] = this->test->storage_service2;
            post_file_copies.insert(
                    std::make_tuple(output_file, this->test->storage_service2, this->test->storage_service1));
            cleanup_file_deletions.insert(std::make_tuple(input_file, wrench::ComputeService::SCRATCH));

            auto wu = std::shared_ptr<wrench::Workunit>(new wrench::Workunit(
                    job,
                    pre_file_copies,
                    task,
                    file_locations,
                    post_file_copies,
                    cleanup_file_deletions
            ));

            auto wue = std::shared_ptr<wrench::WorkunitExecutor>(
                    new wrench::WorkunitExecutor(this->hostname, 1, 1, this->mailbox_name,
                                                 wu, nullptr, job, 1, true));
            wue->simulation = this->simulation;
            wue->start(wue, true, false);

            auto msg = wrench::S4U_Mailbox::getMessage(this->mailbox_name);
            if (not std::dynamic_pointer_cast<wrench::WorkunitExecutorFailedMessage>(msg)) {
                throw std::runtime_error("Was expecting a WorkunitExecutorFailedMessage message!");
            } else {
                auto real_msg = std::dynamic_pointer_cast<wrench::WorkunitExecutorFailedMessage>(msg);
                if (not std::dynamic_pointer_cast<wrench::NoScratchSpace>(real_msg->cause)) {
                    throw std::runtime_error(
                            "Got the expected WorkunitExecutorFailedMessage message but not the expected NoScratchSpace failure cause");
                }
            }
        }


        return 0;
    }
};

TEST_F(WorkunitExecutorTest, BadScratchSpaceTest) {
    DO_TEST_WITH_FORK(do_WorkunitExecutorBadScratchSpace_test);
}

void WorkunitExecutorTest::do_WorkunitExecutorBadScratchSpace_test() {

    // Create and initialize a simulation
    simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = simulation->getHostnameList()[0];

    // Create a Compute Service
    std::shared_ptr<wrench::ComputeService> compute_service;
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair(hostname, std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                                                {})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService(hostname, 10000000000000.0)));

    // Create another Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
            new wrench::SimpleStorageService(hostname, 10000000000000.0)));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new WorkunitExecutorBadScratchSpaceTestWMS(
                    this,  {compute_service}, {storage_service1, storage_service2}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow.get()));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create two workflow files
    wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000000.0);
    wrench::WorkflowFile *output_file = this->workflow->addFile("output_file", 20000.0);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "run a single task" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}



