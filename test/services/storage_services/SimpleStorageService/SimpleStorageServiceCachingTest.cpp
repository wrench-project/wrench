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

WRENCH_LOG_CATEGORY(simple_storage_service_lru_caching_test, "Log category for SimpleStorageServiceLRUCachingTest");


class SimpleStorageServiceCachingTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::StorageService> storage_service_1 = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service_2 = nullptr;

    void do_SimpleLRUCaching_test(double buffer_size);
    void do_SimpleLRUCachingCopy_test(double buffer_size);
    void do_SimpleLRUCachingUnevictable_test(double buffer_size);

protected:
    ~SimpleStorageServiceCachingTest() {
    }

    SimpleStorageServiceCachingTest() {

        // Create a one-host platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"SingleHost\" speed=\"1f\">"
                          "          <disk id=\"disk1\" read_bw=\"1Bps\" write_bw=\"1Bps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/disk1/\"/>"
                          "          </disk>"
                          "          <disk id=\"disk2\" read_bw=\"1Bps\" write_bw=\"1Bps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/disk2/\"/>"
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
/**  SIMPLE LRU CACHING  TEST                                        **/
/**********************************************************************/

class SimpleStorageServiceLRUCachingTestWMS : public wrench::ExecutionController {

public:
    SimpleStorageServiceLRUCachingTestWMS(SimpleStorageServiceCachingTest *test,
                                          const std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    SimpleStorageServiceCachingTest *test;

    int main() override {

        // Create 3 files
        auto file_50 = wrench::Simulation::addFile("file_50", 50);
        auto file_30 = wrench::Simulation::addFile("file_30", 30);
        auto file_10 = wrench::Simulation::addFile("file_10", 10);

        this->test->storage_service_1->writeFile(file_50);
        this->test->storage_service_1->writeFile(file_30);
        this->test->storage_service_1->writeFile(file_10);

        // The LRU list:  file_50, file_30, file_10

        // Add a file of size 70, which should kick out file_50 and file_30
        auto file_70 = wrench::Simulation::addFile("file_70", 70);

        this->test->storage_service_1->writeFile(file_70);

        // Check that file_50 and file_30 have been kicked out
        if (this->test->storage_service_1->lookupFile(
                    wrench::FileLocation::LOCATION(this->test->storage_service_1, file_50))) {
            throw std::runtime_error("File_50 should have been evicted");
        }
        if (this->test->storage_service_1->lookupFile(
                    wrench::FileLocation::LOCATION(this->test->storage_service_1, file_30))) {
            throw std::runtime_error("File_30 should have been evicted");
        }

        // The LRU list: file_10, file_70
        this->test->storage_service_1->readFile(file_10);

        // The LRU list: file_70, file_10

        // Add a file of size 20, which should fit
        auto file_20 = wrench::Simulation::addFile("file_20", 20);
        this->test->storage_service_1->writeFile(file_20);

        // The LRU list: file_70, file_10, file_20

        // Writing file_10, which is already there
        this->test->storage_service_1->writeFile(file_10);

        // The LRU list: file_70, file_20, file_10
        this->test->storage_service_1->readFile(file_70);
        // The LRU list: file_20, file_10, file_70

        // Add a file of size 30, which should evict file_20 and file_10
        this->test->storage_service_1->writeFile(file_30);

        // Check that file_10 has been evicted
        if (this->test->storage_service_1->lookupFile(
                    wrench::FileLocation::LOCATION(this->test->storage_service_1, file_10))) {
            throw std::runtime_error("File_10 should have been evicted");
        }
        // Check that file_20 has been evicted
        if (this->test->storage_service_1->lookupFile(
                    wrench::FileLocation::LOCATION(this->test->storage_service_1, file_20))) {
            throw std::runtime_error("File_20 should have been evicted");
        }
        // Check that file_70 has NOT been evicted
        if (not this->test->storage_service_1->lookupFile(
                    wrench::FileLocation::LOCATION(this->test->storage_service_1, file_70))) {
            throw std::runtime_error("File_70 should not have been evicted");
        }

        return 0;
    }
};

TEST_F(SimpleStorageServiceCachingTest, SimpleLRU) {
    DO_TEST_WITH_FORK_ONE_ARG(do_SimpleLRUCaching_test, 10);
    DO_TEST_WITH_FORK_ONE_ARG(do_SimpleLRUCaching_test, 0);
}

void SimpleStorageServiceCachingTest::do_SimpleLRUCaching_test(double buffer_size) {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

    //    std::cerr << "\nBUFFER SIZE = " << buffer_size << "\n";

    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //        argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];


    // Create A Storage Service
    ASSERT_NO_THROW(this->storage_service_1 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(
                                    hostname, {"/disk1"},
                                    {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, std::to_string(buffer_size)},
                                     {wrench::SimpleStorageServiceProperty::CACHING_BEHAVIOR, "LRU"}},
                                    {})));


    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
                            new SimpleStorageServiceLRUCachingTestWMS(this, hostname)));


    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  SIMPLE LRU CACHING COPY TEST                                    **/
/**********************************************************************/

class SimpleStorageServiceLRUCachingCopyTestWMS : public wrench::ExecutionController {

public:
    SimpleStorageServiceLRUCachingCopyTestWMS(SimpleStorageServiceCachingTest *test,
                                              const std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    SimpleStorageServiceCachingTest *test;

    int main() override {

        auto ss1 = this->test->storage_service_1;
        auto ss2 = this->test->storage_service_2;

        auto file_10 = wrench::Simulation::addFile("file_10", 10);
        auto file_20 = wrench::Simulation::addFile("file_20", 20);
        auto file_30 = wrench::Simulation::addFile("file_30", 30);
        auto file_40 = wrench::Simulation::addFile("file_40", 40);
        auto file_50 = wrench::Simulation::addFile("file_50", 50);
        auto file_60 = wrench::Simulation::addFile("file_60", 60);
        auto file_70 = wrench::Simulation::addFile("file_70", 70);

        ss1->writeFile(file_10);
        ss1->writeFile(file_20);
        ss1->writeFile(file_50);

        ss2->writeFile(file_30);
        ss2->writeFile(file_40);

        // The LRU list:  SS1: file_10, file_20, file_50
        //                SS2: file_30, file_40

        // Copy file_50 SS1 -> SS2
        wrench::StorageService::copyFile(wrench::FileLocation::LOCATION(ss1, file_50), wrench::FileLocation::LOCATION(ss2, file_50));

        // The LRU list:  SS1: file_10, file_20, file_50
        //                SS2: file_40, file_50
        if (ss2->lookupFile(file_30)) {
            throw std::runtime_error("file_30 should have been evicted");
        }
        if (not ss2->lookupFile(file_40)) {
            throw std::runtime_error("file_40 should not have been evicted");
        }
        if (not ss2->lookupFile(file_50)) {
            throw std::runtime_error("file_50 should have been stored");
        }

        // Read file_40 on SS1
        ss2->readFile(file_40);

        // The LRU list:  SS1: file_10, file_20, file_50
        //                SS2: file_50, file_40

        // Copy file file_20 SS1->SS2
        wrench::StorageService::copyFile(wrench::FileLocation::LOCATION(ss1, file_20), wrench::FileLocation::LOCATION(ss2, file_20));
        // The LRU list:  SS1: file_10, file_20, file_50
        //                SS2: file_40, file_20
        if (ss2->lookupFile(file_50)) {
            throw std::runtime_error("file_50 should have been evicted");
        }
        if (not ss2->lookupFile(file_40)) {
            throw std::runtime_error("file_40 should not have been evicted");
        }
        if (not ss2->lookupFile(file_20)) {
            throw std::runtime_error("file_20 should have been stored");
        }

        return 0;
    }
};

TEST_F(SimpleStorageServiceCachingTest, SimpleLRUCopy) {
    DO_TEST_WITH_FORK_ONE_ARG(do_SimpleLRUCachingCopy_test, 10);
    DO_TEST_WITH_FORK_ONE_ARG(do_SimpleLRUCachingCopy_test, 0);
}

void SimpleStorageServiceCachingTest::do_SimpleLRUCachingCopy_test(double buffer_size) {

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

    // Create TWO Storage Service
    ASSERT_NO_THROW(this->storage_service_1 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(
                                    hostname, {"/disk1"},
                                    {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, std::to_string(buffer_size)},
                                     {wrench::SimpleStorageServiceProperty::CACHING_BEHAVIOR, "LRU"}},
                                    {})));

    ASSERT_NO_THROW(this->storage_service_2 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(
                                    hostname, {"/disk2"},
                                    {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, std::to_string(buffer_size)},
                                     {wrench::SimpleStorageServiceProperty::CACHING_BEHAVIOR, "LRU"}},
                                    {})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
                            new SimpleStorageServiceLRUCachingCopyTestWMS(this, hostname)));


    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  SIMPLE LRU CACHING UNEVICTABLE TEST                             **/
/**********************************************************************/

class SimpleStorageServiceLRUCachingUnevictableTestWMS : public wrench::ExecutionController {

public:
    SimpleStorageServiceLRUCachingUnevictableTestWMS(SimpleStorageServiceCachingTest *test,
                                                     const std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    SimpleStorageServiceCachingTest *test;

    int main() override {

        auto data_manager = this->createDataMovementManager();

        auto ss1 = this->test->storage_service_1;
        auto ss2 = this->test->storage_service_2;

        auto file_10 = wrench::Simulation::addFile("file_10", 10);
        auto file_20 = wrench::Simulation::addFile("file_20", 20);
        auto file_30 = wrench::Simulation::addFile("file_30", 30);
        auto file_40 = wrench::Simulation::addFile("file_40", 40);
        auto file_50 = wrench::Simulation::addFile("file_50", 50);
        auto file_60 = wrench::Simulation::addFile("file_60", 60);
        auto file_70 = wrench::Simulation::addFile("file_70", 70);

        ss1->writeFile(file_10);
        ss1->writeFile(file_20);
        ss1->writeFile(file_50);

        ss2->writeFile(file_30);

        // The LRU list:  SS1: file_10, file_20, file_50
        //                SS2: file_30

        // Copy Asynchronously file_50 SS1 -> SS2
        //        std::cerr << "STARTING ASYNCHRONOUS COPY: file_50 SS1 -> SS2\n";
        data_manager->initiateAsynchronousFileCopy(wrench::FileLocation::LOCATION(ss1, file_50), wrench::FileLocation::LOCATION(ss2, file_50));

        wrench::Simulation::sleep(1);
        //        std::cerr << "Reading file_10 from SS1\n";
        ss1->readFile(wrench::FileLocation::LOCATION(ss1, file_10));
        //        std::cerr << "Reading file_20 from SS1\n";
        ss1->readFile(wrench::FileLocation::LOCATION(ss1, file_20));
        //        std::cerr << "NOW file_50 should be the LRU on SS1\n";

        // The LRU list:  SS1: file_50, file_10, file_20
        //                SS2: file_30

        // Writing a file to ss1, which "shouldn't" evict file_50 even though it's the LRU
        // Instead it should evict file_10 and file_20
        //        std::cerr << "WRITING file_40 to SS1, which should evict file_10 and file_20\n";
        ss1->writeFile(file_40);

        if (ss1->lookupFile(file_10)) {
            throw std::runtime_error("file_10 should have been evicted");
        }
        if (ss1->lookupFile(file_20)) {
            throw std::runtime_error("file_20 should have been evicted");
        }

        // Wait for the asynchronous file copy
        this->waitForAndProcessNextEvent();
        // The LRU list:  SS1: file_50, file_40
        //                SS2: file_30, file_50

        // LRU: SS1: file_50, file_40
        if (not ss1->lookupFile(file_50)) {
            throw std::runtime_error("file_50 should be on ss1");
        }
        if (not ss1->lookupFile(file_40)) {
            throw std::runtime_error("file_40 should be on ss1");
        }

        if (not ss2->lookupFile(file_30)) {
            throw std::runtime_error("file_30 should be on ss2");
        }
        if (not ss2->lookupFile(file_50)) {
            throw std::runtime_error("file_50 should be on ss2");
        }

        // Make file_30 the LRU on SS2
        ss2->readFile(file_30);

        // Make sure the file_50 can be evicted
        ss2->writeFile(file_60);

        if (not ss2->lookupFile(file_30)) {
            throw std::runtime_error("file_40 should be on ss2");
        }
        if (not ss2->lookupFile(file_60)) {
            throw std::runtime_error("file_60 should be on ss2");
        }
        if (ss2->lookupFile(file_50)) {
            throw std::runtime_error("file_50 should have been evicted on ss2");
        }


        return 0;
    }
};

TEST_F(SimpleStorageServiceCachingTest, SimpleLRUUnevictable) {
    DO_TEST_WITH_FORK_ONE_ARG(do_SimpleLRUCachingUnevictable_test, 10);
    DO_TEST_WITH_FORK_ONE_ARG(do_SimpleLRUCachingUnevictable_test, 0);
}

void SimpleStorageServiceCachingTest::do_SimpleLRUCachingUnevictable_test(double buffer_size) {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create TWO Storage Service
    ASSERT_NO_THROW(this->storage_service_1 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(
                                    hostname, {"/disk1"},
                                    {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, std::to_string(buffer_size)},
                                     {wrench::SimpleStorageServiceProperty::CACHING_BEHAVIOR, "LRU"}},
                                    {})));

    ASSERT_NO_THROW(this->storage_service_2 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(
                                    hostname, {"/disk2"},
                                    {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, std::to_string(buffer_size)},
                                     {wrench::SimpleStorageServiceProperty::CACHING_BEHAVIOR, "LRU"}},
                                    {})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
                            new SimpleStorageServiceLRUCachingUnevictableTestWMS(this, hostname)));


    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
