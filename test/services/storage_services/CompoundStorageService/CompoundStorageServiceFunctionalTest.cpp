/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>
#include <vector>

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"
#include <wrench-dev.h>
#include <xbt/log.h>

WRENCH_LOG_CATEGORY(Compound_storage_service_functional_test, "Log category for CompoundStorageServiceFunctionalTest");

class CompoundStorageServiceFunctionalTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::Workflow> workflow;

    std::shared_ptr<wrench::DataFile> file_1;
    std::shared_ptr<wrench::DataFile> file_10;
    std::shared_ptr<wrench::DataFile> file_100;
    std::shared_ptr<wrench::DataFile> file_500;
    std::shared_ptr<wrench::DataFile> file_1000;

    // External storage
    std::shared_ptr<wrench::SimpleStorageService> simple_storage_service_external = nullptr;

    // Services for disks of node SimpleStorageHost0
    std::shared_ptr<wrench::SimpleStorageService> simple_storage_service_100_0 = nullptr;
    std::shared_ptr<wrench::SimpleStorageService> simple_storage_service_510_0 = nullptr;
    std::shared_ptr<wrench::SimpleStorageService> simple_storage_service_1000_0 = nullptr;
    // Services for disks of node SimpleStorageHost1
    std::shared_ptr<wrench::SimpleStorageService> simple_storage_service_100_1 = nullptr;
    std::shared_ptr<wrench::SimpleStorageService> simple_storage_service_510_1 = nullptr;
    std::shared_ptr<wrench::SimpleStorageService> simple_storage_service_1000_1 = nullptr;

    std::shared_ptr<wrench::CompoundStorageService> compound_storage_service = nullptr;

    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;

    void do_CopyToCSS_test();
    void do_WriteToCSS_test();
    void do_CopyFromCSS_test();
    void do_fullJob_test();

    void do_BasicFunctionality_test();
    void do_BasicError_test();

protected:
    ~CompoundStorageServiceFunctionalTest() {
        workflow->clear();
    }

    CompoundStorageServiceFunctionalTest() {

        workflow = wrench::Workflow::createWorkflow();

        // Create the files
        file_1 = workflow->addFile("file_1", 1.0);
        file_10 = workflow->addFile("file_10", 10.0);
        file_100 = workflow->addFile("file_100", 100.0);
        file_500 = workflow->addFile("file_500", 500.0);
        file_1000 = workflow->addFile("file_1000", 1000.0);

        // Create a three-hosts platform file (2 for simple storage, one for Compound Storage)
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"ComputeHost\" speed=\"1f\">"
                          "       </host>"
                          "       <host id=\"SimpleStorageHost0\" speed=\"1f\">"
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/disk100/\"/>"
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
                          "       <host id=\"SimpleStorageHost1\" speed=\"1f\">"
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/disk100/\"/>"
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
                          "       <host id=\"CompoundStorageHost\" speed=\"1f\">"
                          "       </host>"
                          "       <link id=\"Link1\" bandwidth=\"50MBps\" latency=\"150us\"/> "
                          "       <route src=\"CompoundStorageHost\" dst=\"SimpleStorageHost0\"><link_ctn id=\"Link1\"/></route> "
                          "       <route src=\"CompoundStorageHost\" dst=\"SimpleStorageHost1\"><link_ctn id=\"Link1\"/></route> "
                          "       <route src=\"CompoundStorageHost\" dst=\"ComputeHost\"><link_ctn id=\"Link1\"/></route> "
                          "       <route src=\"SimpleStorageHost0\" dst=\"ComputeHost\"><link_ctn id=\"Link1\"/></route> "
                          "       <route src=\"SimpleStorageHost1\" dst=\"ComputeHost\"><link_ctn id=\"Link1\"/></route> "
                          "       <route src=\"SimpleStorageHost0\" dst=\"SimpleStorageHost1\"><link_ctn id=\"Link1\"/></route> "
                          "   </zone> "
                          "</platform>";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};

class TestAllocator {

public:
    /* For testing purpose, dummy rr StorageSelectionStrategyCallback */
    std::vector<std::shared_ptr<wrench::FileLocation>> operator()(
            const std::shared_ptr<wrench::DataFile> &file,
            const std::map<std::string, std::vector<std::shared_ptr<wrench::StorageService>>> &resources,
            const std::map<std::shared_ptr<wrench::DataFile>, std::vector<std::shared_ptr<wrench::FileLocation>>> &mapping,
            const std::vector<std::shared_ptr<wrench::FileLocation>> &previous_allocations) {

        // Init round-robin
        static auto last_selected_server = resources.begin()->first;
        static auto internal_disk_selection = 0;
        // static auto call_count = 0;
        // std::cout << "# Call count 1: "<< std::to_string(call_count) << std::endl;
        auto capacity_req = file->getSize();
        std::shared_ptr<wrench::FileLocation> designated_location = nullptr;
        // std::cout << "Calling on the rrStorageSelectionStrategy for file " << file->getID() << " (" << std::to_string(file->getSize()) << "B)" << std::endl;
        auto current = resources.find(last_selected_server);
        unsigned int current_disk_selection = internal_disk_selection;
        // std::cout << "Last selected server " << last_selected_server << std::endl;
        // std::cout << "Starting from server " << current->first << std::endl;
        // std::cout << "Internal disk selection " << std::to_string(internal_disk_selection) << std::endl;

        auto continue_disk_loop = true;

        do {

            // std::cout << "Considering disk index " << std::to_string(current_disk_selection) << std::endl;
            auto nb_of_local_disks = current->second.size();
            auto storage_service = current->second[current_disk_selection % nb_of_local_disks];
            // std::cout << "- Looking at storage service " << storage_service->getName() << std::endl;

            auto free_space = storage_service->getTotalFreeSpace();
            // std::cout << "- It has " << free_space << "B of free space" << std::endl;

            if (free_space >= capacity_req) {
                designated_location = wrench::FileLocation::LOCATION(std::shared_ptr<wrench::StorageService>(storage_service), file);
                // std::cout << "Chose server " << current->first << storage_service->getBaseRootPath() << std::endl;
                // Update for next function call
                std::advance(current, 1);
                if (current == resources.end()) {
                    current = resources.begin();
                    current_disk_selection++;
                }
                last_selected_server = current->first;
                internal_disk_selection = current_disk_selection;
                // std::cout << "Next first server will be " << last_selected_server << std::endl;
                break;
            }

            std::advance(current, 1);
            if (current == resources.end()) {
                current = resources.begin();
                current_disk_selection++;
            }
            if (current_disk_selection > (internal_disk_selection + nb_of_local_disks + 1)) {
                // std::cout << "Stopping continue_disk_loop" << std::endl;
                continue_disk_loop = false;
            }
            // std::cout << "Next server will be " << current->first << std::endl;
        } while ((current->first != last_selected_server) or (continue_disk_loop));

        // call_count++;
        // std::cout << "# Call count 2: "<< std::to_string(call_count) << std::endl;

        // std::cout << "smartStorageSelectionStrategy has done its work." << std::endl;
        std::vector<std::shared_ptr<wrench::FileLocation>> ret = {};
        if (designated_location) {
            ret.push_back(designated_location);
        }
        return ret;
    }
};

/**********************************************************************/
/**  COPY TO CSS TEST                                                **/
/**********************************************************************/

class CSSCopyToCSSTestCtrl : public wrench::ExecutionController {

public:
    CSSCopyToCSSTestCtrl(CompoundStorageServiceFunctionalTest *test,
                         const std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    CompoundStorageServiceFunctionalTest *test;

    int main() override {

        wrench::S4U_Simulation::computeZeroFlop();

        auto job_manager = this->createJobManager();
        auto job = job_manager->createCompoundJob("copyToCSSJob");

        // ## Stage file on external storage
        wrench::StorageService::createFileAtLocation(wrench::FileLocation::LOCATION(test->simple_storage_service_external, "/disk1000", test->file_100));
        // Copy to CSS
        auto fileCopyAction = job->addFileCopyAction(
                "stagingCopy_1",
                wrench::FileLocation::LOCATION(test->simple_storage_service_external, "/disk1000", test->file_100),
                wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_100));

        // Read from CSS
        auto fileReadAction = job->addFileReadAction("fRead_1", wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_100));
        job->addActionDependency(fileCopyAction, fileReadAction);

        // ## Stage file on external storage (WITH STRIPPING)
        wrench::StorageService::createFileAtLocation(wrench::FileLocation::LOCATION(test->simple_storage_service_external, "/disk1000", test->file_500));
        // Copy to CSS
        auto fileCopyAction_2 = job->addFileCopyAction(
                "stagingCopy_2",
                wrench::FileLocation::LOCATION(test->simple_storage_service_external, "/disk1000", test->file_500),
                wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_500));
        job->addActionDependency(fileReadAction, fileCopyAction_2);

        // Read from CSS
        auto fileReadAction_2 = job->addFileReadAction("fRead_2", wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_500));
        job->addActionDependency(fileCopyAction_2, fileReadAction_2);

        job_manager->submitJob(job, test->compute_service, {});
        std::shared_ptr<wrench::ExecutionEvent> event = this->waitForNextEvent();

        if (not std::dynamic_pointer_cast<wrench::CompoundJobCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        if (job->getState() != wrench::CompoundJob::State::COMPLETED) {
            throw std::runtime_error("Unexpected job state: " + job->getStateAsString());
        }

        // Check that all file copies worked as intended
        auto tmp_mailbox = wrench::S4U_Mailbox::getTemporaryMailbox();
        auto read_file_copy_1 = test->compound_storage_service->lookupFileLocation(test->file_100, tmp_mailbox);
        if (read_file_copy_1.size() != 1) {
            throw std::runtime_error("Lookup returned an incorrect number of parts for file_100 on CSS");
        }
        if (read_file_copy_1[0]->getStorageService()->getBaseRootPath() != "/disk510/") {
            throw std::runtime_error("file_100 should be on /disk510/");
        }

        auto read_file_copy_2 = test->compound_storage_service->lookupFileLocation(test->file_500, tmp_mailbox);
        if (read_file_copy_2.size() != 2) {
            throw std::runtime_error("Lookup returned an incorrect number of parts for file_500 on CSS");
        }
        for (auto part: read_file_copy_2) {
            auto file_location = wrench::FileLocation::LOCATION(test->simple_storage_service_external, part->getFile());
            if (test->simple_storage_service_external->hasFile(file_location)) {
                throw std::runtime_error("Src file part from stripping shouldn't be on source anymore");
            }
        }
        if (read_file_copy_2[0]->getStorageService()->getBaseRootPath() != "/disk1000/") {
            throw std::runtime_error("file_500_part_0 should be on /disk1000/");
        }
        if (read_file_copy_2[1]->getStorageService()->getBaseRootPath() != "/disk510/") {
            throw std::runtime_error("file_500_part_1 should be on /disk510/");
        }

        auto external_free_space = test->simple_storage_service_external->getTotalFreeSpace();
        if (external_free_space != 400) {
            throw std::runtime_error("Residual data on external free space not cleaned up after stropped copy");
        }
        wrench::S4U_Mailbox::retireTemporaryMailbox(tmp_mailbox);

        return 0;
    }
};

TEST_F(CompoundStorageServiceFunctionalTest, BasicFunctionalityCopyToCSS) {
    DO_TEST_WITH_FORK(do_CopyToCSS_test);
}

void CompoundStorageServiceFunctionalTest::do_CopyToCSS_test() {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

    // xbt_log_control_set("wrench_core_file_transfer_thread.thres:info");
    // xbt_log_control_set("wrench_core_compound_storage_system.thresh:debug");
    // xbt_log_control_set("wrench_core_file_transfer_thread.thres:info");
    // xbt_log_control_set("wrench_core_storage_service.thres:debug");

    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    // argv[1] = strdup("--wrench-full-log");

    // Setting up simulation and platform
    ASSERT_NO_THROW(simulation->init(&argc, argv));
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    ASSERT_NO_THROW(
            compute_service = simulation->add(
                    new wrench::BareMetalComputeService("ComputeHost",
                                                        {std::make_pair("ComputeHost", std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                       wrench::ComputeService::ALL_RAM))},
                                                        {})));

    // Create some simple storage services (unbufferized)
    ASSERT_NO_THROW(simple_storage_service_510_0 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("SimpleStorageHost0", {"/disk510"},
                                                                                     {}, {})));
    ASSERT_NO_THROW(simple_storage_service_1000_0 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("SimpleStorageHost0", {"/disk1000"},
                                                                                     {}, {})));

    ASSERT_NO_THROW(simple_storage_service_external = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("SimpleStorageHost1", {"/disk1000"},
                                                                                     {}, {})));

    // Create a valid Compound Storage Service, without user provided callback (no intercept capabilities)
    // std::shared_ptr<wrench::StorageAllocator> allocator = std::make_shared<TestAllocator>();

    TestAllocator allocator;
    wrench::StorageSelectionStrategyCallback allocatorCallback = [&allocator](
                                                                         const std::shared_ptr<wrench::DataFile> &file,
                                                                         const std::map<std::string, std::vector<std::shared_ptr<wrench::StorageService>>> &resources,
                                                                         const std::map<std::shared_ptr<wrench::DataFile>, std::vector<std::shared_ptr<wrench::FileLocation>>> &mapping,
                                                                         const std::vector<std::shared_ptr<wrench::FileLocation>> &previous_allocations) {
        return allocator(file, resources, mapping, previous_allocations);
    };
    ASSERT_NO_THROW(compound_storage_service = simulation->add(
                            new wrench::CompoundStorageService(
                                    "CompoundStorageHost",
                                    {simple_storage_service_510_0, simple_storage_service_1000_0},
                                    allocatorCallback,
                                    {{wrench::CompoundStorageServiceProperty::MAX_ALLOCATION_CHUNK_SIZE, "400"}}, {})));

    // Create a Controler
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
                            new CSSCopyToCSSTestCtrl(this, "CompoundStorageHost")));

    // Tun the simulation
    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  WRITE TO CSS TEST                                                **/
/**********************************************************************/

class CSSWriteToCSSTestCtrl : public wrench::ExecutionController {

public:
    CSSWriteToCSSTestCtrl(CompoundStorageServiceFunctionalTest *test,
                          const std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    CompoundStorageServiceFunctionalTest *test;

    int main() override {

        wrench::S4U_Simulation::computeZeroFlop();

        auto job_manager = this->createJobManager();
        auto job = job_manager->createCompoundJob("writeToCSSJob");

        // Write to CSS
        auto fileWriteAction_1 = job->addFileWriteAction("fWrite_1", wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_100));

        // Write to CSS (with stripping)
        auto fileWriteAction_2 = job->addFileWriteAction("fWrite_2", wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_500));
        job->addActionDependency(fileWriteAction_1, fileWriteAction_2);

        job_manager->submitJob(job, test->compute_service, {});
        std::shared_ptr<wrench::ExecutionEvent> event = this->waitForNextEvent();

        if (not std::dynamic_pointer_cast<wrench::CompoundJobCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        if (job->getState() != wrench::CompoundJob::State::COMPLETED) {
            throw std::runtime_error("Unexpected job state: " + job->getStateAsString());
        }

        auto tmp_mailbox = wrench::S4U_Mailbox::getTemporaryMailbox();
        // Check that all file copies worked as intended
        auto write_file_lookup_1 = test->compound_storage_service->lookupFileLocation(test->file_100, tmp_mailbox);
        if (write_file_lookup_1.size() != 1) {
            throw std::runtime_error("Lookup returned an incorrect number of parts for file_500 on CSS");
        }
        if (write_file_lookup_1[0]->getStorageService()->getBaseRootPath() != "/disk510/") {
            throw std::runtime_error("file_100 should be on /disk510/");
        }

        auto write_file_lookup_2 = test->compound_storage_service->lookupFileLocation(test->file_500, tmp_mailbox);
        if (write_file_lookup_2.size() != 2) {
            throw std::runtime_error("Lookup returned an incorrect number of parts for file_500 on CSS");
        }
        if (write_file_lookup_2[0]->getStorageService()->getBaseRootPath() != "/disk1000/") {
            throw std::runtime_error("file_500_part_0 should be on /disk1000/");
        }
        if (write_file_lookup_2[1]->getStorageService()->getBaseRootPath() != "/disk510/") {
            throw std::runtime_error("file_500_part_1 should be on /disk510/");
        }

        wrench::S4U_Mailbox::retireTemporaryMailbox(tmp_mailbox);

        return 0;
    }
};

TEST_F(CompoundStorageServiceFunctionalTest, BasicFunctionalityWriteToCSS) {
    DO_TEST_WITH_FORK(do_WriteToCSS_test);
}

void CompoundStorageServiceFunctionalTest::do_WriteToCSS_test() {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

    // xbt_log_control_set("wrench_core_storage_service.thres:debug");
    // xbt_log_control_set("wrench_core_file_transfer_thread.thres:info");
    // xbt_log_control_set("wrench_core_compound_storage_system.thresh:debug");
    // xbt_log_control_set("wrench_core_file_transfer_thread.thres:info");

    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    // argv[1] = strdup("--wrench-full-log");

    // Setting up simulation and platform
    ASSERT_NO_THROW(simulation->init(&argc, argv));
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    ASSERT_NO_THROW(
            compute_service = simulation->add(
                    new wrench::BareMetalComputeService("ComputeHost",
                                                        {std::make_pair("ComputeHost", std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                       wrench::ComputeService::ALL_RAM))},
                                                        {})));

    // Create some simple storage services (unbufferized)
    ASSERT_NO_THROW(simple_storage_service_510_0 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("SimpleStorageHost0", {"/disk510"},
                                                                                     {}, {})));
    ASSERT_NO_THROW(simple_storage_service_1000_0 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("SimpleStorageHost0", {"/disk1000"},
                                                                                     {}, {})));

    // Create a valid Compound Storage Service, without user provided callback (no intercept capabilities)
    TestAllocator allocator;
    wrench::StorageSelectionStrategyCallback allocatorCallback = [&allocator](
                                                                         const std::shared_ptr<wrench::DataFile> &file,
                                                                         const std::map<std::string, std::vector<std::shared_ptr<wrench::StorageService>>> &resources,
                                                                         const std::map<std::shared_ptr<wrench::DataFile>, std::vector<std::shared_ptr<wrench::FileLocation>>> &mapping,
                                                                         const std::vector<std::shared_ptr<wrench::FileLocation>> &previous_allocations) {
        return allocator(file, resources, mapping, previous_allocations);
    };
    ASSERT_NO_THROW(compound_storage_service = simulation->add(
                            new wrench::CompoundStorageService(
                                    "CompoundStorageHost",
                                    {simple_storage_service_510_0, simple_storage_service_1000_0},
                                    allocatorCallback,
                                    {{wrench::CompoundStorageServiceProperty::MAX_ALLOCATION_CHUNK_SIZE, "400"}}, {})));

    // Create a Controler
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
                            new CSSWriteToCSSTestCtrl(this, "CompoundStorageHost")));

    // Tun the simulation
    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  COPY FROM CSS TEST                                                **/
/**********************************************************************/

class CSSCopyFromCSSTestCtrl : public wrench::ExecutionController {

public:
    CSSCopyFromCSSTestCtrl(CompoundStorageServiceFunctionalTest *test,
                           const std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    CompoundStorageServiceFunctionalTest *test;

    int main() override {

        wrench::S4U_Simulation::computeZeroFlop();

        auto job_manager = this->createJobManager();
        auto job = job_manager->createCompoundJob("copyFromCSSJob");

        // Write to CSS
        auto fileWriteAction_1 = job->addFileWriteAction("fWrite_1", wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_100));
        // Copy to external storage
        auto fileCopyAction_1 = job->addFileCopyAction("fCopy_1",
                                                       wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_100),
                                                       wrench::FileLocation::LOCATION(test->simple_storage_service_external, "/disk1000", test->file_100));
        job->addActionDependency(fileWriteAction_1, fileCopyAction_1);

        // Write to CSS
        auto fileWriteAction_2 = job->addFileWriteAction("fWrite_2", wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_500));
        job->addActionDependency(fileCopyAction_1, fileWriteAction_2);
        // Copy to external storage (with stripping)
        auto fileCopyAction_2 = job->addFileCopyAction("fCopy_2",
                                                       wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_500),
                                                       wrench::FileLocation::LOCATION(test->simple_storage_service_external, "/disk1000", test->file_500));
        job->addActionDependency(fileWriteAction_2, fileCopyAction_2);

        job_manager->submitJob(job, test->compute_service, {});
        std::shared_ptr<wrench::ExecutionEvent> event = this->waitForNextEvent();

        if (not std::dynamic_pointer_cast<wrench::CompoundJobCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        if (job->getState() != wrench::CompoundJob::State::COMPLETED) {
            throw std::runtime_error("Unexpected job state: " + job->getStateAsString());
        }

        // Check that all file copies worked as intended
        auto tmp_mailbox = wrench::S4U_Mailbox::getTemporaryMailbox();

        auto write_file_lookup_1 = test->compound_storage_service->lookupFileLocation(test->file_100, tmp_mailbox);
        if (write_file_lookup_1.size() != 1) {
            throw std::runtime_error("Lookup returned an incorrect number of parts for file_500 on CSS");
        }
        if (write_file_lookup_1[0]->getStorageService()->getBaseRootPath() != "/disk510/") {
            throw std::runtime_error("file_100 should be on /disk510/");
        }

        if (!test->simple_storage_service_external->hasFile(wrench::FileLocation::LOCATION(test->simple_storage_service_external, "/disk1000", test->file_500))) {
            throw std::runtime_error("File 500 not found on external storage");
        }
        if (!test->simple_storage_service_external->hasFile(wrench::FileLocation::LOCATION(test->simple_storage_service_external, "/disk1000", test->file_100))) {
            throw std::runtime_error("File 100 not found on external storage");
        }
        if (test->simple_storage_service_external->getTotalFreeSpace() != 400) {
            throw std::runtime_error("External storage doesn't have the expected free space (should be 400)");
        }

        auto write_file_lookup_2 = test->compound_storage_service->lookupFileLocation(test->file_500, tmp_mailbox);
        if (write_file_lookup_2.size() != 2) {
            throw std::runtime_error("Lookup returned an incorrect number of parts for file_500 on CSS");
        }
        if (write_file_lookup_2[0]->getStorageService()->getBaseRootPath() != "/disk1000/") {
            throw std::runtime_error("file_500_part_0 should be on /disk1000/");
        }
        if (write_file_lookup_2[1]->getStorageService()->getBaseRootPath() != "/disk510/") {
            throw std::runtime_error("file_500_part_1 should be on /disk510/");
        }

        wrench::S4U_Mailbox::retireTemporaryMailbox(tmp_mailbox);

        return 0;
    }
};

TEST_F(CompoundStorageServiceFunctionalTest, BasicFunctionalityCopyFromCSS) {
    DO_TEST_WITH_FORK(do_CopyFromCSS_test);
}

void CompoundStorageServiceFunctionalTest::do_CopyFromCSS_test() {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

    // xbt_log_control_set("wrench_core_storage_service.thres:debug");
    // xbt_log_control_set("wrench_core_file_transfer_thread.thres:info");
    // xbt_log_control_set("wrench_core_compound_storage_system.thresh:debug");
    // xbt_log_control_set("wrench_core_file_transfer_thread.thres:info");

    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    // argv[1] = strdup("--wrench-full-log");

    // Setting up simulation and platform
    ASSERT_NO_THROW(simulation->init(&argc, argv));
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    ASSERT_NO_THROW(
            compute_service = simulation->add(
                    new wrench::BareMetalComputeService("ComputeHost",
                                                        {std::make_pair("ComputeHost", std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                       wrench::ComputeService::ALL_RAM))},
                                                        {})));

    // Create some simple storage services (unbufferized)
    ASSERT_NO_THROW(simple_storage_service_510_0 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("SimpleStorageHost0", {"/disk510"},
                                                                                     {}, {})));
    ASSERT_NO_THROW(simple_storage_service_1000_0 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("SimpleStorageHost0", {"/disk1000"},
                                                                                     {}, {})));
    ASSERT_NO_THROW(simple_storage_service_external = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("SimpleStorageHost1", {"/disk1000"},
                                                                                     {}, {})));

    // Create a valid Compound Storage Service, without user provided callback (no intercept capabilities)
    TestAllocator allocator;
    wrench::StorageSelectionStrategyCallback allocatorCallback = [&allocator](
                                                                         const std::shared_ptr<wrench::DataFile> &file,
                                                                         const std::map<std::string, std::vector<std::shared_ptr<wrench::StorageService>>> &resources,
                                                                         const std::map<std::shared_ptr<wrench::DataFile>, std::vector<std::shared_ptr<wrench::FileLocation>>> &mapping,
                                                                         const std::vector<std::shared_ptr<wrench::FileLocation>> &previous_allocations) {
        return allocator(file, resources, mapping, previous_allocations);
    };
    ASSERT_NO_THROW(compound_storage_service = simulation->add(
                            new wrench::CompoundStorageService(
                                    "CompoundStorageHost",
                                    {simple_storage_service_510_0, simple_storage_service_1000_0},
                                    allocatorCallback,
                                    {{wrench::CompoundStorageServiceProperty::MAX_ALLOCATION_CHUNK_SIZE, "400"}}, {})));

    // Create a Controler
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
                            new CSSCopyFromCSSTestCtrl(this, "CompoundStorageHost")));

    // Tun the simulation
    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  FULL JOB TEST                                                **/
/**********************************************************************/

class CSSFullJobTestCtrl : public wrench::ExecutionController {

public:
    CSSFullJobTestCtrl(CompoundStorageServiceFunctionalTest *test,
                       const std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    CompoundStorageServiceFunctionalTest *test;

    int main() override {

        /**
         *  - Create a file on external storage
         *  - Copy from external storage to CSS storage (stripping on dest)
         *  - Read parts of file from CSS
         *  - Write results to CSS (stripped)
         *  - Delete original file from external storage
         *  - Archive results from CSS (stripped)  to external storage
         *  - Delete all files from CSS
         */

        wrench::S4U_Simulation::computeZeroFlop();
        std::vector<std::shared_ptr<wrench::Action>> actions;

        auto job_manager = this->createJobManager();
        auto job = job_manager->createCompoundJob("fullJob");

        wrench::StorageService::createFileAtLocation(wrench::FileLocation::LOCATION(test->simple_storage_service_external, "/disk1000", test->file_1000));
        auto stagingAction = job->addFileCopyAction(
                "stagingCopy_1000",
                wrench::FileLocation::LOCATION(test->simple_storage_service_external, test->file_1000),
                wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_1000));
        actions.push_back(stagingAction);
        auto fileReadAction = job->addFileReadAction("fRead_1000", wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_1000));
        actions.push_back(fileReadAction);
        job->addActionDependency(stagingAction, fileReadAction);
        // Compute would go here

        auto fileWriteAction = job->addFileWriteAction("fWrite_500", wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_500));
        actions.push_back(fileWriteAction);
        job->addActionDependency(fileReadAction, fileWriteAction);

        // Free some space on the external storage
        auto deleteExternalFileAction = job->addFileDeleteAction("delExt_1000", wrench::FileLocation::LOCATION(test->simple_storage_service_external, test->file_1000));
        actions.push_back(deleteExternalFileAction);
        job->addActionDependency(fileWriteAction, deleteExternalFileAction);

        // "Archive" hypothetical results to external storage
        auto archiveAction = job->addFileCopyAction(
                "archiveCopy_500",
                wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_500),
                wrench::FileLocation::LOCATION(test->simple_storage_service_external, test->file_500));
        actions.push_back(archiveAction);
        job->addActionDependency(deleteExternalFileAction, archiveAction);

        // Cleanup CSS (initial file 1000 and written file 500)
        auto deleteCSS1000Action = job->addFileDeleteAction("delRF_1000", wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_1000));
        actions.push_back(deleteCSS1000Action);
        job->addActionDependency(archiveAction, deleteCSS1000Action);

        auto deleteCSS500Action = job->addFileDeleteAction("delRF_500", wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_500));
        actions.push_back(deleteCSS500Action);
        job->addActionDependency(deleteCSS1000Action, deleteCSS500Action);

        job_manager->submitJob(job, test->compute_service);
        std::shared_ptr<wrench::ExecutionEvent> event = this->waitForNextEvent();

        for (auto const &a: actions) {
            if (a->getState() != wrench::Action::State::COMPLETED) {
                auto exc_msg = "Action " + a->getName() + " did not complete";
                throw std::runtime_error(exc_msg);
            }
        }

        if (!test->simple_storage_service_external->hasFile(wrench::FileLocation::LOCATION(test->simple_storage_service_external, "/disk1000", test->file_500))) {
            throw std::runtime_error("File 500 not found on external storage");
        }

        auto external_free_space = test->simple_storage_service_external->getTotalFreeSpace();
        if (external_free_space != 500) {
            auto exc_msg = "External free space has " + std::to_string(external_free_space) + "B of free space."
                                                                                              "It should have 500B";
            throw std::runtime_error(exc_msg);
        }

        auto tmp_mailbox = wrench::S4U_Mailbox::getTemporaryMailbox();
        auto css_File_500 = test->compound_storage_service->lookupFileLocation(test->file_500, tmp_mailbox);
        if (!css_File_500.empty()) {
            throw std::runtime_error("file_500 is still present on CSS, it shouldn't");
        }

        wrench::S4U_Mailbox::retireTemporaryMailbox(tmp_mailbox);

        return 0;
    }
};

TEST_F(CompoundStorageServiceFunctionalTest, BasicFunctionalityFullJob) {
    DO_TEST_WITH_FORK(do_fullJob_test);
}

void CompoundStorageServiceFunctionalTest::do_fullJob_test() {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

    // xbt_log_control_set("wrench_core_storage_service.thres:debug");
    // xbt_log_control_set("wrench_core_file_transfer_thread.thres:info");
    // xbt_log_control_set("wrench_core_compound_storage_system.thresh:debug");
    // xbt_log_control_set("wrench_core_file_transfer_thread.thres:info");

    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    // argv[1] = strdup("--wrench-full-log");

    // Setting up simulation and platform
    ASSERT_NO_THROW(simulation->init(&argc, argv));
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    ASSERT_NO_THROW(
            compute_service = simulation->add(
                    new wrench::BareMetalComputeService("ComputeHost",
                                                        {std::make_pair("ComputeHost", std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                       wrench::ComputeService::ALL_RAM))},
                                                        {})));

    // Create some simple storage services (unbufferized)
    ASSERT_NO_THROW(simple_storage_service_510_0 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("SimpleStorageHost0", {"/disk510"},
                                                                                     {}, {})));
    ASSERT_NO_THROW(simple_storage_service_1000_0 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("SimpleStorageHost0", {"/disk1000"},
                                                                                     {}, {})));
    ASSERT_NO_THROW(simple_storage_service_100_1 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("SimpleStorageHost1", {"/disk100"},
                                                                                     {}, {})));

    ASSERT_NO_THROW(simple_storage_service_external = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("SimpleStorageHost1", {"/disk1000"},
                                                                                     {}, {})));

    // Create a valid Compound Storage Service, without user provided callback (no intercept capabilities)
    TestAllocator allocator;
    wrench::StorageSelectionStrategyCallback allocatorCallback = [&allocator](
                                                                         const std::shared_ptr<wrench::DataFile> &file,
                                                                         const std::map<std::string, std::vector<std::shared_ptr<wrench::StorageService>>> &resources,
                                                                         const std::map<std::shared_ptr<wrench::DataFile>, std::vector<std::shared_ptr<wrench::FileLocation>>> &mapping,
                                                                         const std::vector<std::shared_ptr<wrench::FileLocation>> &previous_allocations) {
        return allocator(file, resources, mapping, previous_allocations);
    };
    ASSERT_NO_THROW(compound_storage_service = simulation->add(
                            new wrench::CompoundStorageService(
                                    "CompoundStorageHost",
                                    {simple_storage_service_510_0, simple_storage_service_100_1, simple_storage_service_1000_0},
                                    allocatorCallback,
                                    {{wrench::CompoundStorageServiceProperty::MAX_ALLOCATION_CHUNK_SIZE, "100"}}, {})));

    // Create a Controler
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
                            new CSSFullJobTestCtrl(this, "CompoundStorageHost")));

    // Tun the simulation
    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  BASIC FUNCTIONALITY SIMULATION TEST                             **/
/**********************************************************************/

class CompoundStorageServiceBasicFunctionalityTestCtrl : public wrench::ExecutionController {

public:
    CompoundStorageServiceBasicFunctionalityTestCtrl(CompoundStorageServiceFunctionalTest *test,
                                                     const std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    CompoundStorageServiceFunctionalTest *test;

    int main() override {

        wrench::S4U_Simulation::computeZeroFlop();

        // Retrieve internal SimpleStorageServices
        auto simple_storage_services = test->compound_storage_service->getAllServices();
        std::map<std::string, std::vector<std::shared_ptr<wrench::StorageService>>> expected_services{
                {"SimpleStorageHost0", {test->simple_storage_service_100_0}},
                {"SimpleStorageHost1", {test->simple_storage_service_510_1}}};

        for (auto const &svc: simple_storage_services) {
            if (expected_services.find(svc.first) == expected_services.end()) {
                throw std::runtime_error("Can't find one of the hosts in the services list");
            }
            if (expected_services[svc.first] != svc.second) {
                throw std::runtime_error("One of the Storage hosts has a different list of associated disk services than expected");
            }
        }

        // Verify synchronous request for current free space (currently same as capacity, as no file has been placed on internal services)
        auto expected_capacity = 100.0 + 510.0;
        auto free_space = test->compound_storage_service->getTotalFreeSpace();
        if (free_space != expected_capacity) {
            throw std::runtime_error("'Free Space' available to CompoundStorageService is incorrect");
        }
        // Verify that total space is correct
        auto capacity = test->compound_storage_service->getTotalSpace();
        if (capacity != expected_capacity) {
            throw std::runtime_error("'Total Space' available to CompoundStorageService is incorrect");
        }

        // Verify that compound storage service unique mount point is DEV_NULL
        //        auto mount_point = test->compound_storage_service->getMountPoint();
        //        if (mount_point != wrench::LogicalFileSystem::DEV_NULL + "/") {
        //            throw std::runtime_error("CompoundStorageService should have only one 'LogicalFileSystem::DEV_NULL' filesystem");
        //        }

        // We don't support getLoad or getFileLastWriteDate on CompoundStorageService yet (and won't ?)
        try {
            test->compound_storage_service->getLoad();
            throw std::runtime_error("CompoundStorageService doesn't have a getLoad() implemented");
        } catch (std::logic_error &e) {
        }

        {
            auto file_1_loc = wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_1);
            try {
                test->compound_storage_service->getFileLastWriteDate(file_1_loc);
                throw std::runtime_error("We shouldn't be able to get a FileLastWriteDate on a file that wasn't written to the CSS first");
            } catch (std::invalid_argument &e) {
            }
        }

        // CompoundStorageServer should never be a scratch space (at init or set as later)
        if (test->compound_storage_service->isScratch() == true) {
            throw std::runtime_error("CompoundStorageService should never have isScratch == true");
        }

        try {
            test->compound_storage_service->setIsScratch(true);
            throw std::runtime_error("CompoundStorageService can't be setup as a scratch space");
        } catch (std::logic_error &e) {
        }

        if (test->compound_storage_service->isBufferized()) {
            throw std::runtime_error("CompoundStorageService shouldn't be bufferized");
        }
        // ## Test multiple messages that should answer with a failure cause, and in turn generate an ExecutionException
        // on caller's side

        // File copy, CSS as src, file is not known by CSS:
        {
            auto file_1_loc_ss = wrench::FileLocation::LOCATION(test->simple_storage_service_100_0, test->file_1);
            wrench::StorageService::createFileAtLocation(file_1_loc_ss);
            auto file_1_loc_css = wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_1);
            try {
                wrench::StorageService::copyFile(file_1_loc_css, file_1_loc_ss);
                throw std::runtime_error("File doesn't exist on CSS, copy with CSS as src should be impossible");
            } catch (wrench::ExecutionException &e) {
                if (!std::dynamic_pointer_cast<wrench::FileNotFound>(e.getCause())) {
                    throw std::runtime_error("Exception cause should have been 'FileNotFound'");
                }
            }
            test->simple_storage_service_100_0->deleteFile(file_1_loc_ss);
        }

        // File copy, CSS as dst, file can't be allocated (no callback provided, and the default one doesn't return anything) - src file exists:
        {
            auto file_1_loc_ss = wrench::FileLocation::LOCATION(test->simple_storage_service_100_0, test->file_1);
            wrench::StorageService::createFileAtLocation(file_1_loc_ss);
            auto file_1_loc_css = wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_1);
            try {
                wrench::StorageService::copyFile(file_1_loc_ss, file_1_loc_css);
                throw std::runtime_error("File doesn't exist and can't be allocated on CSS, copy with CSS as dst should be impossible");
            } catch (wrench::ExecutionException &e) {
                if (!std::dynamic_pointer_cast<wrench::StorageServiceNotEnoughSpace>(e.getCause())) {
                    throw std::runtime_error("Exception cause should have been 'StorageServiceNotEnoughSpace' (which is slightly incorrect in this case)");
                }
            }
            test->simple_storage_service_100_0->deleteFile(file_1_loc_ss);
        }

        auto file_1_loc_ss = wrench::FileLocation::LOCATION(test->simple_storage_service_100_0, test->file_1);
        auto file_1_loc_css = wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_1);
        try {
            wrench::StorageService::deleteFileAtLocation(file_1_loc_css);
            throw std::runtime_error("Should not be able to delete file from a CompoundStorageService if it has not first been written / copied to it");
        } catch (wrench::ExecutionException &e) {
            if (!std::dynamic_pointer_cast<wrench::FileNotFound>(e.getCause())) {
                throw std::runtime_error("Exception cause should have been 'FileNotFound'");
            }
        }

        try {
            wrench::StorageService::readFileAtLocation(file_1_loc_css);
            throw std::runtime_error("Should not be able to read file from a CompoundStorageService if it has not first been written / copied to it");
        } catch (wrench::ExecutionException &e) {
            if (!std::dynamic_pointer_cast<wrench::FileNotFound>(e.getCause())) {
                throw std::runtime_error("Exception cause should have been 'FileNotFound'");
            }
        }

        try {
            wrench::StorageService::writeFileAtLocation(file_1_loc_css);
            throw std::runtime_error("Should not be able to write file on a CompoundStorageService because no selection callback was provided");
        } catch (wrench::ExecutionException &e) {
            if (!std::dynamic_pointer_cast<wrench::StorageServiceNotEnoughSpace>(e.getCause())) {
                throw std::runtime_error("Exception cause should have been 'StorageServiceNotEnoughSpace' (which is slightly incorrect in this case)");
            }
        }

        // This one simply answers that the file was not found
        if (wrench::StorageService::lookupFileAtLocation(file_1_loc_css))
            throw std::runtime_error("Should not be able to lookup file from a CompoundStorageService if it has not been written/copied to it first");

        return 0;
    }
};

TEST_F(CompoundStorageServiceFunctionalTest, BasicFunctionality) {
    DO_TEST_WITH_FORK(do_BasicFunctionality_test);
}

void CompoundStorageServiceFunctionalTest::do_BasicFunctionality_test() {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

    // xbt_log_control_set("wrench_core_storage_service.thres:debug");
    // xbt_log_control_set("wrench_core_file_transfer_thread.thres:info");
    // xbt_log_control_set("wrench_core_compound_storage_system.thresh:debug");
    // xbt_log_control_set("wrench_core_file_transfer_thread.thres:info");

    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    // argv[1] = strdup("--wrench-full-log");

    // Setting up simulation and platform
    ASSERT_NO_THROW(simulation->init(&argc, argv));
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a Compute Service
    ASSERT_NO_THROW(
            compute_service = simulation->add(
                    new wrench::BareMetalComputeService("ComputeHost",
                                                        {std::make_pair("ComputeHost", std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                       wrench::ComputeService::ALL_RAM))},
                                                        {})));

    // Create some simple storage services
    // Unbufferized
    ASSERT_NO_THROW(simple_storage_service_100_0 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("SimpleStorageHost0", {"/disk100"},
                                                                                     {}, {})));
    // Bufferized
    ASSERT_NO_THROW(simple_storage_service_510_1 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("SimpleStorageHost1", {"/disk510"},
                                                                                     {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "1000000"}}, {})));

    // Fail to create a Compound Storage Service (no storage services provided)
    ASSERT_THROW(compound_storage_service = simulation->add(
                         new wrench::CompoundStorageService("CompoundStorageHost", {})),
                 std::invalid_argument);

    // Fail to create a Compound Storage Service (one of the storage service is just a nullptr)
    ASSERT_THROW(compound_storage_service = simulation->add(
                         new wrench::CompoundStorageService(
                                 "CompoundStorageHost",
                                 {simple_storage_service_1000_0, simple_storage_service_100_0})),
                 std::invalid_argument);

    // Create a valid Compound Storage Service, without user provided callback
    TestAllocator allocator;
    ASSERT_NO_THROW(compound_storage_service = simulation->add(
                            new wrench::CompoundStorageService("CompoundStorageHost",
                                                               {simple_storage_service_100_0, simple_storage_service_510_1},
                                                               {{wrench::CompoundStorageServiceProperty::MAX_ALLOCATION_CHUNK_SIZE, "100"},
                                                                {wrench::CompoundStorageServiceProperty::INTERNAL_STRIPING, "true"}})));

    // Create a Controler
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
                            new CompoundStorageServiceBasicFunctionalityTestCtrl(this, "CompoundStorageHost")));

    // A bogus staging (can't use CompoundStorageService for staging)
    ASSERT_THROW(simulation->stageFile(file_10, compound_storage_service), std::invalid_argument);

    // Tun the simulation
    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  BASIC ERROR CASE  SIMULATION TEST                   **/
/**********************************************************************/

class CompoundStorageServiceErrorTestCtrl : public wrench::ExecutionController {

public:
    CompoundStorageServiceErrorTestCtrl(CompoundStorageServiceFunctionalTest *test,
                                        std::string hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    CompoundStorageServiceFunctionalTest *test;

    int main() override {

        auto job_manager = this->createJobManager();

        // 1 - Copy from CSS to SS, using a file that was not created on CSS beforehand
        auto jobCopyError = job_manager->createCompoundJob("jobCopyError");
        auto fileCopyActionCSS_SS = jobCopyError->addFileCopyAction(
                "fileCopySrcCSS_DstSS",
                wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_500),
                wrench::FileLocation::LOCATION(test->simple_storage_service_1000_0, test->file_500));

        job_manager->submitJob(jobCopyError, test->compute_service, {});

        // Dirty solution to make all tests run : normally we would stop simulation after the
        // first error, and wouldn't care about what happens to following jobs, but here we need to
        // give some time to each job to "properly" fail
        simulation->sleep(10000);

        // 2 - Copy from SS to CSS, using a file that is too big to be allocated
        auto jobCopySizeError = job_manager->createCompoundJob("jobCopySizeError");
        wrench::StorageService::createFileAtLocation(wrench::FileLocation::LOCATION(test->simple_storage_service_1000_0, test->file_1000));
        auto fileCopyActionSS_CSS = jobCopySizeError->addFileCopyAction(
                "fileCopySrcSS_DstCSS",
                wrench::FileLocation::LOCATION(test->simple_storage_service_1000_0, test->file_1000),
                wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_1000));

        job_manager->submitJob(jobCopySizeError, test->compute_service, {});

        simulation->sleep(10000);

        // 3 - Read from CSS, using a file that was not written/copied to it beforehand
        auto jobReadError = job_manager->createCompoundJob("jobReadError");
        auto fileReadActionCSS = jobReadError->addFileReadAction(
                "fileReadActionCSS",
                wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_1000));

        job_manager->submitJob(jobReadError, test->compute_service, {});

        simulation->sleep(10000);

        // 4 - Try to write a file too big on CSS
        auto jobWriteError = job_manager->createCompoundJob("jobWriteError");
        auto fileWriteActionCSS = jobWriteError->addFileReadAction(
                "fileWriteActionCSS",
                wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_1000));

        job_manager->submitJob(jobWriteError, test->compute_service, {});

        simulation->sleep(10000);

        // 5 - Delete file from CSS, but it's not there
        auto jobDeleteError = job_manager->createCompoundJob("jobDeleteError");
        auto fileDeleteActionCSS = jobDeleteError->addFileReadAction(
                "fileDeleteActionCSS",
                wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_1000));

        job_manager->submitJob(jobDeleteError, test->compute_service, {});

        // 1
        this->waitForNextEvent();
        if (!jobCopyError->hasFailed())
            throw std::runtime_error("1-Unexpected job state: " + jobCopyError->getStateAsString());
        if (!std::dynamic_pointer_cast<wrench::FileNotFound>(fileCopyActionCSS_SS->getFailureCause()))
            throw std::runtime_error("1-Did not receive a 'NotAllowed' failure cause as expected");

        // 2
        this->waitForNextEvent();
        if (!jobCopySizeError->hasFailed())
            throw std::runtime_error("2-Unexpected job state: " + jobCopySizeError->getStateAsString());
        if (!std::dynamic_pointer_cast<wrench::StorageServiceNotEnoughSpace>(fileCopyActionSS_CSS->getFailureCause()))
            throw std::runtime_error("2-Did not receive a 'StorageServiceNotEnoughSpace' failure cause as expected");

        // 3
        this->waitForNextEvent();
        if (!jobReadError->hasFailed())
            throw std::runtime_error("3-Unexpected job state: " + jobReadError->getStateAsString());
        if (!std::dynamic_pointer_cast<wrench::FileNotFound>(fileReadActionCSS->getFailureCause()))
            throw std::runtime_error("3-Did not receive a 'FileNotFound' failure cause as expected");

        // 4
        this->waitForNextEvent();
        if (!jobWriteError->hasFailed())
            throw std::runtime_error("4-Unexpected job state: " + jobWriteError->getStateAsString());
        if (!std::dynamic_pointer_cast<wrench::FileNotFound>(fileWriteActionCSS->getFailureCause()))
            throw std::runtime_error("4-Did not receive a 'FileNotFound' failure cause as expected");

        // 5
        this->waitForNextEvent();
        if (!jobDeleteError->hasFailed())
            throw std::runtime_error("5-Unexpected job state: " + jobDeleteError->getStateAsString());
        if (!std::dynamic_pointer_cast<wrench::FileNotFound>(fileDeleteActionCSS->getFailureCause()))
            throw std::runtime_error("5-Did not receive a 'FileNotFound' failure cause as expected");

        return 0;
    }
};

TEST_F(CompoundStorageServiceFunctionalTest, BasicError) {
    DO_TEST_WITH_FORK(do_BasicError_test);
}

void CompoundStorageServiceFunctionalTest::do_BasicError_test() {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    // xbt_log_control_set("ker_engine.thres:debug");
    // xbt_log_control_set("wrench_core_file_transfer_thread.thres:info");
    // xbt_log_control_set("wrench_core_compound_storage_system.thresh:debug");
    // xbt_log_control_set("wrench_core_file_transfer_thread.thres:info");
    // xbt_log_control_set("wrench_core_storage_service.thres:info");

    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a Compute Service
    ASSERT_NO_THROW(
            compute_service = simulation->add(
                    new wrench::BareMetalComputeService("ComputeHost",
                                                        {std::make_pair("ComputeHost", std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                       wrench::ComputeService::ALL_RAM))},
                                                        {})));

    // Create some simple storage services
    // Bufferized
    ASSERT_NO_THROW(simple_storage_service_1000_0 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("SimpleStorageHost0", {"/disk1000"},
                                                                                     {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "1000000"}}, {})));

    // Non-bufferized
    ASSERT_NO_THROW(simple_storage_service_100_0 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("SimpleStorageHost0", {"/disk100"},
                                                                                     {}, {})));

    // Non-bufferized
    ASSERT_NO_THROW(simple_storage_service_510_1 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("SimpleStorageHost1", {"/disk510"},
                                                                                     {}, {})));

    // Create a valid Compound Storage Service (using a non-bufferized storage service in this case) with a user-provided callback
    // CAREFUL -> REUSING CALLBACK FROM PREVIOUS TEST
    TestAllocator allocator;
    wrench::StorageSelectionStrategyCallback allocatorCallback = [&allocator](
                                                                         const std::shared_ptr<wrench::DataFile> &file,
                                                                         const std::map<std::string, std::vector<std::shared_ptr<wrench::StorageService>>> &resources,
                                                                         const std::map<std::shared_ptr<wrench::DataFile>, std::vector<std::shared_ptr<wrench::FileLocation>>> &mapping,
                                                                         const std::vector<std::shared_ptr<wrench::FileLocation>> &previous_allocations) {
        return allocator(file, resources, mapping, previous_allocations);
    };
    ASSERT_NO_THROW(compound_storage_service = simulation->add(
                            new wrench::CompoundStorageService(
                                    "CompoundStorageHost",
                                    {simple_storage_service_510_1},
                                    allocatorCallback,
                                    {{wrench::CompoundStorageServiceProperty::MAX_ALLOCATION_CHUNK_SIZE, "100"}})));

    // Create a Controller
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
                            new CompoundStorageServiceErrorTestCtrl(this, "CompoundStorageHost")));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
