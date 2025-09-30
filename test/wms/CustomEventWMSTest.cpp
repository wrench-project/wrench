/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>
#include <wrench-dev.h>
#include "wrench/execution_controller/ExecutionControllerMessage.h"
#include <numeric>

#include "../include/TestWithFork.h"
#include "../include/UniqueTmpPathPrefix.h"

class CustomEventWMSTest : public ::testing::Test {
public:
    void do_sendAndProcessCustomEvent_test();
    std::shared_ptr<wrench::ExecutionController> sender_wms;
    std::shared_ptr<wrench::ExecutionController> receiver_wms;

protected:
    CustomEventWMSTest() {
        // Create a platform file
        std::string xml = "<?xml version='1.0'?>"
            "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
            "<platform version=\"4.1\"> "
            "   <zone id=\"AS0\" routing=\"Full\"> "
            "       <host id=\"DualCoreHost\" speed=\"1f\" core=\"2\"> "
            "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
            "             <prop id=\"size\" value=\"100B\"/>"
            "             <prop id=\"mount\" value=\"/\"/>"
            "          </disk>"
            "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
            "             <prop id=\"size\" value=\"100B\"/>"
            "             <prop id=\"mount\" value=\"/scratch\"/>"
            "          </disk>"
            "       </host>"
            "       <host id=\"QuadCoreHost\" speed=\"1f\" core=\"4\"> "
            "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
            "             <prop id=\"size\" value=\"100B\"/>"
            "             <prop id=\"mount\" value=\"/\"/>"
            "          </disk>"
            "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
            "             <prop id=\"size\" value=\"100B\"/>"
            "             <prop id=\"mount\" value=\"/scratch\"/>"
            "          </disk>"
            "       </host>"
            "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
            "       <route src=\"DualCoreHost\" dst=\"QuadCoreHost\"> <link_ctn id=\"1\"/> </route>"
            "   </zone> "
            "</platform>";
        FILE* platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};

/**********************************************************************/
/**  CLASSES                                                         **/
/**********************************************************************/

class MyCustomMessage : public wrench::ExecutionControllerCustomEventMessage {
public:
    MyCustomMessage(sg_size_t payload, int val) : wrench::ExecutionControllerCustomEventMessage(payload), value(val) {
    }

    int value;
};

class MyOtherCustomMessage : public wrench::ExecutionControllerCustomEventMessage {
public:
    MyOtherCustomMessage(sg_size_t payload, std::string str) : wrench::ExecutionControllerCustomEventMessage(payload),
                                                               text(str) {
    }

    std::string text;
};


class CustomEventWMS : public wrench::ExecutionController {
public:
    CustomEventWMS(CustomEventWMSTest* test,
                   const std::string& hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    CustomEventWMSTest* test;

    int main() override {
        if (this == this->test->sender_wms.get()) {
            // I AM THE SENDER
            this->test->receiver_wms->commport->dputMessage(new MyCustomMessage(0, 10));
            this->test->receiver_wms->commport->dputMessage(new MyOtherCustomMessage(0, "stuff"));
        }
        else {
            // I AM THE RECEIVER

            std::shared_ptr<wrench::ExecutionEvent> event;

            {
                event = this->waitForNextEvent();
                auto custom_event = std::dynamic_pointer_cast<wrench::CustomEvent>(event);
                if (not custom_event) {
                    throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
                }
                auto received_message = std::dynamic_pointer_cast<MyCustomMessage>(custom_event->message);
                if (not received_message) {
                    throw std::runtime_error("Was expecting a MyCustomMessage!");
                }
            }

            {
                event = this->waitForNextEvent();
                auto custom_event = std::dynamic_pointer_cast<wrench::CustomEvent>(event);
                if (not custom_event) {
                    throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
                }
                auto received_message = std::dynamic_pointer_cast<MyOtherCustomMessage>(custom_event->message);
                if (not received_message) {
                    throw std::runtime_error("Was expecting a MyCustomMessage!");
                }
            }
        }
        return 0;
    }
};

TEST_F(CustomEventWMSTest, CustomEventBasics) {
    DO_TEST_WITH_FORK(do_sendAndProcessCustomEvent_test);
}

void CustomEventWMSTest::do_sendAndProcessCustomEvent_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char**)calloc(argc, sizeof(char*));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Sender/Receiver WMS
    this->sender_wms = simulation->add(new CustomEventWMS(this, hostname));
    this->receiver_wms = simulation->add(new CustomEventWMS(this, hostname));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
