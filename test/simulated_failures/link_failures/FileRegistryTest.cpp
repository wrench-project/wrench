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

#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"

#include "../failure_test_util/ResourceRandomRepeatSwitcher.h"

#define NUM_FILES 100
#define NUM_STORAGE_SERVICES 10

XBT_LOG_NEW_DEFAULT_CATEGORY(file_registry_link_failure_test, "Log category for FileRegistryLinkFailureTest");


class FileRegistryLinkFailureTest : public ::testing::Test {

public:
    std::vector<std::shared_ptr<wrench::StorageService>> storage_services;
    std::shared_ptr<wrench::FileRegistryService> file_registry_service = nullptr;

    void do_FileRegistryLinkFailureSimpleRandom_Test();

protected:
    FileRegistryLinkFailureTest() {

        // Create the simplest workflow
        workflow = new wrench::Workflow();

        // Create a one-host platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"10\"/> "
                          "       <host id=\"Host2\" speed=\"1f\" core=\"10\"/> "
                          "       <link id=\"link1\" bandwidth=\"1Bps\" latency=\"0us\"/>"
                          "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"link1\""
                          "       /> </route>"
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
/**  LINK FAILURE  TEST                                              **/
/**********************************************************************/

class FileRegistryLinkFailureTestWMS : public wrench::WMS {

public:
    FileRegistryLinkFailureTestWMS(FileRegistryLinkFailureTest *test,
                                   std::string hostname) :
            wrench::WMS(nullptr, nullptr,  {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    FileRegistryLinkFailureTest *test;

    int main() {

        // Create a bunch of files
        std::vector<wrench::WorkflowFile *> files;
        for (unsigned int i=0; i < NUM_FILES; i++) {
            files.push_back(this->getWorkflow()->addFile("file_" + std::to_string(i) , 100.0));
        }

        // Create a link switcher on/off er
        auto switcher = std::shared_ptr<wrench::ResourceRandomRepeatSwitcher>(
                new wrench::ResourceRandomRepeatSwitcher("Host1", 123, 1, 15, 1, 5,
                                                         "link1", wrench::ResourceRandomRepeatSwitcher::ResourceType::LINK));
        switcher->simulation = this->simulation;
        switcher->start(switcher, true, false); // Daemonized, no auto-restart

        std::mt19937 rng;
        rng.seed(666);
        std::uniform_int_distribution<unsigned long> dist_files(0, files.size() - 1);
        std::uniform_int_distribution<unsigned long> dist_storage(0, this->test->storage_services.size() - 1);

        // Do a bunch of file registry operations
        for (unsigned int i=0; i < 10000; i++) {
            try {
                // Do a random add
                wrench::Simulation::sleep(1.0);
                this->test->file_registry_service->addEntry(files.at(dist_files(rng)),
                                                            this->test->storage_services.at(dist_storage(rng)));

                // Do a random delete
                wrench::Simulation::sleep(1.0);
                this->test->file_registry_service->removeEntry(files.at(dist_files(rng)),
                                                               this->test->storage_services.at(dist_storage(rng)));

                // Do a random lookup
                wrench::Simulation::sleep(1.0);
                this->test->file_registry_service->lookupEntry(files.at(dist_files(rng)));

            } catch (wrench::WorkflowExecutionException &e) {
            }
        }


        return 0;
    }
};

TEST_F(FileRegistryLinkFailureTest, SimpleRandomTest) {
    DO_TEST_WITH_FORK(do_FileRegistryLinkFailureSimpleRandom_Test);
}

void FileRegistryLinkFailureTest::do_FileRegistryLinkFailureSimpleRandom_Test() {

    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("file_registry_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = simulation->getHostnameList()[0];

    // Create a bunch of storage services
    for (unsigned int i = 0; i < NUM_STORAGE_SERVICES; i++) {
        storage_services.push_back(simulation->add(
                new wrench::SimpleStorageService(hostname, 100000000.0)));
    }

    // Create a file registry service
    double message_payload = 2;
    std::map<std::string, double> payloads =
            {
                    {wrench::FileRegistryServiceMessagePayload::ADD_ENTRY_REQUEST_MESSAGE_PAYLOAD, message_payload},
                    {wrench::FileRegistryServiceMessagePayload::ADD_ENTRY_ANSWER_MESSAGE_PAYLOAD, message_payload},
                    {wrench::FileRegistryServiceMessagePayload::REMOVE_ENTRY_REQUEST_MESSAGE_PAYLOAD, message_payload},
                    {wrench::FileRegistryServiceMessagePayload::REMOVE_ENTRY_ANSWER_MESSAGE_PAYLOAD, message_payload},
                    {wrench::FileRegistryServiceMessagePayload::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD, message_payload},
                    {wrench::FileRegistryServiceMessagePayload::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD, message_payload}
            };
    std::map<std::string, std::string> props =
            {
                    {wrench::FileRegistryServiceProperty::LOOKUP_COMPUTE_COST, "0"}
            };

    file_registry_service = simulation->add(
            new wrench::FileRegistryService("Host2",
                                            props,
                                            payloads));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new FileRegistryLinkFailureTestWMS(
                    this, "Host1")));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}
