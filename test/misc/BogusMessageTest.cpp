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
#include <algorithm>
#include <exception>

#include "../include/TestWithFork.h"
#include "../include/UniqueTmpPathPrefix.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(bogus_message_test, "Log category for BogusMessageTest");


class BogusMessageTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::Service> service = nullptr;
    std::string dst_mailbox;

    void do_BogusMessage_Test(std::string service_type);

protected:
    BogusMessageTest() {


        // Create the simplest workflow
        workflow = new wrench::Workflow();

        // Create a one-host platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"10\" > "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"2000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"1000000B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Host2\" speed=\"1f\" core=\"10\" > "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"2000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"1000000B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Host3\" speed=\"1f\" core=\"10\" > "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"2000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"1000000B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Host4\" speed=\"1f\" core=\"10\" > "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"2000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"1000000B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                          "       <link id=\"2\" bandwidth=\"1000GBps\" latency=\"1000us\"/>"
                          "       <link id=\"3\" bandwidth=\"2000GBps\" latency=\"1500us\"/>"
                          "       <link id=\"4\" bandwidth=\"3000GBps\" latency=\"0us\"/>"
                          "       <link id=\"5\" bandwidth=\"8000GBps\" latency=\"0us\"/>"
                          "       <link id=\"6\" bandwidth=\"2900GBps\" latency=\"0us\"/>"
                          "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\""
                          "/> </route>"
                          "       <route src=\"Host3\" dst=\"Host4\"> <link_ctn id=\"2\""
                          "/> </route>"
                          "       <route src=\"Host1\" dst=\"Host3\"> <link_ctn id=\"3\""
                          "/> </route>"
                          "       <route src=\"Host1\" dst=\"Host4\"> <link_ctn id=\"4\""
                          "/> </route>"
                          "       <route src=\"Host2\" dst=\"Host4\"> <link_ctn id=\"5\""
                          "/> </route>"
                          "       <route src=\"Host2\" dst=\"Host3\"> <link_ctn id=\"6\""
                          "/> </route>"
                          "   </zone> "
                          "</platform>";

        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);

    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
    wrench::Workflow *workflow;

};

/**********************************************************************/
/**  BOGUS MESSAGE TEST                                              **/
/**********************************************************************/

class BogusMessageTestWMS : public wrench::WMS {

    class BogusMessage : public wrench::SimulationMessage {

    public:
        BogusMessage() : wrench::SimulationMessage("bogus", 1) {
        }
    };

public:
    BogusMessageTestWMS(BogusMessageTest *test,
                        std::string hostname) :
            wrench::WMS(nullptr, nullptr,  {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    BogusMessageTest *test;

    int main() {
        wrench::Simulation::sleep(1000);
        try {
            wrench::S4U_Mailbox::putMessage(this->test->dst_mailbox, new BogusMessage());
        } catch (std::runtime_error &e) {
        }
        return 0;
    }
};


class NoopWMS : public wrench::WMS {

public:
    NoopWMS(BogusMessageTest *test, std::string hostname, bool create_data_movement_manager) :
            wrench::WMS(nullptr, nullptr,  {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
        this->create_data_movement_manager = create_data_movement_manager;
    }

private:

    BogusMessageTest *test;
    bool create_data_movement_manager;

    int main() {

        if (this->create_data_movement_manager)
        {
            auto dmm = this->createDataMovementManager();
            this->test->dst_mailbox = dmm->mailbox_name;
        }
        this->waitForAndProcessNextEvent();
        return 0;
    }
};

/*************************************************************
 ** WARNING: THESE TESTS ARE A NICE THOUGHT, AND THEY WORK
 ** BUT ALTHOUGH THE "UNEXPECTED MESSAGE" EXECPTIONS ARE THROWN
 ** THE LINES OF CODE THAT THROW ARE NOT MARKED AS COVERED
 ** BY COVERALLS...
 *************************************************************/

TEST_F(BogusMessageTest, FileRegistryService) {
    DO_TEST_WITH_FORK_ONE_ARG_EXPECT_FATAL_FAILURE(do_BogusMessage_Test, "file_registry", false);
}

TEST_F(BogusMessageTest, SimpleStorage) {
    DO_TEST_WITH_FORK_ONE_ARG_EXPECT_FATAL_FAILURE(do_BogusMessage_Test, "simple_storage", true);
}

TEST_F(BogusMessageTest, WMS) {
    DO_TEST_WITH_FORK_ONE_ARG_EXPECT_FATAL_FAILURE(do_BogusMessage_Test, "wms", true);
}

TEST_F(BogusMessageTest, DataMovementManager) {
    DO_TEST_WITH_FORK_ONE_ARG_EXPECT_FATAL_FAILURE(do_BogusMessage_Test, "data_movement_manager", true);
}

void BogusMessageTest::do_BogusMessage_Test(std::string service_type) {


    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("file_registry_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a service
    if (service_type == "file_registry") {
        this->service = simulation->add(new wrench::FileRegistryService(hostname));
        this->dst_mailbox = this->service->mailbox_name;
    } else if (service_type == "simple_storage") {
        this->service = simulation->add(new wrench::SimpleStorageService(hostname, {"/"}));
        this->dst_mailbox = this->service->mailbox_name;
    } else if (service_type == "wms") {
        auto wms = new NoopWMS(this, hostname, false);
        wms->addWorkflow(workflow);
        this->service = simulation->add(wms);
        this->dst_mailbox = workflow->getCallbackMailbox();
    } else if (service_type == "data_movement_manager") {
        auto wms = new NoopWMS(this, hostname, true);
        wms->addWorkflow(workflow);
        this->service = simulation->add(wms);
        this->dst_mailbox = ""; // Will be set by the WMS on DMM is created
    }

    // Create the Bogus Message WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new BogusMessageTestWMS(this, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}
