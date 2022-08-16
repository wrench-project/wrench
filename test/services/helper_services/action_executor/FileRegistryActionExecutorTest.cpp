/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>
#include <wrench-dev.h>

#include <wrench/action/FileRegistryAction.h>
#include <wrench/services/helper_services/action_executor/ActionExecutorMessage.h>
#include <wrench/services/helper_services/action_executor/ActionExecutor.h>
#include <wrench/job/CompoundJob.h>
#include <wrench/failure_causes/HostError.h>

#include <memory>

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(file_registry_action_executor_test, "Log category for FileRegistryActionExecutorTest");

class FileRegistryActionExecutorTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::Simulation> simulation;

    void do_FileRegistryActionExecutorSuccessTest_test();

protected:
    FileRegistryActionExecutorTest() {

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
                          "       <link id=\"2\" bandwidth=\"0.1MBps\" latency=\"10us\"/>"
                          "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host2\" dst=\"Host3\"> <link_ctn id=\"2\"/> </route>"
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
    std::shared_ptr<wrench::Workflow> workflow;

public:
    std::shared_ptr<wrench::DataFile> file;
    std::shared_ptr<wrench::StorageService> ss;
    std::shared_ptr<wrench::FileRegistryService> fr;
};


/**********************************************************************/
/**  DO FILE_REGISTRY ACTION EXECUTOR SUCCESS TEST                   **/
/**********************************************************************/

class FileRegistryActionExecutorSuccessTestWMS : public wrench::ExecutionController {

public:
    FileRegistryActionExecutorSuccessTestWMS(FileRegistryActionExecutorTest *test,
                                         std::shared_ptr<wrench::Workflow> workflow,
                                         std::string hostname) : wrench::ExecutionController(hostname, "test"), test(test), workflow(workflow) {
    }

private:
    FileRegistryActionExecutorTest *test;
    std::shared_ptr<wrench::Workflow> workflow;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        {
            // Create a compound job
            auto job = job_manager->createCompoundJob("");
            // Add a file_read_action
            auto file_registry_add_entry_action = job->addFileRegistryAddEntryAction("", this->test->fr,
                                                                                     this->test->file,
                                                                                     wrench::FileLocation::LOCATION(
                                                                                             this->test->ss));
            // coverage
            wrench::Action::getActionTypeAsString(file_registry_add_entry_action);

            // Create a file read action executor
            auto file_read_action_executor = std::shared_ptr<wrench::ActionExecutor>(
                    new wrench::ActionExecutor("Host2", 0, 0.0, 0, false, this->mailbox, file_registry_add_entry_action,
                                               nullptr));

            // Start it
            file_read_action_executor->setSimulation(this->simulation);
            file_read_action_executor->start(file_read_action_executor, true, false);

            // Wait for a message from it
            std::shared_ptr<wrench::SimulationMessage> message;
            try {
                message = wrench::S4U_Mailbox::getMessage(this->mailbox);
            } catch (std::shared_ptr<wrench::NetworkError> &cause) {
                throw std::runtime_error("Network error while getting reply from Executor!" + cause->toString());
            }

            // Did we get the expected message?
            auto msg = std::dynamic_pointer_cast<wrench::ActionExecutorDoneMessage>(message);
            if (!msg) {
                throw std::runtime_error("Unexpected '" + message->getName() + "' message");
            }

            // Is the entry added
            if (this->test->fr->lookupEntry(this->test->file).size() != 1) {
                throw std::runtime_error("An entry should have been created");
            }
        }

        {
            // Create a compound job
            auto job = job_manager->createCompoundJob("");
            // Add a file_read_action
            auto file_registry_delete_entry_action = job->addFileRegistryDeleteEntryAction("", this->test->fr,
                                                                                     this->test->file,
                                                                                     wrench::FileLocation::LOCATION(
                                                                                             this->test->ss));
            // coverage
            wrench::Action::getActionTypeAsString(file_registry_delete_entry_action);
            file_registry_delete_entry_action->getFile();
            file_registry_delete_entry_action->getFileRegistryService();
            file_registry_delete_entry_action->getFileLocation();

            // Create a file read action executor
            auto file_read_action_executor = std::shared_ptr<wrench::ActionExecutor>(
                    new wrench::ActionExecutor("Host2", 0, 0.0, 0, false, this->mailbox, file_registry_delete_entry_action,
                                               nullptr));

            // Start it
            file_read_action_executor->setSimulation(this->simulation);
            file_read_action_executor->start(file_read_action_executor, true, false);

            // Wait for a message from it
            std::shared_ptr<wrench::SimulationMessage> message;
            try {
                message = wrench::S4U_Mailbox::getMessage(this->mailbox);
            } catch (std::shared_ptr<wrench::NetworkError> &cause) {
                throw std::runtime_error("Network error while getting reply from Executor!" + cause->toString());
            }

            // Did we get the expected message?
            auto msg = std::dynamic_pointer_cast<wrench::ActionExecutorDoneMessage>(message);
            if (!msg) {
                throw std::runtime_error("Unexpected '" + message->getName() + "' message");
            }

            // Is the entry added
            if (!this->test->fr->lookupEntry(this->test->file).empty()) {
                throw std::runtime_error("An entry should have been deleted");
            }
        }


        return 0;
    }
};

TEST_F(FileRegistryActionExecutorTest, SuccessTest) {
    DO_TEST_WITH_FORK(do_FileRegistryActionExecutorSuccessTest_test);
}


void FileRegistryActionExecutorTest::do_FileRegistryActionExecutorSuccessTest_test() {

    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a Storage Service
    this->ss = simulation->add(new wrench::SimpleStorageService("Host3", {"/"}));

    // Create a workflow
    workflow = wrench::Workflow::createWorkflow();

    // Create a file
    this->file = workflow->addFile("some_file", 1000000.0);

    wrench::Simulation::createFile(file, wrench::FileLocation::LOCATION(ss));

    // Create a file registry
    this->fr = simulation->add(new wrench::FileRegistryService("Host1"));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    wms = simulation->add(new FileRegistryActionExecutorSuccessTestWMS(this, workflow, "Host1"));

    ASSERT_NO_THROW(simulation->launch());

    workflow->clear();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
