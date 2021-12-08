
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
#define FILE_SIZE 1
#define NUM_STORAGE_SERVICES 10
#define STORAGE_SERVICE_CAPACITY (0.5 * NUM_FILES * FILE_SIZE)

WRENCH_LOG_CATEGORY(storage_service_link_failures_test, "Log category for StorageServiceLinkFailuresTest");


class StorageServiceLinkFailuresTest : public ::testing::Test {

public:
    std::vector<std::shared_ptr<wrench::DataFile> > files;
    std::vector<std::shared_ptr<wrench::StorageService>> storage_services;
    std::shared_ptr<wrench::FileRegistryService> file_registry_service = nullptr;

    void do_StorageServiceLinkFailureSimpleRandom_Test();

protected:
    StorageServiceLinkFailuresTest() {

        // Create the simplest workflow
        workflow = new wrench::Workflow();

        // Create a one-host platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> ";

        for (int host=0; host < 4; host++) {
            xml +=
                    "       <host id=\"Host" + std::to_string(host) + "\" speed=\"1f\" core=\"10\"> ";
            for (int i = 0; i < NUM_STORAGE_SERVICES; i++) {
                xml +=
                        "          <disk id=\"large_disk" + std::to_string(i) +
                        "\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                        "             <prop id=\"size\" value=\"" + std::to_string(STORAGE_SERVICE_CAPACITY) +
                        "B\"/>"
                        "             <prop id=\"mount\" value=\"/disk" + std::to_string(i) + "/\"/>"
                                                                                              "          </disk>";
            }

            xml +=
                    "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                    "             <prop id=\"size\" value=\"101B\"/>"
                    "             <prop id=\"mount\" value=\"/scratch\"/>"
                    "          </disk>"
                    "       </host>  ";
        }

        xml +=
                "       <link id=\"link1\" bandwidth=\"1Bps\" latency=\"0us\"/>"
                "       <link id=\"link2\" bandwidth=\"1Bps\" latency=\"0us\"/>"
                "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"link1\""
                "       /> </route>"
                "       <route src=\"Host1\" dst=\"Host3\"> <link_ctn id=\"link1\""
                "       /> </route>"
                "       <route src=\"Host2\" dst=\"Host3\"> <link_ctn id=\"link2\""
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
/**  LINK FAILURE TEST                                               **/
/**********************************************************************/

class StorageServiceLinkFailuresTestWMS : public wrench::WMS {

public:
    StorageServiceLinkFailuresTestWMS(StorageServiceLinkFailuresTest *test,
                                      std::string hostname) :
            wrench::WMS(nullptr, nullptr,  {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
        this->rng.seed(666);
    }

private:

    std::mt19937 rng;

    StorageServiceLinkFailuresTest *test;

    std::shared_ptr<wrench::DataMovementManager> data_movement_manager;

    std::shared_ptr<wrench::DataFile> findRandomFileOnStorageService(std::shared_ptr<wrench::StorageService> ss) {
        std::uniform_int_distribution<unsigned long> dist_files(0, this->test->files.size()-1);

        for (int trial = 0; trial < NUM_FILES; trial++) {
            auto potential_file = this->test->files.at(dist_files(rng));
            if (wrench::StorageService::lookupFile(potential_file, wrench::FileLocation::LOCATION(ss))) {
                return potential_file;
            }
        }
        return nullptr;
    }

    void doRandomSynchronousFileCopy() {

        std::uniform_int_distribution<unsigned long> dist_storage(
                0, this->test->storage_services.size()-1);

        auto source = wrench::FileLocation::LOCATION(this->test->storage_services.at(dist_storage(rng)));
        auto destination = wrench::FileLocation::LOCATION(this->test->storage_services.at(dist_storage(rng)));
        while (destination == source)
            destination = wrench::FileLocation::LOCATION(this->test->storage_services.at(dist_storage(rng)));

        auto file = findRandomFileOnStorageService(source->getStorageService());
        if ((file != nullptr) &&
            (not wrench::StorageService::lookupFile(file, destination))) {
            this->data_movement_manager->doSynchronousFileCopy(
                    file, source, destination, this->test->file_registry_service);
        }
    }

    void doRandomAsynchronousFileCopy() {

        std::uniform_int_distribution<unsigned long> dist_storage(
                0, this->test->storage_services.size()-1);

        auto source = wrench::FileLocation::LOCATION(this->test->storage_services.at(dist_storage(rng)));
        auto destination = wrench::FileLocation::LOCATION(this->test->storage_services.at(dist_storage(rng)));
        while (destination == source)
            destination = wrench::FileLocation::LOCATION(this->test->storage_services.at(dist_storage(rng)));

        auto file = findRandomFileOnStorageService(source->getStorageService());
        if ((file == nullptr) ||
            (wrench::StorageService::lookupFile(file, destination))) {
            return;
        }

        this->data_movement_manager->initiateAsynchronousFileCopy(
                file, source, destination, this->test->file_registry_service);

        // Wait for the event (with a timeout)
        std::shared_ptr<wrench::ExecutionEvent> event;
        try {
            event = this->waitForNextEvent(100);
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        if (event == nullptr) {
            WRENCH_INFO("TIMEOUT!");
        }
        if (std::dynamic_pointer_cast<wrench::FileCopyCompletedEvent>(event)) {
            return;
        }
        if (auto real_event = std::dynamic_pointer_cast<wrench::FileCopyFailedEvent>(event)) {
            throw wrench::ExecutionException(real_event->failure_cause);
        }

    }

    void doRandomFileDelete() {

        std::uniform_int_distribution<unsigned long> dist_storage(
                0, this->test->storage_services.size()-1);

        auto source = wrench::FileLocation::LOCATION(this->test->storage_services.at(dist_storage(rng)));
        auto file = findRandomFileOnStorageService(source->getStorageService());
        if (file == nullptr) {
            return;
        }
        auto space = source->getStorageService()->getFreeSpace();
        if (space.size() > 1) {
            throw std::runtime_error("There should be a single disk!");
        }
        if (space.begin()->second < STORAGE_SERVICE_CAPACITY * .25) {
            wrench::StorageService::deleteFile(file, source,  this->test->file_registry_service);
        }
    }

    void doRandomFileRead() {

        std::uniform_int_distribution<unsigned long> dist_storage(
                0, this->test->storage_services.size()-1);

        auto source = wrench::FileLocation::LOCATION(this->test->storage_services.at(dist_storage(rng)));
        auto file = findRandomFileOnStorageService(source->getStorageService());
        if (file == nullptr) {
            return;
        }

        wrench::StorageService::readFile(file, source);
    }

    void doRandomFileWrite() {
        static unsigned int count=0;

        std::uniform_int_distribution<unsigned long> dist_storage(
                0, this->test->storage_services.size()-1);

        auto dest = wrench::FileLocation::LOCATION(this->test->storage_services.at(dist_storage(rng)));
        auto file = this->getWorkflow()->addFile("written_file_" + std::to_string(count++), FILE_SIZE);
        wrench::StorageService::writeFile(file, dest);
        wrench::StorageService::deleteFile(file, dest);
    }

    

    int main() {

        // Create a link switcher on/off er for link1
        auto switcher1 = std::shared_ptr<wrench::ResourceRandomRepeatSwitcher>(
                new wrench::ResourceRandomRepeatSwitcher("Host1", 123, 1, 1045, 1, 5,
                                                         "link1", wrench::ResourceRandomRepeatSwitcher::ResourceType::LINK));
        switcher1->setSimulation(this->simulation);
        switcher1->start(switcher1, true, false); // Daemonized, no auto-restart

        // Create a link switcher on/off er for link2
        auto switcher2 = std::shared_ptr<wrench::ResourceRandomRepeatSwitcher>(
                new wrench::ResourceRandomRepeatSwitcher("Host1", 234, 1, 15, 1, 5,
                                                         "link2", wrench::ResourceRandomRepeatSwitcher::ResourceType::LINK));
        switcher2->setSimulation(this->simulation);
        switcher2->start(switcher2, true, false); // Daemonized, no auto-restart


        this->data_movement_manager = this->createDataMovementManager();


        unsigned long network_failure_1 = 0, network_failure_2 = 0;
        unsigned long network_failure_3 = 0, network_failure_4 = 0;
        unsigned long network_failure_5 = 0;

        // Do a bunch of operations
        unsigned long NUM_TRIALS = 5000;
        for (unsigned long i=0; i < NUM_TRIALS; i++) {

            // Do a random synchronous file copy
            try {
                this->doRandomSynchronousFileCopy();
            } catch (wrench::ExecutionException &e) {
                if (std::dynamic_pointer_cast<wrench::NetworkError>(e.getCause())) {
                    network_failure_1++;
                }
            }

            wrench::Simulation::sleep(5);

            // Do a random asynchronous copy
            try {
                this->doRandomAsynchronousFileCopy();
            } catch (wrench::ExecutionException &e) {
                if (std::dynamic_pointer_cast<wrench::NetworkError>(e.getCause())) {
                    e.getCause()->toString();
                }
            }

            wrench::Simulation::sleep(5);

            // Do a random delete
            try {
                this->doRandomFileDelete();
            } catch (wrench::ExecutionException &e) {
                if (std::dynamic_pointer_cast<wrench::NetworkError>(e.getCause())) {
                    network_failure_3++;
                }
            }

            wrench::Simulation::sleep(5);

            // Do a random file read
            try {
                this->doRandomFileRead();
            } catch (wrench::ExecutionException &e) {
                if (std::dynamic_pointer_cast<wrench::NetworkError>(e.getCause())) {
                    network_failure_4++;
                }
            }

            wrench::Simulation::sleep(5);

            // Do a random file write
            try {
                this->doRandomFileWrite();
            } catch (wrench::ExecutionException &e) {
                if (std::dynamic_pointer_cast<wrench::NetworkError>(e.getCause())) {
                    network_failure_5++;
                }
            }

            wrench::Simulation::sleep(5);
        }

        WRENCH_INFO("NETWORK FAILURES Sync   %lu", network_failure_1);
        WRENCH_INFO("NETWORK FAILURES Async  %lu", network_failure_2);
        WRENCH_INFO("NETWORK FAILURES Delete %lu", network_failure_3);
        WRENCH_INFO("NETWORK FAILURES Read   %lu", network_failure_4);
        WRENCH_INFO("NETWORK FAILURES Write   %lu", network_failure_5);

        return 0;
    }
};

TEST_F(StorageServiceLinkFailuresTest, SimpleRandomTest) {
    DO_TEST_WITH_FORK(do_StorageServiceLinkFailureSimpleRandom_Test);
}

void StorageServiceLinkFailuresTest::do_StorageServiceLinkFailureSimpleRandom_Test() {

    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a storage services
    double message_payload = 1;
    std::map<std::string, double> payloads =
            {
                    {wrench::StorageServiceMessagePayload::FREE_SPACE_REQUEST_MESSAGE_PAYLOAD, message_payload},
                    {wrench::StorageServiceMessagePayload::FREE_SPACE_ANSWER_MESSAGE_PAYLOAD, message_payload},
                    {wrench::StorageServiceMessagePayload::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD, message_payload},
                    {wrench::StorageServiceMessagePayload::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD, message_payload},
                    {wrench::StorageServiceMessagePayload::FILE_DELETE_REQUEST_MESSAGE_PAYLOAD, message_payload},
                    {wrench::StorageServiceMessagePayload::FILE_DELETE_ANSWER_MESSAGE_PAYLOAD, message_payload},
                    {wrench::StorageServiceMessagePayload::FILE_COPY_REQUEST_MESSAGE_PAYLOAD, message_payload},
                    {wrench::StorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD, message_payload},
                    {wrench::StorageServiceMessagePayload::FILE_WRITE_REQUEST_MESSAGE_PAYLOAD, message_payload},
                    {wrench::StorageServiceMessagePayload::FILE_WRITE_ANSWER_MESSAGE_PAYLOAD, message_payload},
                    {wrench::StorageServiceMessagePayload::FILE_READ_REQUEST_MESSAGE_PAYLOAD, message_payload},
                    {wrench::StorageServiceMessagePayload::FILE_READ_ANSWER_MESSAGE_PAYLOAD, message_payload},
                    {wrench::StorageServiceMessagePayload::FILE_NOT_FOUND_MESSAGE_PAYLOAD, message_payload},
                    {wrench::StorageServiceMessagePayload::NOT_ENOUGH_STORAGE_SPACE_MESSAGE_PAYLOAD, message_payload},
            };

    // Create a bunch of storage services
    for (unsigned int i = 0; i < NUM_STORAGE_SERVICES; i++) {
        std::string hostname;
        if (i % 2) {
            hostname = "Host2";
        } else {
            hostname = "Host3";
        }
        storage_services.push_back(simulation->add(
                new wrench::SimpleStorageService(hostname, {"/disk" + std::to_string(i)},
                                                 {{}}, payloads)));
    }


    file_registry_service = simulation->add(
            new wrench::FileRegistryService("Host1",
                                            {
                                                    {wrench::FileRegistryServiceProperty::LOOKUP_COMPUTE_COST, "0"},
                                                    {wrench::FileRegistryServiceProperty::ADD_ENTRY_COMPUTE_COST, "0"},
                                                    {wrench::FileRegistryServiceProperty::REMOVE_ENTRY_COMPUTE_COST, "0"}
                                            },
                                            {
                                                    {wrench::FileRegistryServiceMessagePayload::ADD_ENTRY_REQUEST_MESSAGE_PAYLOAD, message_payload},
                                                    {wrench::FileRegistryServiceMessagePayload::ADD_ENTRY_ANSWER_MESSAGE_PAYLOAD, message_payload},
                                                    {wrench::FileRegistryServiceMessagePayload::REMOVE_ENTRY_REQUEST_MESSAGE_PAYLOAD, message_payload},
                                                    {wrench::FileRegistryServiceMessagePayload::REMOVE_ENTRY_ANSWER_MESSAGE_PAYLOAD, message_payload},
                                                    {wrench::FileRegistryServiceMessagePayload::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD, message_payload},
                                                    {wrench::FileRegistryServiceMessagePayload::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD, message_payload}
                                            }));

    // Create a bunch of file
    for (unsigned int i=0; i < NUM_FILES; i++) {
        files.push_back(workflow->addFile("file_" + std::to_string(i) , FILE_SIZE));
    }

    // Stage some files randomly on storage services
    std::uniform_int_distribution<int> dist_stage(0,4);
    std::mt19937 rng(666);
    for (auto const &ss : storage_services) {
        for (auto const &f : files) {
            if (not dist_stage(rng)) {
                simulation->stageFile(f, ss);
            }
        }
    }

    std::shared_ptr<wrench::WMS> wms = nullptr;;
    wms = simulation->add(
            new StorageServiceLinkFailuresTestWMS(
                    this, "Host1"));

    wms->addWorkflow(workflow);

    simulation->launch();

    delete simulation;

    for (int i=0; i < argc; i++)
     free(argv[i]);
    free(argv);
}
