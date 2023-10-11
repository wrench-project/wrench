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
#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"


WRENCH_LOG_CATEGORY(storage_service_proxy_simultaneous_test, "Log category for StorageServiceProxySimultaneousTest");

class StorageServiceProxySimultaneousTest : public ::testing::Test {

public:
    void do_Simultaneous_test(std::string mode);

    std::shared_ptr<wrench::SimpleStorageService> remote;
    std::shared_ptr<wrench::StorageServiceProxy> proxy;

protected:
    StorageServiceProxySimultaneousTest() {


        // Create a one-host platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Client\" speed=\"1f\">"
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100TB\"/>"
                          "             <prop id=\"mount\" value=\"/disk100/\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Proxy\" speed=\"1f\">"
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100TB\"/>"
                          "             <prop id=\"mount\" value=\"/disk100/\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Remote\" speed=\"1f\">"
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100TB\"/>"
                          "             <prop id=\"mount\" value=\"/disk100/\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <link id=\"link12\" bandwidth=\"5GBps\" latency=\"10us\"/>"
                          "       <link id=\"link13\" bandwidth=\"100MBps\" latency=\"100us\"/>"
                          "      <link id=\"backdoor\" bandwidth=\"0.0001bps\" latency=\"100us\"/>"
                          "       <route src=\"Client\" dst=\"Proxy\"> <link_ctn id=\"link12\"/> </route>"
                          "       <route src=\"Proxy\" dst=\"Remote\"> <link_ctn id=\"link13\"/> </route>"
                          "   </zone> "
                          "</platform>";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};


/**********************************************************************/
/**  Simultaneous Read SIMULATION TEST                             **/
/**********************************************************************/

class StorageServiceProxySimultaneousTestExecutionController : public wrench::ExecutionController {

public:
    StorageServiceProxySimultaneousTestExecutionController(StorageServiceProxySimultaneousTest *test,
                                                           std::string hostname, const std::shared_ptr<wrench::DataFile> &file1, bool testWithDefault) : wrench::ExecutionController(hostname, "test"), file1(file1), test(test), order(testWithDefault) {
    }

private:
    std::shared_ptr<wrench::DataFile> file1;
    StorageServiceProxySimultaneousTest *test;
    bool order;
    int main() override {

        using namespace wrench;
        auto &proxy = test->proxy;
        WRENCH_INFO("Adding a file files to the simulation");


        if (order) {
            simulation->sleep(1);
        }
        WRENCH_INFO("Initiating Read %d", order);
        //read file again to check cache
        proxy->readFile(file1);
        //proxy->getCache()->readFile(file1);

        WRENCH_INFO("Terminating");
        return 0;
    }
};

TEST_F(StorageServiceProxySimultaneousTest, SimultaneousUncachedReadTest_CopyThenRead) {
    DO_TEST_WITH_FORK_ONE_ARG(do_Simultaneous_test, "CopyThenRead");
}
TEST_F(StorageServiceProxySimultaneousTest, SimultaneousUncachedReadTest_MagicRead) {
    DO_TEST_WITH_FORK_ONE_ARG(do_Simultaneous_test, "MagicRead");
}
TEST_F(StorageServiceProxySimultaneousTest, SimultaneousUncachedReadTest_ReadThrough) {
    DO_TEST_WITH_FORK_ONE_ARG(do_Simultaneous_test, "ReadThrough");
}

void StorageServiceProxySimultaneousTest::do_Simultaneous_test(std::string mode) {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //argv[3] = strdup("--wrench-full-log");
    //argv[1] = strdup("--log=wrench_core_mailbox.threshold=debug");
    //argv[2] = strdup("--log=wrench_core_proxy_file_server.threshold=debug");
    simulation->init(&argc, argv);

    simulation->instantiatePlatform(platform_file_path);
    if (mode == "ReadThrough") {

        {

            simgrid::s4u::Engine::get_instance()->netzone_by_name_or_null("AS0")->add_route(simgrid::s4u::Host::by_name("Remote")->get_netpoint(),
                                                                                            simgrid::s4u::Host::by_name("Client")->get_netpoint(),
                                                                                            nullptr,
                                                                                            nullptr,
                                                                                            {simgrid::s4u::LinkInRoute(simgrid::s4u::Link::by_name("link12")), simgrid::s4u::LinkInRoute(simgrid::s4u::Link::by_name("link13"))});
        }
    }

    // Create a XRootD Manager object


    this->remote = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
            "Remote", {"/disk100"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "10TB"}}, {}));
    auto cache = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
            "Proxy", {"/disk100"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "10TB"}}, {}));

    this->proxy = simulation->add(
            wrench::StorageServiceProxy::createRedirectProxy(
                    "Proxy", cache, remote, {{wrench::StorageServiceProxyProperty::UNCACHED_READ_METHOD, mode}}));

    auto file1 = wrench::Simulation::addFile("file1", 1E13);

    // Create a copy file1 on remote
    remote->createFile(file1);
    //cache->createFile(file1);
    // Create an execution controller
    auto controller1 = simulation->add(new StorageServiceProxySimultaneousTestExecutionController(this, "Client", file1, true));
    auto controller2 = simulation->add(new StorageServiceProxySimultaneousTestExecutionController(this, "Client", file1, false));
    // Running a "run a single task1" simulation

    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
