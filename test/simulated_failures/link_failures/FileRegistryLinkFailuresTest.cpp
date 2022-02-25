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

WRENCH_LOG_CATEGORY(file_registry_link_failures_test, "Log category for FileRegistryLinkFailuresTest");


class FileRegistryLinkFailuresTest : public ::testing::Test {

public:
    std::vector<std::shared_ptr<wrench::StorageService>> storage_services;
    std::shared_ptr<wrench::FileRegistryService> file_registry_service = nullptr;

    std::shared_ptr<wrench::Workflow> workflow;


    void do_FileRegistryLinkFailureSimpleRandom_Test();

protected:
    ~FileRegistryLinkFailuresTest() {
        workflow->clear();
    }

    FileRegistryLinkFailuresTest() {

        // Create the simplest workflow
        workflow = wrench::Workflow::createWorkflow();

        // Create a one-host platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"10\"> ";
        for (int i = 0; i < NUM_STORAGE_SERVICES; i++) {
            xml += "          <disk id=\"large_disk_" + std::to_string(i) + "\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                                                                            "             <prop id=\"size\" value=\"100B\"/>"
                                                                            "             <prop id=\"mount\" value=\"/disk_" + std::to_string(i) + "\"/>"
                                                                                                                                                   "          </disk>";
        }
        xml += "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
               "             <prop id=\"size\" value=\"101B\"/>"
               "             <prop id=\"mount\" value=\"/scratch\"/>"
               "          </disk>"
               "       </host>  "
               "       <host id=\"Host2\" speed=\"1f\" core=\"10\"> "
               "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
               "             <prop id=\"size\" value=\"100B\"/>"
               "             <prop id=\"mount\" value=\"/\"/>"
               "          </disk>"
               "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
               "             <prop id=\"size\" value=\"101B\"/>"
               "             <prop id=\"mount\" value=\"/scratch\"/>"
               "          </disk>"
               "       </host>  "
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

};

/**********************************************************************/
/**  LINK FAILURE  TEST                                              **/
/**********************************************************************/

class FileRegistryLinkFailuresTestWMS : public wrench::ExecutionController {

public:
    FileRegistryLinkFailuresTestWMS(FileRegistryLinkFailuresTest *test,
                                    std::string hostname) :
            wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:

    FileRegistryLinkFailuresTest *test;

    int main() {

        // Create a bunch of files
        std::vector<std::shared_ptr<wrench::DataFile> > files;
        for (unsigned int i=0; i < NUM_FILES; i++) {
            files.push_back(this->test->workflow->addFile("file_" + std::to_string(i) , 100.0));
        }

        // Create a link switcher on/off er
        auto switcher = std::shared_ptr<wrench::ResourceRandomRepeatSwitcher>(
                new wrench::ResourceRandomRepeatSwitcher("Host1", 123, 1, 15, 1, 5,
                                                         "link1", wrench::ResourceRandomRepeatSwitcher::ResourceType::LINK));
        switcher->setSimulation(this->simulation);
        switcher->start(switcher, true, false); // Daemonized, no auto-restart

        std::mt19937 rng(666);
        std::uniform_int_distribution<unsigned long> dist_files(0, files.size() - 1);
        std::uniform_int_distribution<unsigned long> dist_storage(0, this->test->storage_services.size() - 1);

        // Do a bunch of file registry operations
        for (unsigned int i=0; i < 10000; i++) {
            try {
                // Do a random add
                wrench::Simulation::sleep(1.0);
                this->test->file_registry_service->addEntry(files.at(dist_files(rng)),
                                                            wrench::FileLocation::LOCATION(this->test->storage_services.at(dist_storage(rng))));

                // Do a random delete
                wrench::Simulation::sleep(1.0);
                this->test->file_registry_service->removeEntry(files.at(dist_files(rng)),
                                                               wrench::FileLocation::LOCATION(this->test->storage_services.at(dist_storage(rng))));

                // Do a random lookup
                wrench::Simulation::sleep(1.0);
                this->test->file_registry_service->lookupEntry(files.at(dist_files(rng)));

            } catch (wrench::ExecutionException &e) {
            }
        }


        return 0;
    }
};

TEST_F(FileRegistryLinkFailuresTest, SimpleRandomTest) {
    DO_TEST_WITH_FORK(do_FileRegistryLinkFailureSimpleRandom_Test);
}

void FileRegistryLinkFailuresTest::do_FileRegistryLinkFailureSimpleRandom_Test() {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a bunch of storage services
    for (unsigned int i = 0; i < NUM_STORAGE_SERVICES; i++) {
        storage_services.push_back(simulation->add(
                new wrench::SimpleStorageService(hostname, {"/disk_" + std::to_string(i)})));
    }

    // Create a file registry service
    double message_payload = 2;
    wrench::WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE payloads =
            {
                    {wrench::FileRegistryServiceMessagePayload::ADD_ENTRY_REQUEST_MESSAGE_PAYLOAD, message_payload},
                    {wrench::FileRegistryServiceMessagePayload::ADD_ENTRY_ANSWER_MESSAGE_PAYLOAD, message_payload},
                    {wrench::FileRegistryServiceMessagePayload::REMOVE_ENTRY_REQUEST_MESSAGE_PAYLOAD, message_payload},
                    {wrench::FileRegistryServiceMessagePayload::REMOVE_ENTRY_ANSWER_MESSAGE_PAYLOAD, message_payload},
                    {wrench::FileRegistryServiceMessagePayload::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD, message_payload},
                    {wrench::FileRegistryServiceMessagePayload::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD, message_payload}
            };
    wrench::WRENCH_PROPERTY_COLLECTION_TYPE props =
            {
                    {wrench::FileRegistryServiceProperty::LOOKUP_COMPUTE_COST, "0"}
            };

    file_registry_service = simulation->add(
            new wrench::FileRegistryService("Host2",
                                            props,
                                            payloads));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new FileRegistryLinkFailuresTestWMS(
                    this, "Host1")));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());



    for (int i=0; i < argc; i++)
     free(argv[i]);
    free(argv);
}
