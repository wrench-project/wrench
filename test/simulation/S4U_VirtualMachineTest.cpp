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

#include "../include/TestWithFork.h"
#include "../include/UniqueTmpPathPrefix.h"

class S4U_VirtualMachineTest : public ::testing::Test {

public:

    void do_basic_Test();

protected:

    ~S4U_VirtualMachineTest() {
        workflow->clear();
    }

    S4U_VirtualMachineTest() {

        // Create the simplest workflow
        workflow = wrench::Workflow::createWorkflow();

        // Create a one-host platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"10\"/> "
                          "       <host id=\"Host2\" speed=\"1f\" core=\"10\"/> "
                          "       <host id=\"Host3\" speed=\"1f\" core=\"10\"/> "
                          "       <host id=\"Host4\" speed=\"1f\" core=\"10\"/> "
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
    std::shared_ptr<wrench::Workflow> workflow;

};

class Sleep100Daemon : public wrench::S4U_Daemon {

public:
    Sleep100Daemon(std::string hostname) :
            S4U_Daemon(hostname, "sleep100daemon", "sleep100daemon") {}

    int main() override {
        simgrid::s4u::this_actor::execute(100);
        return 0;
    }

};

/**********************************************************************/
/**  BASIC TEST                                                      **/
/**********************************************************************/


class S4U_VirtualMachineTestWMS : public wrench::ExecutionController {

public:
    S4U_VirtualMachineTestWMS(S4U_VirtualMachineTest *test,
                      std::string hostname) :
            wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:

    S4U_VirtualMachineTest *test;

    int main() {

        auto vm = new wrench::S4U_VirtualMachine("vm", 1, 1, {}, {});

        vm->getState();
        vm->getStateAsString();

        // Start the VM on a bogus host, which fails
        try {
            std::string bogus = "bogus";
            vm->start(bogus);
            throw std::runtime_error("Should not be able to start a VM on a bogus host");
        } catch (std::invalid_argument &e) {}

        std::string host = "Host1";

        // Suspend the VM, which fails
        try {
            vm->suspend();
            throw std::runtime_error("Should not be able to suspend a VM that's not running");
        } catch (std::runtime_error &e) {}

        // shutdown the VM, which fails
        try {
            vm->shutdown();
            throw std::runtime_error("Should not be able to shutdown a VM that's not running");
        } catch (std::runtime_error &e) {}

        // Start the VM for real
        vm->start(host);
        vm->getStateAsString();

        // Start the VM again, which fails
        try {
            vm->start(host);
            throw std::runtime_error("Should not be able to start a VM that's already running");
        } catch (std::runtime_error &e) {}

        // Resume the VM, which fails
        try {
            vm->resume();
            throw std::runtime_error("Should not be able to resume a VM that's not running");
        } catch (std::runtime_error &e) {}

        // Suspend the VM
        vm->suspend();
        vm->getStateAsString();

        // Resume the VM
        vm->resume();
        vm->getStateAsString();


        return 0;
    }
};

TEST_F(S4U_VirtualMachineTest, Basic) {
    DO_TEST_WITH_FORK(do_basic_Test);
}

void S4U_VirtualMachineTest::do_basic_Test() {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";


    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new S4U_VirtualMachineTestWMS(
                    this, hostname)));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    for (int i=0; i < argc; i++)
     free(argv[i]);
    free(argv);
}

