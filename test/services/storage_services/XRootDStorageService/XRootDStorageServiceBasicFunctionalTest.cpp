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


WRENCH_LOG_CATEGORY(xrootd_storage_service_basic_functional_test, "Log category for XRootDServiceBasicFunctionalTest");


class XRootDServiceBasicFunctionalTest : public ::testing::Test {

public:
    void do_BasicFunctionality_test(std::string arg);

    std::shared_ptr<wrench::XRootD::Node> root_supervisor;

protected:
    XRootDServiceBasicFunctionalTest() {


        // Create a one-host platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\">"
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/disk100/\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Host2\" speed=\"1f\">"
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/disk100/\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Host3\" speed=\"1f\">"
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/disk100/\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <link id=\"link12\" bandwidth=\"5GBps\" latency=\"10us\"/>"
                          "       <link id=\"link13\" bandwidth=\"5GBps\" latency=\"10us\"/>"
                          "       <link id=\"link23\" bandwidth=\"5GBps\" latency=\"10us\"/>"
                          "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"link12\"/> </route>"
                          "       <route src=\"Host1\" dst=\"Host3\"> <link_ctn id=\"link13\"/> </route>"
                          "       <route src=\"Host3\" dst=\"Host3\"> <link_ctn id=\"link23\"/> </route>"
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

class XRootDServiceBasicFunctionalityTestExecutionController : public wrench::ExecutionController {

public:
    XRootDServiceBasicFunctionalityTestExecutionController(XRootDServiceBasicFunctionalTest *test,
                                                           std::string hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    XRootDServiceBasicFunctionalTest *test;

    int main() {

        WRENCH_INFO("Adding a file files to the simulation");
        auto file1 = wrench::Simulation::addFile("file1", 10000);
        auto file2 = wrench::Simulation::addFile("file2", 10000);
        auto file3 = wrench::Simulation::addFile("file3", 10000);

        try {
            this->test->root_supervisor->createFile(file1);
            throw std::runtime_error("Should not be able to create a file on a non-storage Node");
        } catch (std::runtime_error &ignore) {}

        // Create a copy file1 on the first child
        this->test->root_supervisor->getChild(0)->createFile(file1, "/disk100");
        // use the other file create just in case
        this->test->root_supervisor->getChild(0)->createFile(file3, wrench::FileLocation::LOCATION(this->test->root_supervisor->getChild(0)->getStorageServer(), "/disk100"));

        // Read file1 from XRootD
        this->test->root_supervisor->readFile(file1);
        //read file again to cover cache
        this->test->root_supervisor->readFile(file1);
        //read file directly from child
        this->test->root_supervisor->getChild(0)->readFile(file1);

        // Try to read file2 from XRootD although it is nowhere
        try {
            this->test->root_supervisor->readFile(file2);
            throw std::runtime_error("Non extant files should throw exceptions when not found");
        } catch (wrench::ExecutionException &ignore) {
            if (not std::dynamic_pointer_cast<wrench::FileNotFound>(ignore.getCause())) {
                throw std::runtime_error("Should have received a FileNotFound execution when reading file2 from supervisor");
            }
        }

        try {
            this->test->root_supervisor->getChild(0)->readFile(file2);
            throw std::runtime_error("Non extant files should throw exceptions when not found");
        } catch (wrench::ExecutionException &ignore) {
            if (not std::dynamic_pointer_cast<wrench::FileNotFound>(ignore.getCause())) {
                throw std::runtime_error("Should have received a FileNotFound execution when reading file2 from child");
            }
        }

        if(!this->test->root_supervisor->lookupFile(file1)) throw std::runtime_error("File that exists not located - cached");
        if(!this->test->root_supervisor->getChild(0)->lookupFile(file3)) throw std::runtime_error("File that exists not located - direct");
        if(!this->test->root_supervisor->lookupFile(file3)) throw std::runtime_error("File that exists not located - uncached");
        if(this->test->root_supervisor->lookupFile(file2)) throw std::runtime_error("File that does not exist located - indirect");
        if(this->test->root_supervisor->getChild(0)->lookupFile(file2)) throw std::runtime_error("File that does not exist located - direct");

        this->test->root_supervisor->deleteFile(file1);

        try {
            this->test->root_supervisor->readFile(file1);
            throw std::runtime_error("File not deleted properly");
        } catch (wrench::ExecutionException &ignore) {
            if (not std::dynamic_pointer_cast<wrench::FileNotFound>(ignore.getCause())) {
                throw std::runtime_error("Should have received a FileNotFound execution when reading file1");
            }
        }

        // attempt to write file directly to leaf
        this->test->root_supervisor->getChild(0)->writeFile(file3);

        // attempt to write file directly to supervisor.
        try {
            this->test->root_supervisor->writeFile(file3);
            throw std::runtime_error("Should not be able to write a file directly on a supervisor node");
        } catch (wrench::ExecutionException &ignore) {
            if (not std::dynamic_pointer_cast<wrench::NotAllowed>(ignore.getCause())) {
                throw std::runtime_error("Should have gotten a NotAllowed exception");
            }
        }

        // TODO: Do a file copy
//        std::cerr << "DOING THE FILE COPY\n";
//        wrench::StorageService::copyFile(
//                file3,
//                wrench::FileLocation::LOCATION(this->test->root_supervisor->getChild(0)),
//                wrench::FileLocation::LOCATION(this->test->root_supervisor->getChild(1)));

        //mark
        if(this->test->root_supervisor->getChild(3)!=nullptr){
            throw std::runtime_error("Got child where none should be found");
        }
        if(this->test->root_supervisor->getParent()!=nullptr){
            throw std::runtime_error("Root has a parent somehow");
        }
        if(this->test->root_supervisor->getChild(0)->getParent()!=this->test->root_supervisor.get()){
            throw std::runtime_error("Child has wrong parent somehow");
        }
        if(this->test->root_supervisor->getStorageServer()!=nullptr){
            throw std::runtime_error("Why does root have internal storage?");
        }

        //TODO not really sure a good way to test if load is correct but this covers, and root should be 0
        this->test->root_supervisor->getLoad();
        this->test->root_supervisor->getChild(0)->getLoad();

        return 0;

        //TODO hookup cache.clear to something or remove it

    }
};

TEST_F(XRootDServiceBasicFunctionalTest, BasicFunctionalityFullSimulation) {
    DO_TEST_WITH_FORK_ONE_ARG(do_BasicFunctionality_test, "false");
}
TEST_F(XRootDServiceBasicFunctionalTest, FastSearchQuickSimulation) {
    DO_TEST_WITH_FORK_ONE_ARG(do_BasicFunctionality_test, "true");

}

void XRootDServiceBasicFunctionalTest::do_BasicFunctionality_test(std::string arg) {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);

    // Setting up the platform
    simulation->instantiatePlatform(platform_file_path);

    // Create a XRootD Manager object

    wrench::XRootD::Deployment xrootd_deployment(simulation, {{wrench::XRootD::Property::CACHE_MAX_LIFETIME, "28800"}, {wrench::XRootD::Property::REDUCED_SIMULATION, arg},{wrench::XRootD::Property::FILE_NOT_FOUND_TIMEOUT, "10"}}, {{wrench::StorageServiceMessagePayload::FILE_WRITE_REQUEST_MESSAGE_PAYLOAD,1024}});


    this->root_supervisor = xrootd_deployment.createRootSupervisor("Host1");
    ASSERT_THROW(this->root_supervisor = xrootd_deployment.createRootSupervisor("Host1"), std::runtime_error);

    auto ss2 = this->root_supervisor->addChildStorageServer("Host2", "/disk100", {}, {});
    auto ss3 = this->root_supervisor->addChildStorageServer("Host3", "/disk100", {}, {});

    // Create an execution controller
    auto controller = simulation->add(new XRootDServiceBasicFunctionalityTestExecutionController(this, "Host1"));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
