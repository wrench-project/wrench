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


WRENCH_LOG_CATEGORY(storage_service_proxy_basic_functional_test, "Log category for StorageServiceProxyBasicTest");

class StorageServiceProxyBasicTest : public ::testing::Test {

public:
    void do_BasicFunctionality_test(bool arg, std::string mode);

    std::shared_ptr<wrench::SimpleStorageService> remote;
    std::shared_ptr<wrench::SimpleStorageService> target;
    std::shared_ptr<wrench::StorageServiceProxy> proxy;

protected:
    StorageServiceProxyBasicTest() {


        // Create a one-host platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Client\" speed=\"1f\">"
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/disk100/\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Proxy\" speed=\"1f\">"
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/disk100/\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Remote\" speed=\"1f\">"
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/disk100/\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Target\" speed=\"1f\">"
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/disk100/\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <link id=\"link12\" bandwidth=\"5GBps\" latency=\"10us\"/>"
                          "       <link id=\"link13\" bandwidth=\"100MBps\" latency=\"100us\"/>"
                          "       <link id=\"link23\" bandwidth=\"100MBps\" latency=\"100us\"/>"
                          "      <link id=\"backdoor\" bandwidth=\"0.0001bps\" latency=\"100us\"/>"
                          "       <route src=\"Client\" dst=\"Proxy\"> <link_ctn id=\"link12\"/> </route>"
                          "       <route src=\"Proxy\" dst=\"Remote\"> <link_ctn id=\"link13\"/> </route>"
                          "       <route src=\"Proxy\" dst=\"Target\"> <link_ctn id=\"link23\"/> </route>"
                          //"      <route src=\"Client\" dst=\"Remote\"> <link_ctn id=\"backdoor\"/> </route>"
                          //"       <route src=\"Client\" dst=\"Target\"> <link_ctn id=\"backdoor\"/> </route>"
                          "   </zone> "
                          "</platform>";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};


/**********************************************************************/
/**  BASIC FUNCTIONALITY SIMULATION TEST                             **/
/**********************************************************************/

class StorageServiceProxyBasicTestExecutionController : public wrench::ExecutionController {

public:
    StorageServiceProxyBasicTestExecutionController(StorageServiceProxyBasicTest *test,
                                                    std::string hostname, bool testWithDefault) : wrench::ExecutionController(hostname, "test"), test(test), testWithDefault(testWithDefault) {
    }

private:
    StorageServiceProxyBasicTest *test;
    bool testWithDefault;
    int main() override {

        using namespace wrench;
        auto &proxy = test->proxy;
        ///sealing backdoor
        //S4U_Simulation::setLinkBandwidth("backdoor",0);
        WRENCH_INFO("Adding a file files to the simulation");
        auto file1 = wrench::Simulation::addFile("file1", 10000);
        auto file2 = wrench::Simulation::addFile("file2", 10000);
        auto file3 = wrench::Simulation::addFile("file3", 10000);

        try {
            proxy->createFile(file1);
            throw std::runtime_error("Should not be able to create a file on a proxy directly");
        } catch (std::runtime_error &ignore) {}

        // Create a copy file1 on remote
        this->test->remote->createFile(file1);
        // Create a copy file1 on target
        this->test->target->createFile(file2);


        // Create a copy file3 on remote and in cache
        //this->test->remote->createFile(file3);
        //proxy->getCache()->createFile(file3);

        // locate file1 via proxy
        //       if(testWithDefault) ((shared_ptr<StorageService>)proxy)->lookupFile(file1);
        WRENCH_INFO("Lookup tests");
        if (testWithDefault) {
            if (!proxy->lookupFile(file1)) {
                throw std::runtime_error("Failed to find file that exists on remote");
            }
        }


        // Try to lookup file2 from remote although it is on target
        auto start = simulation->getCurrentSimulatedDate();
        if (proxy->lookupFile(test->remote, file2)) {
            throw std::runtime_error("Found file that does not exist");
        }
        auto firstCache = simulation->getCurrentSimulatedDate() - start;

        // locate file2 via proxy and target
        if (!proxy->lookupFile(this->test->target, file2)) {
            throw std::runtime_error("Failed to find file that exists on target");
        }

        WRENCH_INFO("Read tests");
        // Read file1 via proxy
        if (testWithDefault) {
            proxy->readFile(file1);
            //read file again to check cache
            proxy->readFile(file1);
        }

        // Try to read file2 from remote although it is on target
        try {
            proxy->readFile(test->remote, file2);
            throw std::runtime_error("Non extant files should throw exceptions when not found");
        } catch (std::runtime_error &rethrow) {
            throw rethrow;
        } catch (wrench::ExecutionException &ignore) {
            ignore.what();
            //            cerr<<ignore.getCause()->toString()<<endl;
            if (not std::dynamic_pointer_cast<wrench::FileNotFound>(ignore.getCause())) {
                throw std::runtime_error("Should have received a FileNotFound execution when reading file2 from remote");
            }
        }
        // read file2 via proxy and target
        start = simulation->getCurrentSimulatedDate();
        proxy->readFile(this->test->target, file2);
        auto firstFile = simulation->getCurrentSimulatedDate();
        // read file2 via proxy and target
        proxy->readFile(this->test->target, file2);
        auto secondFile = simulation->getCurrentSimulatedDate();

        if (firstFile - start < (secondFile - firstFile) * 1.2) {
            throw std::runtime_error("caching was not significantly faster than not caching for file read");
        }

        //check the file again, it should now be cached
        start = simulation->getCurrentSimulatedDate();

        if (!proxy->lookupFile(test->remote, file2)) {
            throw std::runtime_error("Could not find previously found file after read");
        }
        auto secondCache = simulation->getCurrentSimulatedDate() - start;

        if (firstCache < secondCache * 1.2) {
            throw std::runtime_error("caching was not significantly faster than not caching for file lookup");
        }

        WRENCH_INFO("File Write tests");
        //File write tests
        proxy->writeFile(test->target, file3);

        if (testWithDefault) {
            proxy->writeFile(file3);
        }
        simulation->sleep(1000);
        WRENCH_INFO("Activating backdoor");
        ///activating backdoor
        S4U_Simulation::setLinkBandwidth("backdoor", 1000000000);
        proxy->getCache()->deleteFile(file3);
        if (proxy->lookupFile(test->target, file3)) {
            WRENCH_INFO("Proxy search works fine");
        }
        if (testWithDefault and !test->remote->lookupFile(file3)) {
            throw std::runtime_error("newly written file not found on remote");
        }
        if (!test->target->lookupFile(file3)) {
            throw std::runtime_error("newly written file not found on target");
        }

        //use if network problem
        //proxy->getCache()->deleteFile(file3);
        //if(!test->target->lookupFile(file3)){
        //    throw std::runtime_error("newly written file not found on target");
        //}

        WRENCH_INFO("Delete tests");
        proxy->deleteFile(test->target, file3);

        if (testWithDefault) {
            proxy->deleteFile(file3);
        }
        if (test->target->lookupFile(file3)) {
            throw std::runtime_error("file not deleted from cache");
        }

        if (testWithDefault and test->remote->lookupFile(file3)) {
            throw std::runtime_error("file not deleted from remote");
        }
        if (test->target->lookupFile(file3)) {
            throw std::runtime_error("file not deleted from target");
        }
        WRENCH_INFO("Terminating");
        proxy->stop();
        return 0;
    }
};

TEST_F(StorageServiceProxyBasicTest, BasicFunctionalityCopyThenReadNoDefaultLocation) {
    DO_TEST_WITH_FORK_TWO_ARGS(do_BasicFunctionality_test, false, "CopyThenRead");
}
TEST_F(StorageServiceProxyBasicTest, BasicFunctionalityCopyThenReadDefaultLocation) {
    DO_TEST_WITH_FORK_TWO_ARGS(do_BasicFunctionality_test, true, "CopyThenRead");
    //do_BasicFunctionality_test(true);
}
TEST_F(StorageServiceProxyBasicTest, BasicFunctionalityMagicReadNoDefaultLocation) {
    DO_TEST_WITH_FORK_TWO_ARGS(do_BasicFunctionality_test, false, "MagicRead");
}
TEST_F(StorageServiceProxyBasicTest, BasicFunctionalityMagicReadDefaultLocation) {
    DO_TEST_WITH_FORK_TWO_ARGS(do_BasicFunctionality_test, true, "MagicRead");
    //do_BasicFunctionality_test(true);
}
TEST_F(StorageServiceProxyBasicTest, BasicFunctionalityReadThroughNoDefaultLocation) {
    DO_TEST_WITH_FORK_TWO_ARGS(do_BasicFunctionality_test, false, "ReadThrough");
}
TEST_F(StorageServiceProxyBasicTest, BasicFunctionalityReadThroughDefaultLocation) {
    DO_TEST_WITH_FORK_TWO_ARGS(do_BasicFunctionality_test, true, "ReadThrough");
    //do_BasicFunctionality_test(true);
}

void StorageServiceProxyBasicTest::do_BasicFunctionality_test(bool arg, std::string mode) {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-default-control-message-size=1");
    //   argv[1] = strdup("--wrench-full-log");
    //argv[2] = strdup("--log=wrench_core_commport.threshold=debug");
    //argv[3] = strdup("--log=wrench_core_proxy_file_server.threshold=debug");
    simulation->init(&argc, argv);

    simulation->instantiatePlatform(platform_file_path);
    if (mode == "ReadThrough") {

        {

            simgrid::s4u::Engine::get_instance()->netzone_by_name_or_null("AS0")->add_route(simgrid::s4u::Host::by_name("Remote"),
                                                                                            simgrid::s4u::Host::by_name("Client"),
                                                                                            {simgrid::s4u::LinkInRoute(simgrid::s4u::Link::by_name("link12")), simgrid::s4u::LinkInRoute(simgrid::s4u::Link::by_name("link13"))});
        }
        {

            simgrid::s4u::Engine::get_instance()->netzone_by_name_or_null("AS0")->add_route(simgrid::s4u::Host::by_name("Target"),
                                                                                            simgrid::s4u::Host::by_name("Client"),
                                                                                            {simgrid::s4u::LinkInRoute(simgrid::s4u::Link::by_name("link12")), simgrid::s4u::LinkInRoute(simgrid::s4u::Link::by_name("link23"))});
        }
    } else {
        {

            simgrid::s4u::Engine::get_instance()->netzone_by_name_or_null("AS0")->add_route(simgrid::s4u::Host::by_name("Target"),
                                                                                            simgrid::s4u::Host::by_name("Client"),
                                                                                            {simgrid::s4u::LinkInRoute(simgrid::s4u::Link::by_name("backdoor"))});
        }
        {

            simgrid::s4u::Engine::get_instance()->netzone_by_name_or_null("AS0")->add_route(simgrid::s4u::Host::by_name("Remote"),
                                                                                            simgrid::s4u::Host::by_name("Client"),
                                                                                            {simgrid::s4u::LinkInRoute(simgrid::s4u::Link::by_name("backdoor"))});
        }
    }
    // Create a XRootD Manager object


    this->remote = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
            "Remote", {"/disk100"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "10MB"}}, {}));
    auto cache = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
            "Proxy", {"/disk100"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "10MB"}}, {}));
    this->target = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
            "Target", {"/disk100"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "10MB"}}, {}));

    if (arg) {
        this->proxy = simulation->add(
                wrench::StorageServiceProxy::createRedirectProxy(
                        "Proxy", cache, remote, {{wrench::StorageServiceProxyProperty::UNCACHED_READ_METHOD, mode}}));
    } else {
        this->proxy = simulation->add(
                wrench::StorageServiceProxy::createRedirectProxy(
                        "Proxy", cache, nullptr, {{wrench::StorageServiceProxyProperty::UNCACHED_READ_METHOD, mode}}));
    }

    // Create an execution controller
    auto controller = simulation->add(new StorageServiceProxyBasicTestExecutionController(this, "Client", arg));

    // Running a "run a single task1" simulation

    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
