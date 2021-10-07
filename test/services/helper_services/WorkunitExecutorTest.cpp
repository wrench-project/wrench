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

#include "helper_services/standard_job_executor/StandardJobExecutorMessage.h"


#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(workunit_executor_test, "Log category for Simple WorkunitExecutorTest");

#define EPSILON 0.05

class WorkunitExecutorTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::StorageService> storage_service1 = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service2 = nullptr;
    wrench::Simulation *simulation;


    void do_WorkunitConstructor_test();
    void do_WorkunitExecutorConstructor_test();


protected:
    WorkunitExecutorTest() {

        // Create the simplest workflow
        workflow = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());

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
/**  DO WORKUNIT CONSTRUCTOR TEST                                    **/
/**********************************************************************/


TEST_F(WorkunitExecutorTest, WorkunitConstructorTest) {
    DO_TEST_WITH_FORK(do_WorkunitConstructor_test);
}

void WorkunitExecutorTest::do_WorkunitConstructor_test() {

    // Create and initialize a simulation
    simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/disk1/"})));

    // Create another Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/disk2/"})));

    // Create workflow filess
    wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000000.0);
    wrench::WorkflowFile *output_file = this->workflow->addFile("output_file", 10000000.0);

    // Create a task
    auto task = this->workflow->addTask("task", 1.0, 1, 1, 0.0);

    {
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>> pre_file_copies;
        std::map<wrench::WorkflowFile *, std::vector<std::shared_ptr<wrench::FileLocation>>> file_locations;
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation> >> post_file_copies;
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation> >> cleanup_file_deletions;

        pre_file_copies.push_back(std::make_tuple(input_file, wrench::FileLocation::LOCATION(this->storage_service1), wrench::FileLocation::LOCATION(this->storage_service2)));
        file_locations[input_file] = {};
        file_locations[input_file].push_back(wrench::FileLocation::LOCATION(this->storage_service2));
        post_file_copies.push_back(std::make_tuple(output_file, wrench::FileLocation::LOCATION(this->storage_service2), wrench::FileLocation::LOCATION(this->storage_service1)));
        cleanup_file_deletions.push_back(std::make_tuple(input_file, wrench::FileLocation::LOCATION(this->storage_service2)));

        ASSERT_NO_THROW(new wrench::Workunit(std::shared_ptr<wrench::StandardJob>((wrench::StandardJob*)666),
                                             0.0,
                                             pre_file_copies, task, file_locations,
                                             post_file_copies, cleanup_file_deletions));
    }
    {
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>> pre_file_copies;
        std::map<wrench::WorkflowFile *, std::vector<std::shared_ptr<wrench::FileLocation>>> file_locations;
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation> >> post_file_copies;
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation> >> cleanup_file_deletions;

        pre_file_copies.push_back(std::make_tuple(nullptr, wrench::FileLocation::LOCATION(this->storage_service1), wrench::FileLocation::LOCATION(this->storage_service2)));
        file_locations[input_file] = {};
        file_locations[input_file].push_back(wrench::FileLocation::LOCATION(this->storage_service2));
        post_file_copies.push_back(std::make_tuple(output_file, wrench::FileLocation::LOCATION(this->storage_service2), wrench::FileLocation::LOCATION(this->storage_service1)));
        cleanup_file_deletions.push_back(std::make_tuple(input_file, wrench::FileLocation::LOCATION(this->storage_service2)));

        ASSERT_THROW(new wrench::Workunit(std::shared_ptr<wrench::StandardJob>((wrench::StandardJob *)666, [](void *ptr){}),
                                          0.0,
                                          pre_file_copies, task, file_locations,
                                          post_file_copies, cleanup_file_deletions), std::invalid_argument);
    }
    {
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>> pre_file_copies;
        std::map<wrench::WorkflowFile *, std::vector<std::shared_ptr<wrench::FileLocation>>> file_locations;
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation> >> post_file_copies;
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation> >> cleanup_file_deletions;

        pre_file_copies.push_back(std::make_tuple(input_file, nullptr, wrench::FileLocation::LOCATION(this->storage_service2)));
        file_locations[input_file] = {};
        file_locations[input_file].push_back(wrench::FileLocation::LOCATION(this->storage_service2));
        post_file_copies.push_back(std::make_tuple(output_file, wrench::FileLocation::LOCATION(this->storage_service2), wrench::FileLocation::LOCATION(this->storage_service1)));
        cleanup_file_deletions.push_back(std::make_tuple(input_file, wrench::FileLocation::LOCATION(this->storage_service2)));

        ASSERT_THROW(new wrench::Workunit(std::shared_ptr<wrench::StandardJob>((wrench::StandardJob *)666, [](void *ptr){}),
                                          0.0,
                                          pre_file_copies, task, file_locations,
                                          post_file_copies, cleanup_file_deletions), std::invalid_argument);
    }
    {
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>> pre_file_copies;
        std::map<wrench::WorkflowFile *, std::vector<std::shared_ptr<wrench::FileLocation>>> file_locations;
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation> >> post_file_copies;
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation> >> cleanup_file_deletions;

        pre_file_copies.push_back(std::make_tuple(input_file, wrench::FileLocation::LOCATION(this->storage_service1), nullptr));
        file_locations[input_file] = {};
        file_locations[input_file].push_back(wrench::FileLocation::LOCATION(this->storage_service2));
        post_file_copies.push_back(std::make_tuple(output_file, wrench::FileLocation::LOCATION(this->storage_service2), wrench::FileLocation::LOCATION(this->storage_service1)));
        cleanup_file_deletions.push_back(std::make_tuple(input_file, wrench::FileLocation::LOCATION(this->storage_service2)));

        ASSERT_THROW(new wrench::Workunit(std::shared_ptr<wrench::StandardJob>((wrench::StandardJob *)666, [](void *ptr){}),
                                          0.0,
                                          pre_file_copies, task, file_locations,
                                          post_file_copies, cleanup_file_deletions), std::invalid_argument);
    }
    {
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>> pre_file_copies;
        std::map<wrench::WorkflowFile *, std::vector<std::shared_ptr<wrench::FileLocation>>> file_locations;
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation> >> post_file_copies;
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation> >> cleanup_file_deletions;

        pre_file_copies.push_back(std::make_tuple(input_file, wrench::FileLocation::LOCATION(this->storage_service1), wrench::FileLocation::LOCATION(this->storage_service2)));
        file_locations[input_file] = {};
        post_file_copies.push_back(std::make_tuple(output_file, wrench::FileLocation::LOCATION(this->storage_service2), wrench::FileLocation::LOCATION(this->storage_service1)));
        cleanup_file_deletions.push_back(std::make_tuple(input_file, wrench::FileLocation::LOCATION(this->storage_service2)));

        ASSERT_THROW(new wrench::Workunit(std::shared_ptr<wrench::StandardJob>((wrench::StandardJob *)666, [](void *ptr){}),
                                          0.0,
                                          pre_file_copies, task, file_locations,
                                          post_file_copies, cleanup_file_deletions), std::invalid_argument);
    }
    {
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>> pre_file_copies;
        std::map<wrench::WorkflowFile *, std::vector<std::shared_ptr<wrench::FileLocation>>> file_locations;
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation> >> post_file_copies;
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation> >> cleanup_file_deletions;

        pre_file_copies.push_back(std::make_tuple(input_file, wrench::FileLocation::LOCATION(this->storage_service1), wrench::FileLocation::LOCATION(this->storage_service2)));
        file_locations[input_file] = {};
        file_locations[input_file].push_back(nullptr);
        post_file_copies.push_back(std::make_tuple(output_file, wrench::FileLocation::LOCATION(this->storage_service2), wrench::FileLocation::LOCATION(this->storage_service1)));
        cleanup_file_deletions.push_back(std::make_tuple(input_file, wrench::FileLocation::LOCATION(this->storage_service2)));

        ASSERT_THROW(new wrench::Workunit(std::shared_ptr<wrench::StandardJob>((wrench::StandardJob *)666, [](void *ptr){}),
                                          0.0,
                                          pre_file_copies, task, file_locations,
                                          post_file_copies, cleanup_file_deletions), std::invalid_argument);
    }
    {
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>> pre_file_copies;
        std::map<wrench::WorkflowFile *, std::vector<std::shared_ptr<wrench::FileLocation>>> file_locations;
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation> >> post_file_copies;
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation> >> cleanup_file_deletions;

        pre_file_copies.push_back(std::make_tuple(input_file, wrench::FileLocation::LOCATION(this->storage_service1), wrench::FileLocation::LOCATION(this->storage_service2)));
        file_locations[input_file] = {};
        file_locations[input_file].push_back(wrench::FileLocation::LOCATION(this->storage_service2));
        post_file_copies.push_back(std::make_tuple(nullptr, wrench::FileLocation::LOCATION(this->storage_service2), wrench::FileLocation::LOCATION(this->storage_service1)));
        cleanup_file_deletions.push_back(std::make_tuple(input_file, wrench::FileLocation::LOCATION(this->storage_service2)));

        ASSERT_THROW(new wrench::Workunit(std::shared_ptr<wrench::StandardJob>((wrench::StandardJob *)666, [](void *ptr){}),
                                          0.0,
                                          pre_file_copies, task, file_locations,
                                          post_file_copies, cleanup_file_deletions), std::invalid_argument);
    }
    {
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>> pre_file_copies;
        std::map<wrench::WorkflowFile *, std::vector<std::shared_ptr<wrench::FileLocation>>> file_locations;
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation> >> post_file_copies;
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation> >> cleanup_file_deletions;

        pre_file_copies.push_back(std::make_tuple(input_file, wrench::FileLocation::LOCATION(this->storage_service1), wrench::FileLocation::LOCATION(this->storage_service2)));
        file_locations[input_file] = {};
        file_locations[input_file].push_back(wrench::FileLocation::LOCATION(this->storage_service2));
        post_file_copies.push_back(std::make_tuple(output_file, nullptr, wrench::FileLocation::LOCATION(this->storage_service1)));
        cleanup_file_deletions.push_back(std::make_tuple(input_file, wrench::FileLocation::LOCATION(this->storage_service2)));

        ASSERT_THROW(new wrench::Workunit(std::shared_ptr<wrench::StandardJob>((wrench::StandardJob *)666, [](void *ptr){}),
                                          0.0,
                                          pre_file_copies, task, file_locations,
                                          post_file_copies, cleanup_file_deletions), std::invalid_argument);
    }
    {
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>> pre_file_copies;
        std::map<wrench::WorkflowFile *, std::vector<std::shared_ptr<wrench::FileLocation>>> file_locations;
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation> >> post_file_copies;
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation> >> cleanup_file_deletions;

        pre_file_copies.push_back(std::make_tuple(input_file, wrench::FileLocation::LOCATION(this->storage_service1), wrench::FileLocation::LOCATION(this->storage_service2)));
        file_locations[input_file] = {};
        file_locations[input_file].push_back(wrench::FileLocation::LOCATION(this->storage_service2));
        post_file_copies.push_back(std::make_tuple(output_file, wrench::FileLocation::LOCATION(this->storage_service2), nullptr));
        cleanup_file_deletions.push_back(std::make_tuple(input_file, wrench::FileLocation::LOCATION(this->storage_service2)));

        ASSERT_THROW(new wrench::Workunit(std::shared_ptr<wrench::StandardJob>((wrench::StandardJob *)666, [](void *ptr){}),
                                          0.0,
                                          pre_file_copies, task, file_locations,
                                          post_file_copies, cleanup_file_deletions), std::invalid_argument);
    }
    {
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>> pre_file_copies;
        std::map<wrench::WorkflowFile *, std::vector<std::shared_ptr<wrench::FileLocation>>> file_locations;
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation> >> post_file_copies;
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation> >> cleanup_file_deletions;

        pre_file_copies.push_back(std::make_tuple(input_file, wrench::FileLocation::LOCATION(this->storage_service1), wrench::FileLocation::LOCATION(this->storage_service2)));
        file_locations[input_file] = {};
        file_locations[input_file].push_back(wrench::FileLocation::LOCATION(this->storage_service2));
        post_file_copies.push_back(std::make_tuple(output_file, wrench::FileLocation::LOCATION(this->storage_service2), wrench::FileLocation::LOCATION(this->storage_service1)));
        cleanup_file_deletions.push_back(std::make_tuple(nullptr, wrench::FileLocation::LOCATION(this->storage_service2)));

        ASSERT_THROW(new wrench::Workunit(std::shared_ptr<wrench::StandardJob>((wrench::StandardJob *)666, [](void *ptr){}),
                                          0.0,
                                          pre_file_copies, task, file_locations,
                                          post_file_copies, cleanup_file_deletions), std::invalid_argument);
    }
    {
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>> pre_file_copies;
        std::map<wrench::WorkflowFile *, std::vector<std::shared_ptr<wrench::FileLocation>>> file_locations;
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation> >> post_file_copies;
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation> >> cleanup_file_deletions;

        pre_file_copies.push_back(std::make_tuple(input_file, wrench::FileLocation::LOCATION(this->storage_service1), wrench::FileLocation::LOCATION(this->storage_service2)));
        file_locations[input_file] = {};
        file_locations[input_file].push_back(wrench::FileLocation::LOCATION(this->storage_service2));
        post_file_copies.push_back(std::make_tuple(output_file, wrench::FileLocation::LOCATION(this->storage_service2), wrench::FileLocation::LOCATION(this->storage_service1)));
        cleanup_file_deletions.push_back(std::make_tuple(input_file, nullptr));

        ASSERT_THROW(new wrench::Workunit(std::shared_ptr<wrench::StandardJob>((wrench::StandardJob *)666, [](void *ptr){}),
                                          0.0,
                                          pre_file_copies, task, file_locations,
                                          post_file_copies, cleanup_file_deletions), std::invalid_argument);
    }

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}



/**********************************************************************/
/**  DO WORKUNITEXECUTOR CONSTRUCTOR TEST                            **/
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
        auto task = this->getWorkflow()->addTask("task", 3600, 1, 1, 0);
        task->addInputFile(input_file);
        task->addOutputFile(output_file);
        auto job = job_manager->createStandardJob(task);


        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>> pre_file_copies;
        std::map<wrench::WorkflowFile *, std::vector<std::shared_ptr<wrench::FileLocation>>> file_locations;
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation> >> post_file_copies;
        std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation> >> cleanup_file_deletions;

        pre_file_copies.push_back(std::make_tuple(input_file,
                                                  wrench::FileLocation::LOCATION(this->test->storage_service1),
                                                  wrench::FileLocation::LOCATION(this->test->storage_service2)));
        file_locations[input_file] = {};
        file_locations[input_file].push_back(wrench::FileLocation::LOCATION(this->test->storage_service2));
        post_file_copies.push_back(std::make_tuple(output_file,
                                                   wrench::FileLocation::LOCATION(this->test->storage_service2),
                                                   wrench::FileLocation::LOCATION(this->test->storage_service1)));
        cleanup_file_deletions.push_back(std::make_tuple(input_file,
                                                         wrench::FileLocation::LOCATION(this->test->storage_service2)));

        auto wu = std::shared_ptr<wrench::Workunit>(new wrench::Workunit(
                job,
                0.0,
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
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Compute Service
    std::shared_ptr<wrench::ComputeService> compute_service;
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair(hostname, std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                                                {})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/disk1/"})));

    // Create another Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/disk2/"})));

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

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}






