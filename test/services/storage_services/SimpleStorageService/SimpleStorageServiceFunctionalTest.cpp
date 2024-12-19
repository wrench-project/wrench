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

WRENCH_LOG_CATEGORY(simple_storage_service_functional_test, "Log category for SimpleStorageServiceFunctionalTest");


class SimpleStorageServiceFunctionalTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::Workflow> workflow;

    std::shared_ptr<wrench::DataFile> file_1;
    std::shared_ptr<wrench::DataFile> file_10;
    std::shared_ptr<wrench::DataFile> file_100;
    std::shared_ptr<wrench::DataFile> file_500;
    std::shared_ptr<wrench::SimpleStorageService> storage_service_100 = nullptr;
    std::shared_ptr<wrench::SimpleStorageService> storage_service_510 = nullptr;
    std::shared_ptr<wrench::SimpleStorageService> storage_service_1000 = nullptr;
    std::shared_ptr<wrench::SimpleStorageService> storage_service_multimp = nullptr;

    std::shared_ptr<wrench::FileRegistryService> file_registry_service = nullptr;

    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;

    void do_BasicFunctionality_test(sg_size_t buffer_size);

    void do_SynchronousFileCopy_test(sg_size_t buffer_size);

    void do_AsynchronousFileCopy_test(sg_size_t buffer_size);

    void do_SynchronousFileCopyFailures_test(sg_size_t buffer_size);

    void do_AsynchronousFileCopyFailures_test(sg_size_t buffer_size);

    void do_Partitions_test(sg_size_t buffer_size);

    void do_FileWrite_test(sg_size_t buffer_size);


protected:
    ~SimpleStorageServiceFunctionalTest() override {
        workflow->clear();
        wrench::Simulation::removeAllFiles();
    }

    SimpleStorageServiceFunctionalTest() {

        // Create the simplest workflow
        workflow = wrench::Workflow::createWorkflow();

        // Create the files
        file_1 = wrench::Simulation::addFile("file_1", 1);
        file_10 = wrench::Simulation::addFile("file_10", 10);
        file_100 = wrench::Simulation::addFile("file_100", 100);
        file_500 = wrench::Simulation::addFile("file_500", 500);

        // Create a one-host platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"SingleHost\" speed=\"1f\">"
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/disk100/\"/>"
                          "          </disk>"
                          "          <disk id=\"somewhere1\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/somewhere1/\"/>"
                          "          </disk>"
                          "          <disk id=\"somewhere2\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/somewhere2/\"/>"
                          "          </disk>"
                          "          <disk id=\"other_large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"510B\"/>"
                          "             <prop id=\"mount\" value=\"/disk510/\"/>"
                          "          </disk>"
                          "          <disk id=\"other_other_large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"1000B\"/>"
                          "             <prop id=\"mount\" value=\"/disk1000/\"/>"
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
/**  BASIC FUNCTIONALITY SIMULATION TEST                             **/
/**********************************************************************/

class SimpleStorageServiceBasicFunctionalityTestWMS : public wrench::ExecutionController {

public:
    SimpleStorageServiceBasicFunctionalityTestWMS(SimpleStorageServiceFunctionalTest *test,
                                                  const std::string& hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    SimpleStorageServiceFunctionalTest *test;

    int main() override {

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Do a few lookups from the file registry service
        for (const auto &f: {this->test->file_1, this->test->file_10, this->test->file_100, this->test->file_500}) {
            std::set<std::shared_ptr<wrench::FileLocation>> result = this->test->file_registry_service->lookupEntry(f);

            if ((result.size() != 1) || ((*(result.begin()))->getStorageService() != this->test->storage_service_1000)) {
                throw std::runtime_error(
                        "File registry service should know that file " + f->getID() + " is (only) on storage service " +
                        this->test->storage_service_1000->getName());
            }
        }



        // Call mount point methods for coverage
        {
            auto sss = std::dynamic_pointer_cast<wrench::SimpleStorageService>(this->test->storage_service_multimp);
            sss->getMountPoints();
            sss->hasMountPoint("/somewhere1");
            sss->hasMountPoint("/somewhere1_not");
            sss->hasMultipleMountPoints();
            try {
                sss->getBaseRootPath();
                throw std::runtime_error("Shouldn't be able to get base root path from a multi-mount-point simple storage service");
            } catch (std::runtime_error &ignore) {
            }
        }


        // Do a bogus lookup
        try {
            wrench::StorageService::lookupFileAtLocation(wrench::FileLocation::LOCATION(this->test->storage_service_1000, nullptr));
            throw std::runtime_error("Should not be able to lookup a nullptr file!");
        } catch (std::invalid_argument &) {
        }

        // Do a few queries to storage services
        for (const auto &f: {this->test->file_1, this->test->file_10, this->test->file_100, this->test->file_500}) {
            if ((not wrench::StorageService::lookupFileAtLocation(wrench::FileLocation::LOCATION(this->test->storage_service_1000, f))) ||
                (wrench::StorageService::lookupFileAtLocation(wrench::FileLocation::LOCATION(this->test->storage_service_100, f))) ||
                (wrench::StorageService::lookupFileAtLocation(wrench::FileLocation::LOCATION(this->test->storage_service_510, f)))) {
                throw std::runtime_error("Some storage services do/don't have the files that they shouldn't/should have");
            }
        }
        // Do a few of bogus copies
        try {
            wrench::StorageService::copyFile(
                    wrench::FileLocation::LOCATION(this->test->storage_service_1000, nullptr),
                    wrench::FileLocation::LOCATION(this->test->storage_service_100, nullptr));
            throw std::runtime_error("Should not be to able to copy a nullptr file!");
        } catch (std::invalid_argument &) {
        }

        try {
            wrench::StorageService::copyFile(
                    wrench::FileLocation::LOCATION(this->test->storage_service_100, this->test->file_500),
                    nullptr);
            throw std::runtime_error("Should not be able to copy a file to a nullptr location!");
        } catch (std::invalid_argument &) {
        }

        try {
            wrench::StorageService::copyFile(
                    nullptr,
                    wrench::FileLocation::LOCATION(this->test->storage_service_100, this->test->file_500));
            throw std::runtime_error("Should not be able to copy a file from a nullptr location!");
        } catch (std::invalid_argument &) {
        }


        // Copy a file to a storage service that doesn't have enough space
        try {
            wrench::StorageService::copyFile(
                    wrench::FileLocation::LOCATION(this->test->storage_service_1000, this->test->file_500),
                    wrench::FileLocation::LOCATION(this->test->storage_service_100, this->test->file_500));
            throw std::runtime_error(
                    "Should not be able to store a file to a storage service that doesn't have enough capacity");
        } catch (wrench::ExecutionException &e) {
        }

        // Make sure the copy didn't happen
        if (wrench::StorageService::lookupFileAtLocation(wrench::FileLocation::LOCATION(this->test->storage_service_100, this->test->file_500))) {
            throw std::runtime_error("File copy to a storage service without enough space shouldn't have succeeded");
        }

        if (this->test->storage_service_1000->getFileLastWriteDate(
                    wrench::FileLocation::LOCATION(this->test->storage_service_1000, this->test->file_10)) > 0) {
            throw std::runtime_error("Last file write date of staged file should be 0");
        }

        // Copy a file to a storage service that has enough space
        double before_copy = wrench::Simulation::getCurrentSimulatedDate();
        try {
            wrench::StorageService::copyFile(
                    wrench::FileLocation::LOCATION(this->test->storage_service_1000, this->test->file_10),
                    wrench::FileLocation::LOCATION(this->test->storage_service_100, this->test->file_10));
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Should be able to store a file to a storage service that has enough capacity");
        }
        double after_copy = wrench::Simulation::getCurrentSimulatedDate();

        double last_file_write_date = this->test->storage_service_100->getFileLastWriteDate(
                wrench::FileLocation::LOCATION(this->test->storage_service_100, this->test->file_10));
        if ((last_file_write_date < before_copy) or (last_file_write_date > after_copy)) {
            throw std::runtime_error("Last file write date is incoherent");
        }

        try {
            this->test->storage_service_100->getFileLastWriteDate((std::shared_ptr<wrench::FileLocation>) nullptr);
            throw std::runtime_error("Should not be able to pass a nullptr location to getFileLastWriteDate()");
        } catch (std::invalid_argument &ignore) {}


        // Send a free space request
        sg_size_t free_space;
        try {
            free_space = this->test->storage_service_100->getTotalFreeSpace();
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Should be able to get a storage's service free space");
        }

        if (free_space != 90) {
            throw std::runtime_error(
                    "Free space on storage service is wrong (" + std::to_string(free_space) + ") instead of 90.0");
        }

        // Send a free space request at a path (without the slash)
        try {
            free_space = this->test->storage_service_100->getTotalFreeSpaceAtPath("/disk100");
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Should be able to get a storage's service free space at a path");
        }
        if (free_space != 90) {
            throw std::runtime_error(
                    "Free space on storage service is wrong (" + std::to_string(free_space) + ") instead of 90.0");
        }

        // Send a free space request at a path (with the slash)
        try {
            free_space = this->test->storage_service_100->getTotalFreeSpaceAtPath("/disk100/");
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Should be able to get a storage's service free space at a path");
        }
        if (free_space != 90) {
            throw std::runtime_error(
                    "Free space on storage service is wrong (" + std::to_string(free_space) + ") instead of 90.0");
        }

        // Send a free space request at a bogus path
        try {
            free_space = this->test->storage_service_100->getTotalFreeSpaceAtPath("bogus");
        } catch (wrench::ExecutionException &ignore) {
            throw std::runtime_error("Should be able to get a storage's service free space, even at a bogus path");
        }
        if (free_space != 0) {
            throw std::runtime_error(
                    "Free space on storage service at a bogus path should be 0.0 (" + std::to_string(free_space) + ")");
        }

        // Bogus read
        try {
            wrench::StorageService::readFileAtLocation(wrench::FileLocation::LOCATION(this->test->storage_service_100, nullptr));
            throw std::runtime_error("Should not be able to read nullptr file");
        } catch (std::invalid_argument &e) {
        }

        // Read a file on a storage service
        try {
            wrench::StorageService::readFileAtLocation(wrench::FileLocation::LOCATION(this->test->storage_service_100, this->test->file_10));
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Should be able to read a file available on a storage service");
        }

        // Read a file on a storage service that doesn't have that file
        try {
            wrench::StorageService::readFileAtLocation(wrench::FileLocation::LOCATION(this->test->storage_service_100, this->test->file_100));
            throw std::runtime_error("Should not be able to read a file unavailable a storage service");
        } catch (wrench::ExecutionException &e) {
        }

        {// Test using readFiles()

            // Bogus read
            try {
                std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>> locations;
                locations[nullptr] = wrench::FileLocation::LOCATION(this->test->storage_service_100, nullptr);
                wrench::StorageService::readFiles(locations);
                throw std::runtime_error("Should not be able to read nullptr file");
            } catch (std::invalid_argument &e) {
            }


            // Read a file on a storage service
            try {
                std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>> locations;
                locations[this->test->file_10] = wrench::FileLocation::LOCATION(this->test->storage_service_100, this->test->file_10);
                wrench::StorageService::readFiles(locations);
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error("Should be able to read a file available on a storage service");
            }

            // Read a file on a storage service that doesn't have that file
            try {
                std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>> locations;
                locations[this->test->file_100] = wrench::FileLocation::LOCATION(this->test->storage_service_100, this->test->file_100);
                wrench::StorageService::readFiles(locations);
                throw std::runtime_error("Should not be able to read a file unavailable a storage service");
            } catch (wrench::ExecutionException &e) {
            }
        }

        {// Test using writeFiles()

            // Bogus write
            try {
                std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>> locations;
                locations[nullptr] = wrench::FileLocation::LOCATION(this->test->storage_service_100, nullptr);
                wrench::StorageService::writeFiles(locations);
                throw std::runtime_error("Should not be able to write nullptr file");
            } catch (std::invalid_argument &e) {
            }
        }


        // Delete a file on a storage service that doesn't have it
        try {
            wrench::StorageService::deleteFileAtLocation(wrench::FileLocation::LOCATION(this->test->storage_service_100, this->test->file_100));
            throw std::runtime_error("Should not be able to delete a file unavailable a storage service");
        } catch (wrench::ExecutionException &e) {
            auto cause = std::dynamic_pointer_cast<wrench::FileNotFound>(e.getCause());
            if (not cause) {
                throw std::runtime_error("Got an expected exception, but unexpected failure cause: " +
                                         e.getCause()->toString() + " (expected: FileNotFound)");
            }
            if (cause->getLocation()->getStorageService() != this->test->storage_service_100) {
                throw std::runtime_error(
                        "Got the expected 'file not found' exception, but the failure cause does not point to the correct storage service");
            }
            if (cause->getFile() != this->test->file_100) {
                throw std::runtime_error(
                        "Got the expected 'file not found' exception, but the failure cause does not point to the correct file");
            }
        }

        // Delete a file in a bogus path
        try {
            wrench::StorageService::deleteFileAtLocation(wrench::FileLocation::LOCATION(this->test->storage_service_100, "/disk100/bogus", this->test->file_100));
            throw std::runtime_error("Should not be able to delete a file unavailable a storage service");
        } catch (wrench::ExecutionException &e) {
            auto cause = std::dynamic_pointer_cast<wrench::FileNotFound>(e.getCause());
            if (not cause) {
                throw std::runtime_error("Got an expected 'file not found' exception, but unexpected failure cause: " +
                                         e.getCause()->toString() + " (expected: FileNotFound)");
            }
            if (cause->getLocation()->getStorageService() != this->test->storage_service_100) {
                throw std::runtime_error(
                        "Got the expected 'file not found' exception, but the failure cause does not point to the correct storage service");
            }
            if (cause->getFile() != this->test->file_100) {
                throw std::runtime_error(
                        "Got the expected 'file not found' exception, but the failure cause does not point to the correct file");
            }
        }

        // Delete a file on a storage service that has it
        try {
            wrench::StorageService::deleteFileAtLocation(
                    wrench::FileLocation::LOCATION(this->test->storage_service_100, this->test->file_10));
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Should  be able to delete a file available a storage service");
        }

        // Check that the storage capacity is back to what it should be
        try {
            free_space = this->test->storage_service_100->getTotalFreeSpace();
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Should be able to get a storage's service free space");
        }

        if (free_space != 100) {
            throw std::runtime_error(
                    "Free space on storage service is wrong (" + std::to_string(free_space) + ") instead of 100.0");
        }

        // Do a bogus asynchronous file copy (file = nullptr);
        try {
            data_movement_manager->initiateAsynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->storage_service_1000, nullptr),
                    wrench::FileLocation::LOCATION(this->test->storage_service_100, nullptr));
            throw std::runtime_error("Shouldn't be able to do an initiateAsynchronousFileCopy with a nullptr file");
        } catch (std::invalid_argument &e) {
        }


        // Do a bogus asynchronous file copy (src = nullptr);
        try {
            data_movement_manager->initiateAsynchronousFileCopy(
                    nullptr,
                    wrench::FileLocation::LOCATION(this->test->storage_service_100, this->test->file_1));
            throw std::runtime_error("Shouldn't be able to do an initiateAsynchronousFileCopy with a nullptr src");
        } catch (std::invalid_argument &e) {
        }

        // Do a bogus asynchronous file copy (dst = nullptr);
        try {
            data_movement_manager->initiateAsynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->storage_service_1000, this->test->file_1),
                    nullptr);
            throw std::runtime_error("Shouldn't be able to do an initiateAsynchronousFileCopy with a nullptr dst");
        } catch (std::invalid_argument &e) {
        }

        // Do a valid asynchronous file copy
        try {
            data_movement_manager->initiateAsynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->storage_service_1000, this->test->file_1),
                    wrench::FileLocation::LOCATION(this->test->storage_service_100, this->test->file_1));
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Error while submitting a file copy operations");
        }


        // Wait for a workflow execution event
        std::shared_ptr<wrench::ExecutionEvent> event;
        try {
            event = this->waitForNextEvent();
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
        }

        if (not std::dynamic_pointer_cast<wrench::FileCopyCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        // Check that the copy has happened
        if (!wrench::StorageService::lookupFileAtLocation(wrench::FileLocation::LOCATION(this->test->storage_service_100, this->test->file_1))) {
            throw std::runtime_error("Asynchronous file copy operation didn't copy the file");
        }

        // Check that the free space has been updated at the destination
        try {
            free_space = this->test->storage_service_100->getTotalFreeSpace();
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Should be able to get a storage's service free space");
        }
        if (free_space != 99) {
            throw std::runtime_error(
                    "Free space on storage service is wrong (" + std::to_string(free_space) + ") instead of 99.0");
        }


        // Do an INVALID asynchronous file copy (file too big)
        try {
            data_movement_manager->initiateAsynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->storage_service_1000, this->test->file_500),
                    wrench::FileLocation::LOCATION(this->test->storage_service_100, this->test->file_500));
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Error while submitting a file copy operations");
        }

        // Wait for a workflow execution event
        try {
            event = this->waitForNextEvent();
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
        }

        if (auto real_event = std::dynamic_pointer_cast<wrench::FileCopyFailedEvent>(event)) {
            auto cause = std::dynamic_pointer_cast<wrench::StorageServiceNotEnoughSpace>(real_event->failure_cause);
            if (not cause) {
                throw std::runtime_error("Got expected event but unexpected failure cause: " +
                                         real_event->failure_cause->toString() + " (expected: FileCopyFailedEvent)");
            }
        } else {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        // Do an INVALID asynchronous file copy (file not there)
        try {
            data_movement_manager->initiateAsynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->storage_service_100, this->test->file_500),
                    wrench::FileLocation::LOCATION(this->test->storage_service_510, this->test->file_500));
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Error while submitting a file copy operations");
        }

        // Wait for a workflow execution event
        try {
            event = this->waitForNextEvent();
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
        }

        auto real_event = std::dynamic_pointer_cast<wrench::FileCopyFailedEvent>(event);
        if (real_event) {
            auto cause = std::dynamic_pointer_cast<wrench::FileNotFound>(real_event->failure_cause);
            if (not cause) {
                throw std::runtime_error("Got expected event but unexpected failure cause: " +
                                         real_event->failure_cause->toString() + " (expected: FileNotFound)");
            }
        } else {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }
        // Do a really bogus file removal
        try {
            wrench::StorageService::deleteFileAtLocation(
                    wrench::FileLocation::LOCATION(this->test->storage_service_100, nullptr));
            throw std::runtime_error("Should not be able to delete a nullptr file from a location");
        } catch (std::invalid_argument &e) {
        }

        // Shutdown the service
        this->test->storage_service_100->stop();

        // Try to do stuff with a shutdown service
        try {
            wrench::StorageService::lookupFileAtLocation(
                    wrench::FileLocation::LOCATION(this->test->storage_service_100, this->test->file_1));
            throw std::runtime_error("Should not be able to lookup a file from a DOWN service");
        } catch (wrench::ExecutionException &e) {
            // Check Exception
            auto cause = std::dynamic_pointer_cast<wrench::ServiceIsDown>(e.getCause());
            if (not cause) {
                throw std::runtime_error("Got an exception, as expected, but of the unexpected failure cause: " +
                                         e.getCause()->toString() + " (expected: ServiceIsDown)");
            }
            // Check Exception details
            if (cause->getService() != this->test->storage_service_100) {
                throw std::runtime_error(
                        "Got the expected 'service is down' exception, but the failure cause does not point to the correct storage service");
            }
        }


        try {
            wrench::StorageService::lookupFileAtLocation(
                    wrench::FileLocation::LOCATION(this->test->storage_service_100, "/disk100", this->test->file_1));
            throw std::runtime_error("Should not be able to lookup a file from a DOWN service");
        } catch (wrench::ExecutionException &e) {
            // Check Exception
            auto cause = std::dynamic_pointer_cast<wrench::ServiceIsDown>(e.getCause());
            if (not cause) {
                throw std::runtime_error("Got an exception, as expected, but an unexpected failure cause: " +
                                         e.getCause()->toString() + " (was expecting ServiceIsDown)");
            }
            // Check Exception details
            if (cause->getService() != this->test->storage_service_100) {
                throw std::runtime_error(
                        "Got the expected 'service is down' exception, but the failure cause does not point to the correct storage service");
            }
        }


        try {
            wrench::StorageService::readFileAtLocation(
                    wrench::FileLocation::LOCATION(this->test->storage_service_100, this->test->file_1));
            throw std::runtime_error("Should not be able to read a file from a down service");
        } catch (wrench::ExecutionException &e) {
            // Check Exception
            auto cause = std::dynamic_pointer_cast<wrench::ServiceIsDown>(e.getCause());
            if (not cause) {
                throw std::runtime_error("Got an exception, as expected, but of the unexpected failure cause: " +
                                         e.getCause()->toString() + " (was expecting ServiceIsDown)");
            }
            // Check Exception details
            if (cause->getService() != this->test->storage_service_100) {
                throw std::runtime_error(
                        "Got the expected 'service is down' exception, but the failure cause does not point to the correct storage service");
            }
        }

        // Bogus write file with nullptr location
        try {
            wrench::StorageService::writeFileAtLocation(nullptr);
        } catch (std::invalid_argument &ignore) {}

        try {
            wrench::StorageService::writeFileAtLocation(
                    wrench::FileLocation::LOCATION(this->test->storage_service_100, this->test->file_1));
            throw std::runtime_error("Should not be able to write a file from a DOWN service");
        } catch (wrench::ExecutionException &e) {
            // Check Exception
            auto cause = std::dynamic_pointer_cast<wrench::ServiceIsDown>(e.getCause());
            if (not cause) {
                throw std::runtime_error("Got an exception, as expected, but of the unexpected failure cause: " +
                                         e.getCause()->toString() + " (was expecting ServiceIsDown)");
            }
            // Check Exception details
            if (cause->getService() != this->test->storage_service_100) {
                throw std::runtime_error(
                        "Got the expected 'service is down' exception, but the failure cause does not point to the correct storage service");
            }
        }

        try {
            this->test->storage_service_100->getTotalFreeSpace();
            throw std::runtime_error("Should not be able to get free space info from a DOWN service");
        } catch (wrench::ExecutionException &e) {
            // Check Exception
            auto cause = std::dynamic_pointer_cast<wrench::ServiceIsDown>(e.getCause());
            if (not cause) {
                throw std::runtime_error("Got an exception, as expected, but of the unexpected failure cause: " +
                                         e.getCause()->toString() + " (was expecting ServiceIsDown)");
            }
            // Check Exception details
            wrench::ServiceIsDown *real_cause = (wrench::ServiceIsDown *) e.getCause().get();
            if (real_cause->getService() != this->test->storage_service_100) {
                throw std::runtime_error(
                        "Got the expected 'service is down' exception, but the failure cause does not point to the correct storage service");
            }
        }

        return 0;
    }
};

TEST_F(SimpleStorageServiceFunctionalTest, BasicFunctionality) {
    DO_TEST_WITH_FORK_ONE_ARG(do_BasicFunctionality_test, 1000000);
    DO_TEST_WITH_FORK_ONE_ARG(do_BasicFunctionality_test, 0);
}

void SimpleStorageServiceFunctionalTest::do_BasicFunctionality_test(sg_size_t buffer_size) {

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

    // Create a Compute Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BareMetalComputeService(hostname,
                                                                {std::make_pair(hostname, std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                          wrench::ComputeService::ALL_RAM))},
                                                                {})));
    // Create a bad Storage Service (no mount point)
    ASSERT_THROW(storage_service_100 = simulation->add(
                         wrench::SimpleStorageService::createSimpleStorageService(hostname, {})),
                 std::invalid_argument);

    // Create a bad Storage Service (invalid mountpoint)
    ASSERT_THROW(storage_service_100 = simulation->add(
                         wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/bogus"})),
                 std::invalid_argument);

    // Create a Storage Service with a bogus property
    ASSERT_THROW(storage_service_100 = simulation->add(
                         wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/"}, {{wrench::SimpleStorageServiceProperty::MAX_NUM_CONCURRENT_DATA_CONNECTIONS, "BOGUS"}}, {})),
                 std::invalid_argument);

    // Create Three Storage Services
    ASSERT_NO_THROW(storage_service_100 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk100"},
                                                                                     {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, std::to_string(buffer_size)}}, {})));
    ASSERT_NO_THROW(storage_service_510 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk510"},
                                                                                     {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, std::to_string(buffer_size)}}, {})));
    ASSERT_NO_THROW(storage_service_1000 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk1000"},
                                                                                     {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, std::to_string(buffer_size)}}, {})));
    ASSERT_NO_THROW(storage_service_multimp = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/somewhere1", "/somewhere2"},
                                                                                     {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, std::to_string(buffer_size)}}, {})));

    // Create a file registry
    file_registry_service = simulation->add(new wrench::FileRegistryService(hostname));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
                            new SimpleStorageServiceBasicFunctionalityTestWMS(this, hostname)));

    // Staging all files on the 1000 storage service
    ASSERT_NO_THROW(storage_service_1000->createFile(file_1));
    ASSERT_NO_THROW(storage_service_1000->createFile(file_10));
    ASSERT_NO_THROW(storage_service_1000->createFile(file_100));
    ASSERT_NO_THROW(storage_service_1000->createFile(file_500));

    ASSERT_NO_THROW(file_registry_service->addEntry(wrench::FileLocation::LOCATION(storage_service_1000, file_1)));
    ASSERT_NO_THROW(file_registry_service->addEntry(wrench::FileLocation::LOCATION(storage_service_1000, file_10)));
    ASSERT_NO_THROW(file_registry_service->addEntry(wrench::FileLocation::LOCATION(storage_service_1000, file_100)));
    ASSERT_NO_THROW(file_registry_service->addEntry(wrench::FileLocation::LOCATION(storage_service_1000, file_500)));

    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  SYNCHRONOUS FILE COPY TEST                                     **/
/**********************************************************************/

class SimpleStorageServiceSynchronousFileCopyTestWMS : public wrench::ExecutionController {

public:
    SimpleStorageServiceSynchronousFileCopyTestWMS(SimpleStorageServiceFunctionalTest *test,
                                                   const std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    SimpleStorageServiceFunctionalTest *test;

    int main() override {

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Do a bogus file copy (file = nullptr)
        try {
            data_movement_manager->doSynchronousFileCopy(wrench::FileLocation::LOCATION(this->test->storage_service_1000, nullptr),
                                                         wrench::FileLocation::LOCATION(this->test->storage_service_510, nullptr));
            throw std::runtime_error("Shouldn't be able to do a synchronous file copy with a nullptr file");
        } catch (std::invalid_argument &e) {
        }

        // Do a bogus file copy (src = nullptr)
        try {
            data_movement_manager->doSynchronousFileCopy(nullptr,
                                                         wrench::FileLocation::LOCATION(this->test->storage_service_510, this->test->file_500));
            throw std::runtime_error("Shouldn't be able to do a synchronous file copy with a nullptr src");
        } catch (std::invalid_argument &e) {
        }

        // Do a bogus file copy (dst = nullptr)
        try {
            data_movement_manager->doSynchronousFileCopy(wrench::FileLocation::LOCATION(this->test->storage_service_1000, this->test->file_500),
                                                         nullptr);
            throw std::runtime_error("Shouldn't be able to do a synchronous file copy with a nullptr src");
        } catch (std::invalid_argument &e) {
        }

        // Do a bogus file copy (src->getFile() = dst->getFile())
        try {
            data_movement_manager->doSynchronousFileCopy(wrench::FileLocation::LOCATION(this->test->storage_service_1000, this->test->file_500),
                                                         wrench::FileLocation::LOCATION(this->test->storage_service_1000, this->test->file_1));
            throw std::runtime_error("Shouldn't be able to do a synchronous file copy where locations don't have the same file");
        } catch (std::invalid_argument &e) {
        }

        // Do the file copy with a bogus path
        try {
            data_movement_manager->doSynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->storage_service_1000, "/disk1000/whatever/", this->test->file_500),
                    wrench::FileLocation::LOCATION(this->test->storage_service_510, this->test->file_500));
            throw std::runtime_error("Should not be able to do a file copy with a bogus path");
        } catch (wrench::ExecutionException &e) {
            auto cause = e.getCause();
            if (auto real_cause_1 = std::dynamic_pointer_cast<wrench::InvalidDirectoryPath>(e.getCause())) {
                real_cause_1->toString();   // Coverage
                real_cause_1->getLocation();// Coverage
            } else if (auto real_cause_2 = std::dynamic_pointer_cast<wrench::FileNotFound>(e.getCause())) {
                real_cause_2->toString();   // Coverage
                real_cause_2->getFile();    // Coverage
                real_cause_2->getLocation();// Coverage
            } else {
                throw std::runtime_error("Got the expected exception, but the failure cause is not InvalidDirectoryPath or FileNotFound (it's " + cause->toString() + ")");
            }
        }

        // Do the file copy
        try {
            data_movement_manager->doSynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->storage_service_1000, this->test->file_500),
                    wrench::FileLocation::LOCATION(this->test->storage_service_510, this->test->file_500));
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Got an exception while doing a synchronous file copy: " + std::string(e.what()));
        }

        // Do the file copy again, which should fail
        try {
            data_movement_manager->doSynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->storage_service_1000, this->test->file_500),
                    wrench::FileLocation::LOCATION(this->test->storage_service_510, "/disk510/foo/", this->test->file_500));
            throw std::runtime_error("Should not be able to write a file beyond the storage capacity");
        } catch (wrench::ExecutionException &e) {
        }

        // Do another file copy with empty src dir and empty dst dir
        try {
            data_movement_manager->doSynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->storage_service_1000, "/disk1000", this->test->file_1),
                    wrench::FileLocation::LOCATION(this->test->storage_service_510, "/disk510", this->test->file_1));
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Got an exception while doing a synchronous file copy: " + std::string(e.what()));
        }

        return 0;
    }
};

TEST_F(SimpleStorageServiceFunctionalTest, SynchronousFileCopy) {
    DO_TEST_WITH_FORK_ONE_ARG(do_SynchronousFileCopy_test, 1000000);
    DO_TEST_WITH_FORK_ONE_ARG(do_SynchronousFileCopy_test, 0);
}

void SimpleStorageServiceFunctionalTest::do_SynchronousFileCopy_test(sg_size_t buffer_size) {

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

    // Create a  Compute Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BareMetalComputeService(hostname,
                                                                {std::make_pair(hostname, std::make_tuple(1, 0))}, "",
                                                                {})));

    // Create 2 Storage Services
    ASSERT_NO_THROW(storage_service_1000 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk1000"},
                                                                                     {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, std::to_string(buffer_size)}}, {})));

    ASSERT_NO_THROW(storage_service_510 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk510"},
                                                                                     {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, std::to_string(buffer_size)}}, {})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
                            new SimpleStorageServiceSynchronousFileCopyTestWMS(
                                    this, hostname)));

    // Create a file registry
    simulation->add(new wrench::FileRegistryService(hostname));

    // Staging file_500 on the 1000-byte storage service
    ASSERT_NO_THROW(storage_service_1000->createFile(file_1));

    // Staging file_500 on the 1000-byte storage service
    ASSERT_NO_THROW(storage_service_1000->createFile(file_500));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  ASYNCHRONOUS FILE COPY TEST                                     **/
/**********************************************************************/

class SimpleStorageServiceAsynchronousFileCopyTestWMS : public wrench::ExecutionController {

public:
    SimpleStorageServiceAsynchronousFileCopyTestWMS(SimpleStorageServiceFunctionalTest *test,
                                                    const std::string& hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    SimpleStorageServiceFunctionalTest *test;

    int main() override {

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Initiate bogus file copy
        try {
            data_movement_manager->initiateAsynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->storage_service_1000, this->test->file_500),
                    wrench::FileLocation::LOCATION(this->test->storage_service_510, this->test->file_10));
            throw std::runtime_error("Should not be able to do an asynchronous file copy for different files");
        } catch (std::invalid_argument &ignore) {
        }

        // Initiate a file copy
        try {
            data_movement_manager->initiateAsynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->storage_service_1000, this->test->file_500),
                    wrench::FileLocation::LOCATION(this->test->storage_service_510, this->test->file_500));
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Got an exception while trying to initiate a file copy: " + std::string(e.what()));
        }

        // Initiate it again which should fail
        try {
            data_movement_manager->initiateAsynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->storage_service_1000, this->test->file_500),
                    wrench::FileLocation::LOCATION(this->test->storage_service_510, this->test->file_500));
            throw std::runtime_error("A duplicate asynchronous file copy should fail!");
        } catch (wrench::ExecutionException &e) {
            auto cause = std::dynamic_pointer_cast<wrench::FileAlreadyBeingCopied>(e.getCause());
            if (not cause) {
                throw std::runtime_error("Got expected exception, but unexpected failure cause: " +
                                         e.getCause()->toString() +
                                         " (expected: FileAlreadyBeingCopied)");
            }
            if (cause->getSourceLocation()->getFile() != this->test->file_500) {
                throw std::runtime_error("Got expected failure cause, but failure cause does not point to the right file");
            }
            if (not cause->getDestinationLocation()->equal(wrench::FileLocation::LOCATION(this->test->storage_service_510, this->test->file_500))) {
                throw std::runtime_error("Got expected failure cause, but failure cause does not point to the right location");
            }
            //            if (cause->getDestinationLocation()->getPath() != cause->getDestinationLocation()->getStorageService()->getBaseRootPath()) {
            //                std::cerr << "cause->getDestinationLocation()->getPath(): " << cause->getDestinationLocation()->getPath() << "\n";
            //                std::cerr << "cause->getDestinationLocation()->getStorageService()->getBaseRootPath(): " << cause->getDestinationLocation()->getStorageService()->getBaseRootPath() << "\n";
            //                throw std::runtime_error("Got expected failure cause, but failure cause does not point to the right path");
            //            }
            cause->getSourceLocation();// for coverage
            cause->toString();         // for coverage
        }

        // Wait for the next execution event
        std::shared_ptr<wrench::ExecutionEvent> event;

        try {
            event = this->waitForNextEvent();
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
        }

        if (not std::dynamic_pointer_cast<wrench::FileCopyCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }


        return 0;
    }
};

TEST_F(SimpleStorageServiceFunctionalTest, AsynchronousFileCopy) {
    DO_TEST_WITH_FORK_ONE_ARG(do_AsynchronousFileCopy_test, 1000000);
    DO_TEST_WITH_FORK_ONE_ARG(do_AsynchronousFileCopy_test, 0);
}

void SimpleStorageServiceFunctionalTest::do_AsynchronousFileCopy_test(sg_size_t buffer_size) {

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

    // Create a Compute Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BareMetalComputeService(hostname,
                                                                {std::make_pair(hostname, std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                          wrench::ComputeService::ALL_RAM))},
                                                                {})));

    // Create 2 Storage Services
    ASSERT_NO_THROW(storage_service_1000 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk1000"},
                                                                                     {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, std::to_string(buffer_size)}})));

    ASSERT_NO_THROW(storage_service_510 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk510"},
                                                                                     {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, std::to_string(buffer_size)}})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
                            new SimpleStorageServiceAsynchronousFileCopyTestWMS(
                                    this, hostname)));

    // Create a file registry
    simulation->add(new wrench::FileRegistryService(hostname));

    // Staging file_500 on the 1000-byte storage service
    ASSERT_NO_THROW(storage_service_1000->createFile(file_500));
    ASSERT_NO_THROW(storage_service_1000->createFile(file_1, "/disk1000/some_files/"));// coverage


    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  SYNCHRONOUS FILE COPY TEST WITH FAILURES                        **/
/**********************************************************************/

class SimpleStorageServiceSynchronousFileCopyFailuresTestWMS : public wrench::ExecutionController {

public:
    SimpleStorageServiceSynchronousFileCopyFailuresTestWMS(SimpleStorageServiceFunctionalTest *test,
                                                           const std::string& hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    SimpleStorageServiceFunctionalTest *test;

    int main() override {

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Do the file copy while space doesn't fit
        try {
            data_movement_manager->doSynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->storage_service_1000, this->test->file_500),
                    wrench::FileLocation::LOCATION(this->test->storage_service_100, this->test->file_500));
            throw std::runtime_error("Should have gotten a 'not enough space' exception");
        } catch (wrench::ExecutionException &e) {
            // Check Exception
            auto cause = std::dynamic_pointer_cast<wrench::StorageServiceNotEnoughSpace>(e.getCause());
            if (not cause) {
                throw std::runtime_error("Got an expected exception but an unexpected failure cause: " +
                                         e.getCause()->toString() + " (expected: StorageServiceNotEnoughSpace)");
            }
            // Check Exception details
            std::string error_msg = cause->toString();
            if (cause->getFile() != this->test->file_500) {
                throw std::runtime_error(
                        "Got the expected 'not enough space' exception, but the failure cause does not point to the correct file");
            }
            if (cause->getStorageService() != this->test->storage_service_100) {
                throw std::runtime_error(
                        "Got the expected 'not enough space' exception, but the failure cause does not point to the correct storage service");
            }
        }
        // Check that it worked
        if (wrench::StorageService::hasFileAtLocation(wrench::FileLocation::LOCATION(this->test->storage_service_100, this->test->file_500))) {
            throw std::runtime_error("Copy shouldn't have worked!");
        }

        // Do a file copy from myself
        try {
            data_movement_manager->doSynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->storage_service_1000, this->test->file_500),
                    wrench::FileLocation::LOCATION(this->test->storage_service_1000, this->test->file_500));
        } catch (std::invalid_argument &e) {
            throw std::runtime_error("Copying a file onto itself shouldn't lead to an exception (just a printed warning)");
        }

        // Do the file copy for a file that's not there

        // First delete the file on the 1000 storage service
        wrench::StorageService::deleteFileAtLocation(wrench::FileLocation::LOCATION(this->test->storage_service_1000, this->test->file_500));

        try {
            data_movement_manager->doSynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->storage_service_1000, this->test->file_500),
                    wrench::FileLocation::LOCATION(this->test->storage_service_510, this->test->file_500));
            throw std::runtime_error("Should have gotten a 'file not found' exception");
        } catch (wrench::ExecutionException &e) {
            // Check Exception
            auto cause = std::dynamic_pointer_cast<wrench::FileNotFound>(e.getCause());
            if (not cause) {
                throw std::runtime_error("Got an expected exception but unexpected failure cause: " +
                                         e.getCause()->toString() + " (expected: FileNotFound)");
            }
            // Check Exception details
            if (cause->getFile() != this->test->file_500) {
                throw std::runtime_error(
                        "Got the expected 'file not found' exception, but the failure cause does not point to the correct file");
            }
            if (cause->getLocation()->getStorageService() != this->test->storage_service_1000) {
                throw std::runtime_error(
                        "Got the expected 'file not found' exception, but the failure cause does not point to the correct storage service");
            }
        }

        // Do the file copy from a src storage service that's down
        this->test->storage_service_1000->stop();

        try {
            data_movement_manager->doSynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->storage_service_1000, this->test->file_500),
                    wrench::FileLocation::LOCATION(this->test->storage_service_510, this->test->file_500));
            throw std::runtime_error("Should have gotten a 'service is down' exception");
        } catch (wrench::ExecutionException &e) {
            // Check Exception
            auto cause = std::dynamic_pointer_cast<wrench::ServiceIsDown>(e.getCause());
            if (not cause) {
                throw std::runtime_error("Got an exception, as expected, but of the unexpected failure cause: " +
                                         e.getCause()->toString() + " (expected: ServiceIsDown)");
            }
            // Check Exception details
            wrench::ServiceIsDown *real_cause = (wrench::ServiceIsDown *) e.getCause().get();
            if (real_cause->getService() != this->test->storage_service_1000) {
                throw std::runtime_error(
                        "Got the expected 'service is down' exception, but the failure cause does not point to the correct storage service");
            }
        }

        // Do the file copy to a dst storage service that's down
        this->test->storage_service_510->stop();

        try {
            data_movement_manager->doSynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->storage_service_100, this->test->file_1),
                    wrench::FileLocation::LOCATION(this->test->storage_service_510, this->test->file_1));
            throw std::runtime_error("Should have gotten a 'service is down' exception");
        } catch (wrench::ExecutionException &e) {
            // Check Exception
            auto cause = std::dynamic_pointer_cast<wrench::ServiceIsDown>(e.getCause());
            if (not cause) {
                throw std::runtime_error("Got an exception, as expected, but of the unexpected failure cause: " +
                                         e.getCause()->toString() + " (expected: ServiceIsDown)");
            }
            // Check Exception details
            wrench::ServiceIsDown *real_cause = (wrench::ServiceIsDown *) e.getCause().get();
            if (real_cause->getService() != this->test->storage_service_510) {
                throw std::runtime_error(
                        "Got the expected 'service is down' exception, but the failure cause does not point to the correct storage service");
            }
        }

        return 0;
    }
};

TEST_F(SimpleStorageServiceFunctionalTest, SynchronousFileCopyFailures) {
    DO_TEST_WITH_FORK_ONE_ARG(do_SynchronousFileCopyFailures_test, 1000000);
    DO_TEST_WITH_FORK_ONE_ARG(do_SynchronousFileCopyFailures_test, 0);
}

void SimpleStorageServiceFunctionalTest::do_SynchronousFileCopyFailures_test(sg_size_t buffer_size) {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");
    //    argv[2] = strdup("--log=wrench_core_commport.t=debug");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Compute Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BareMetalComputeService(hostname,
                                                                {std::make_pair(hostname, std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                          wrench::ComputeService::ALL_RAM))},
                                                                {})));

    // Create 3 Storage Services
    ASSERT_NO_THROW(storage_service_1000 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk1000"},
                                                                                     {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, std::to_string(buffer_size)}}, {})));

    ASSERT_NO_THROW(storage_service_510 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk510"},
                                                                                     {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, std::to_string(buffer_size)}}, {})));

    ASSERT_NO_THROW(storage_service_100 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk100"},
                                                                                     {{wrench::SimpleStorageServiceProperty::MAX_NUM_CONCURRENT_DATA_CONNECTIONS, "infinity"},
                                                                                      {wrench::SimpleStorageServiceProperty::BUFFER_SIZE, std::to_string(buffer_size)}},
                                                                                     {})));

    ASSERT_NO_THROW(storage_service_100->getPropertyValueAsDouble(wrench::SimpleStorageServiceProperty::MAX_NUM_CONCURRENT_DATA_CONNECTIONS));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
                            new SimpleStorageServiceSynchronousFileCopyFailuresTestWMS(
                                    this, hostname)));

    // Create a file registry
    simulation->add(new wrench::FileRegistryService(hostname));

    // Staging file_500 on the 1000-byte storage service
    ASSERT_NO_THROW(storage_service_1000->createFile(file_500));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  ASYNCHRONOUS FILE COPY TEST WITH FAILURES                        **/
/**********************************************************************/

class SimpleStorageServiceAsynchronousFileCopyFailuresTestWMS : public wrench::ExecutionController {

public:
    SimpleStorageServiceAsynchronousFileCopyFailuresTestWMS(SimpleStorageServiceFunctionalTest *test,
                                                            const std::string& hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    SimpleStorageServiceFunctionalTest *test;

    int main() override {

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Do the file copy while space doesn't fit
        try {
            data_movement_manager->initiateAsynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->storage_service_1000, this->test->file_500),
                    wrench::FileLocation::LOCATION(this->test->storage_service_100, this->test->file_500));
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Got an unexpected exception");
        }

        // Wait for the next execution event
        std::shared_ptr<wrench::ExecutionEvent> event;

        try {
            event = this->waitForNextEvent();
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
        }

        auto real_event = std::dynamic_pointer_cast<wrench::FileCopyFailedEvent>(event);
        if (real_event) {
            auto cause = std::dynamic_pointer_cast<wrench::StorageServiceNotEnoughSpace>(real_event->failure_cause);
            if (not cause) {
                throw std::runtime_error("Got an expected exception, but an unexpected failure cause: " +
                                         real_event->failure_cause->toString() + " (expected: StorageServiceNotEnoughSpace");
            }
            if (cause->getFile() != this->test->file_500) {
                throw std::runtime_error(
                        "Got the expected exception and failure type, but the failure cause doesn't point to the right file");
            }
            if (cause->getStorageService() != this->test->storage_service_100) {
                throw std::runtime_error(
                        "Got the expected exception and failure type, but the failure cause doesn't point to the right storage service");
            }

        } else {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        // Do the file copy for a file that's not there
        try {
            data_movement_manager->initiateAsynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->storage_service_1000, this->test->file_100),
                    wrench::FileLocation::LOCATION(this->test->storage_service_100, this->test->file_100));
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Got an unexpected exception");
        }

        // Wait for the next execution event
        try {
            event = this->waitForNextEvent();
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
        }

        auto real_event2 = std::dynamic_pointer_cast<wrench::FileCopyFailedEvent>(event);
        if (real_event2) {
            auto cause = std::dynamic_pointer_cast<wrench::FileNotFound>(real_event2->failure_cause);
            if (not cause) {
                throw std::runtime_error("Got an expected exception, but an unexpected failure cause: " +
                                         real_event2->failure_cause->toString() + " (expected: FileNotFound)");
            }
            if (cause->getFile() != this->test->file_100) {
                throw std::runtime_error(
                        "Got the expected exception and failure type, but the failure cause doesn't point to the right file");
            }
            if (cause->getLocation()->getStorageService() != this->test->storage_service_1000) {
                throw std::runtime_error(
                        "Got the expected exception and failure type, but the failure cause doesn't point to the right storage service");
            }
        } else {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        // Do the file copy for a src storage service that's down
        this->test->storage_service_1000->stop();

        try {
            data_movement_manager->initiateAsynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->storage_service_1000, this->test->file_100),
                    wrench::FileLocation::LOCATION(this->test->storage_service_510, this->test->file_100));
            throw std::runtime_error("Should have gotten a 'service is down' exception");

        } catch (wrench::ExecutionException &e) {

            auto cause = std::dynamic_pointer_cast<wrench::ServiceIsDown>(e.getCause());
            if (not cause) {
                throw std::runtime_error("Got an expected exception, but an unexpected failure cause: " +
                                         e.getCause()->toString() + " (expected: ServiceIsDown)");
            }
            if (cause->getService() != this->test->storage_service_1000) {
                throw std::runtime_error(
                        "Got the expected exception and failure type, but the failure cause doesn't point to the right storage service");
            }
        }


        // Do the file copy from a dst storage service that's down
        try {
            data_movement_manager->initiateAsynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->storage_service_510, this->test->file_500),
                    wrench::FileLocation::LOCATION(this->test->storage_service_1000, this->test->file_500));
            throw std::runtime_error("Should have gotten a 'service is down' exception");
        } catch (wrench::ExecutionException &e) {
            // Check Exception
            auto cause = std::dynamic_pointer_cast<wrench::ServiceIsDown>(e.getCause());
            if (not cause) {
                throw std::runtime_error("Got an exception, as expected, but of the unexpected failure cause: " +
                                         e.getCause()->toString() + " (expected: ServiceIsDown");
            }
            // Check Exception details
            if (cause->getService() != this->test->storage_service_1000) {
                throw std::runtime_error(
                        "Got the expected 'service is down' exception, but the failure cause does not point to the correct storage service");
            }
        }

        return 0;
    }
};

TEST_F(SimpleStorageServiceFunctionalTest, AsynchronousFileCopyFailures) {
    DO_TEST_WITH_FORK_ONE_ARG(do_AsynchronousFileCopyFailures_test, 1000000);
    DO_TEST_WITH_FORK_ONE_ARG(do_AsynchronousFileCopyFailures_test, 0);
}

void SimpleStorageServiceFunctionalTest::do_AsynchronousFileCopyFailures_test(sg_size_t buffer_size) {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Compute Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BareMetalComputeService(hostname,
                                                                {std::make_pair(hostname, std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                          wrench::ComputeService::ALL_RAM))},
                                                                {})));

    // Create 3 Storage Services
    ASSERT_NO_THROW(storage_service_1000 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk1000"},
                                                                                     {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, std::to_string(buffer_size)}}, {})));

    ASSERT_NO_THROW(storage_service_510 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk510"},
                                                                                     {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, std::to_string(buffer_size)}}, {})));

    ASSERT_NO_THROW(storage_service_100 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk100"},
                                                                                     {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, std::to_string(buffer_size)}}, {})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
                            new SimpleStorageServiceAsynchronousFileCopyFailuresTestWMS(
                                    this, hostname)));

    // Create a file registry
    simulation->add(new wrench::FileRegistryService(hostname));

    // Staging file_500 on the 1000-byte storage service
    ASSERT_NO_THROW(storage_service_1000->createFile(file_500));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  DIRECTORIES TEST                                                **/
/**********************************************************************/

class PartitionsTestWMS : public wrench::ExecutionController {

public:
    PartitionsTestWMS(SimpleStorageServiceFunctionalTest *test,
                      const std::string& hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    SimpleStorageServiceFunctionalTest *test;

    int main() override {

        std::shared_ptr<wrench::ExecutionEvent> event;

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

//        wrench::StorageService::createFileAtLocation(wrench::FileLocation::LOCATION(this->test->storage_service_510, "/disk510/foo/", this->test->file_10));
        // Copy storage_service_1000:/:file_10 to storage_service_510:foo:file_10
        try {
            data_movement_manager->initiateAsynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->storage_service_1000, "/disk1000", this->test->file_10),
                    wrench::FileLocation::LOCATION(this->test->storage_service_510, "/disk510/foo/", this->test->file_10));
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Got an unexpected exception");
        }

        // Wait for the next execution event

        try {
            event = this->waitForNextEvent();
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
        }

        if (not std::dynamic_pointer_cast<wrench::FileCopyCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        // Copy storage_service_1000:/:file_10 to storage_service_510:/:file_10
        try {
            data_movement_manager->initiateAsynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->storage_service_1000, "/disk1000/", this->test->file_10),
                    wrench::FileLocation::LOCATION(this->test->storage_service_510, "/disk510", this->test->file_10));
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Got an unexpected exception");
        }

        // Wait for the next execution event
        try {
            event = this->waitForNextEvent();
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
        }

        if (not std::dynamic_pointer_cast<wrench::FileCopyCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        // Remove the file at storage_service_510:/:file_10
        try {
            wrench::StorageService::deleteFileAtLocation(wrench::FileLocation::LOCATION(this->test->storage_service_510, "/disk510", this->test->file_10));
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Should be able to delete file storage_service_510:/:file_10");
        }


        // Copy storage_service_510:/:file_10 to storage_service_1000:foo:file_10: SHOULD NOT WORK
        try {
            data_movement_manager->initiateAsynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->storage_service_510, this->test->file_10),
                    wrench::FileLocation::LOCATION(this->test->storage_service_1000, "/disk1000/foo", this->test->file_10));
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Got an unexpected exception");
        }

        // Wait for the next execution event
        try {
            event = this->waitForNextEvent();
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
        }

        if (not std::dynamic_pointer_cast<wrench::FileCopyFailedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        // Copy storage_service_510:foo:file_10 to storage_service_1000:foo
        try {
            data_movement_manager->initiateAsynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->storage_service_510, "/disk510/foo", this->test->file_10),
                    wrench::FileLocation::LOCATION(this->test->storage_service_1000, "/disk1000/foo", this->test->file_10));
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Got an unexpected exception");
        }

        // Wait for the next execution event
        try {
            event = this->waitForNextEvent();
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
        }

        if (not std::dynamic_pointer_cast<wrench::FileCopyCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        // Copy storage_service_510:foo:file_10 to storage_service_510:bar
        try {
            data_movement_manager->initiateAsynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->storage_service_510, "/disk510/foo", this->test->file_10),
                    wrench::FileLocation::LOCATION(this->test->storage_service_510, "/disk510/bar", this->test->file_10));
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Got an unexpected exception");
        }

        // Wait for the next execution event
        try {
            event = this->waitForNextEvent();
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
        }

        if (not std::dynamic_pointer_cast<wrench::FileCopyCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        // Copy storage_service_510:foo:file_10 to storage_service_510:foo    SHOULD NOT WORK
        try {
            data_movement_manager->initiateAsynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->storage_service_510, "/disk510/foo", this->test->file_10),
                    wrench::FileLocation::LOCATION(this->test->storage_service_510, "/disk510/foo", this->test->file_10));
        } catch (std::invalid_argument &e) {
            throw std::runtime_error("Should be able to copy a file onto itself (no-op)");
        }

        // Check all lookups
        if (not wrench::StorageService::lookupFileAtLocation(wrench::FileLocation::LOCATION(this->test->storage_service_1000, this->test->file_10))) {
            throw std::runtime_error("File should be in storage_service_1000 at the mount point root");
        }
        if (not wrench::StorageService::lookupFileAtLocation(wrench::FileLocation::LOCATION(this->test->storage_service_1000, "/disk1000", this->test->file_10))) {
            throw std::runtime_error("File should be in storage_service_1000 at the mount point root");
        }
        if (not wrench::StorageService::lookupFileAtLocation(wrench::FileLocation::LOCATION(this->test->storage_service_510, "/disk510/foo", this->test->file_10))) {
            throw std::runtime_error("File should be in storage_service_510 at path /large_disk/foo/");
        }
        if (not wrench::StorageService::lookupFileAtLocation(wrench::FileLocation::LOCATION(this->test->storage_service_1000, "/disk1000/foo", this->test->file_10))) {
            throw std::runtime_error("File should be in storage_service_1000 at path /large_disk/foo");
        }

        // Check using hasFile
        if (not this->test->storage_service_1000->hasFile(this->test->file_10)) {
            throw std::runtime_error("File should be in storage_service_1000 at the mount point root");
        }


        // Bogus lookup
        try {
            wrench::StorageService::lookupFileAtLocation(wrench::FileLocation::LOCATION(this->test->storage_service_1000, nullptr));
            throw std::runtime_error("Should not be able to lookup a nullptr file");
        } catch (std::invalid_argument &e) {
        }

        try {
            wrench::StorageService::lookupFileAtLocation(wrench::FileLocation::LOCATION(this->test->storage_service_1000, "/disk1000", nullptr));
            throw std::runtime_error("Should not be able to lookup a nullptr file");
        } catch (std::invalid_argument &e) {
        }

        // File copy from oneself to oneself!
        try {
            data_movement_manager->initiateAsynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->storage_service_510, "/disk510/foo", this->test->file_10),
                    wrench::FileLocation::LOCATION(this->test->storage_service_510, "/disk510/faa", this->test->file_10));
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Got an unexpected exception");
        }

        // Wait for the next execution event
        try {
            event = this->waitForNextEvent();
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
        }
        if (not std::dynamic_pointer_cast<wrench::FileCopyCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        return 0;
    }
};

TEST_F(SimpleStorageServiceFunctionalTest, Partitions) {
    DO_TEST_WITH_FORK_ONE_ARG(do_Partitions_test, 1000000);
    DO_TEST_WITH_FORK_ONE_ARG(do_Partitions_test, 0);
}

void SimpleStorageServiceFunctionalTest::do_Partitions_test(sg_size_t buffer_size) {

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

    // Create 2 Storage Services
    ASSERT_NO_THROW(storage_service_1000 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk1000"},
                                                                                     {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, std::to_string(buffer_size)}}, {})));

    ASSERT_NO_THROW(storage_service_510 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk510"},
                                                                                     {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, std::to_string(buffer_size)}}, {})));


    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
                            new PartitionsTestWMS(
                                    this, hostname)));

    // Create a file registry
    simulation->add(new wrench::FileRegistryService(hostname));

    // Staging file_500 on the 1000-byte storage service
    ASSERT_NO_THROW(storage_service_1000->createFile(file_10));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  FILE WRITE TEST                                                **/
/**********************************************************************/

class FileWriteTestWMS : public wrench::ExecutionController {

public:
    FileWriteTestWMS(SimpleStorageServiceFunctionalTest *test,
                     const std::string& hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    SimpleStorageServiceFunctionalTest *test;

    int main() override {

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();


        try {
            wrench::StorageService::writeFileAtLocation(wrench::FileLocation::LOCATION(this->test->storage_service_100, "/disk100", nullptr));
            throw std::runtime_error("Should not be able to write a nullptr file to a service");
        } catch (std::invalid_argument &ignore) {
        }

        try {
            wrench::StorageService::writeFileAtLocation(wrench::FileLocation::LOCATION(this->test->storage_service_100, "/disk100", this->test->file_500));
            throw std::runtime_error("Should not be able to write to a storage service with not enough space");
        } catch (wrench::ExecutionException &e) {
            auto cause = std::dynamic_pointer_cast<wrench::StorageServiceNotEnoughSpace>(e.getCause());
            if (not cause) {
                throw std::runtime_error("Got an expected exception but unexpected cause type: " +
                                         e.getCause()->toString() + " (expected: StorageServiceNotEnoughSpace");
            }
            if (cause->getStorageService() != this->test->storage_service_100) {
                throw std::runtime_error(
                        "Got the expected 'not enough space' exception, but the failure cause does not point to the correct storage service");
            }
            if (cause->getFile() != this->test->file_500) {
                throw std::runtime_error(
                        "Got the expected 'not enough space' exception, but the failure cause does not point to the correct file");
            }
        }


        return 0;
    }
};

TEST_F(SimpleStorageServiceFunctionalTest, FileWrite) {
    DO_TEST_WITH_FORK_ONE_ARG(do_FileWrite_test, 1000000);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileWrite_test, 0);
}

void SimpleStorageServiceFunctionalTest::do_FileWrite_test(sg_size_t buffer_size) {

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

    // Create 1 Storage Services
    ASSERT_NO_THROW(storage_service_100 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk100", "/disk1000"},
                                                                                     {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, std::to_string(buffer_size)}}, {})));


    // FileLocation Testing
    ASSERT_THROW(wrench::FileLocation::LOCATION(nullptr, file_1), std::invalid_argument);
    ASSERT_THROW(wrench::FileLocation::LOCATION(storage_service_100, nullptr), std::invalid_argument);
    ASSERT_THROW(wrench::FileLocation::LOCATION(nullptr, "/disk100", file_1), std::invalid_argument);
    ASSERT_THROW(wrench::FileLocation::LOCATION(storage_service_100, "", file_1), std::invalid_argument);
    //    ASSERT_THROW(wrench::FileLocation::LOCATION(storage_service_100, "/bogus", file_1), std::invalid_argument);
    ASSERT_THROW(wrench::FileLocation::SCRATCH(file_1)->getStorageService(), std::invalid_argument);
    ASSERT_THROW(wrench::FileLocation::SCRATCH(file_1)->getDirectoryPath(), std::invalid_argument);
    ASSERT_THROW(wrench::FileLocation::sanitizePath(""), std::invalid_argument);
    //    ASSERT_THROW(wrench::FileLocation::sanitizePath(" "), std::invalid_argument);
    //    ASSERT_THROW(wrench::FileLocation::sanitizePath("../.."), std::invalid_argument);

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
                            new FileWriteTestWMS(
                                    this, hostname)));

    // Create a file registry
    simulation->add(new wrench::FileRegistryService(hostname));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
