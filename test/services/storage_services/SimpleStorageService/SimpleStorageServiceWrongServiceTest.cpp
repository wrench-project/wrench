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

WRENCH_LOG_CATEGORY(simple_storage_service_wrong_service_test, "Log category for SimpleStorageServiceWrongServiceTest");


class SimpleStorageServiceWrongServiceTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::Workflow> workflow;

    std::shared_ptr<wrench::DataFile> file_1;
    std::shared_ptr<wrench::DataFile> file_10;
    std::shared_ptr<wrench::DataFile> file_100;
    std::shared_ptr<wrench::SimpleStorageService> storage_service_1 = nullptr;
    std::shared_ptr<wrench::SimpleStorageService> storage_service_2 = nullptr;

    void do_WrongService_test(double buffer_size);


protected:
    ~SimpleStorageServiceWrongServiceTest() override {
        workflow->clear();
        wrench::Simulation::removeAllFiles();
    }

    SimpleStorageServiceWrongServiceTest() {

        // Create the simplest workflow
        workflow = wrench::Workflow::createWorkflow();

        // Create the files
        file_1 = wrench::Simulation::addFile("file_1", 1);
        file_10 = wrench::Simulation::addFile("file_10", 10);
        file_100 = wrench::Simulation::addFile("file_100", 100);

        // Create a one-host platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\">"
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/disk100/\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Host2\" speed=\"1f\">"
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/disk100/\"/>"
                          "          </disk>"
                          "       </host>"
                          "   </zone> "
                          "</platform>";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};


/**********************************************************************/
/**  WRONG SERVICE TEST                                              **/
/**********************************************************************/

class SimpleStorageServiceWrongServiceTestWMS : public wrench::ExecutionController {

public:
    SimpleStorageServiceWrongServiceTestWMS(SimpleStorageServiceWrongServiceTest *test,
                                                  const std::string& hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    SimpleStorageServiceWrongServiceTest *test;

    int main() override {

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Wrong-service read
        {
            try {
                this->test->storage_service_1->readFile(wrench::FileLocation::LOCATION(this->test->storage_service_2, this->test->file_1));
                throw std::runtime_error("Shouldn't be able to read with a wrong-service location");
            } catch (std::invalid_argument &ignore) {
            }
        }

        // Wrong-service write
        {
            try {
                this->test->storage_service_1->writeFile(wrench::FileLocation::LOCATION(this->test->storage_service_2, this->test->file_10));
                throw std::runtime_error("Shouldn't be able to write with a wrong-service location");
            } catch (std::invalid_argument &ignore) {
            }
        }

        return 0;
    }
};

TEST_F(SimpleStorageServiceWrongServiceTest, WrongService) {
    DO_TEST_WITH_FORK_ONE_ARG(do_WrongService_test, 1000000);
}

void SimpleStorageServiceWrongServiceTest::do_WrongService_test(double buffer_size) {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
//        argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create Three Storage Services
    ASSERT_NO_THROW(storage_service_1 = simulation->add(
            wrench::SimpleStorageService::createSimpleStorageService("Host1", {"/disk100"},
                                                                     {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, std::to_string(buffer_size)}}, {})));
    ASSERT_NO_THROW(storage_service_2 = simulation->add(
            wrench::SimpleStorageService::createSimpleStorageService("Host2", {"/disk100"},
                                                                     {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, std::to_string(buffer_size)}}, {})));

    // Create a WMS
    ASSERT_NO_THROW(simulation->add(new SimpleStorageServiceWrongServiceTestWMS(this, hostname)));

    // Staging all files on the 1000 storage service
    ASSERT_NO_THROW(storage_service_1->createFile(file_1));
    ASSERT_NO_THROW(storage_service_2->createFile(file_1));

    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

