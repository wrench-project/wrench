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

#include <xbt/log.h>
#include <wrench-dev.h>
#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(Compound_storage_service_functional_test, "Log category for CompoundStorageServiceFunctionalTest");


class CompoundStorageServiceFunctionalTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::Workflow> workflow;

    std::shared_ptr<wrench::DataFile> file_1;
    std::shared_ptr<wrench::DataFile> file_10;
    std::shared_ptr<wrench::DataFile> file_100;
    std::shared_ptr<wrench::DataFile> file_500;
    std::shared_ptr<wrench::DataFile> file_1000;

    std::shared_ptr<wrench::SimpleStorageService> simple_storage_service_100 = nullptr;
    std::shared_ptr<wrench::SimpleStorageService> simple_storage_service_510 = nullptr;
    std::shared_ptr<wrench::SimpleStorageService> simple_storage_service_1000 = nullptr;
    std::shared_ptr<wrench::CompoundStorageService> compound_storage_service = nullptr;

    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;

    void do_BasicFunctionality_test();
    void do_BasicInterceptFunctionality_test();
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
        std::set<std::shared_ptr<wrench::StorageService>> expected_services{test->simple_storage_service_100, test->simple_storage_service_510};
        if (simple_storage_services != expected_services) {
            throw std::runtime_error("The list of returned simple storage services is incorrect");
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
        } catch (std::logic_error &e) {}

        {
            auto file_1_loc = wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_1);
            try {
                test->compound_storage_service->getFileLastWriteDate(file_1_loc);
                throw std::runtime_error("We shouldn't be able to get a FileLastWriteDate on a file that wasn't written to the CSS first");
            } catch (std::invalid_argument &e) {}
        }


        // CompoundStorageServer should never be a scratch space (at init or set as later)
        if (test->compound_storage_service->isScratch() == true) {
            throw std::runtime_error("CompoundStorageService should never have isScratch == true");
        }

        try {
            test->compound_storage_service->setIsScratch(true);
            throw std::runtime_error("CompoundStorageService can't be setup as a scratch space");
        } catch (std::logic_error &e) {}

        if (test->compound_storage_service->isBufferized()) {
            throw std::runtime_error("CompoundStorageService shouldn't be bufferized");
        }

        // ## Test multiple messages that should answer with a failure cause, and in turn generate an ExecutionException
        // on caller's side

        // File copy, CSS as src, file not known:
        {
            auto file_1_loc_ss = wrench::FileLocation::LOCATION(test->simple_storage_service_100, test->file_1);
            wrench::StorageService::createFileAtLocation(file_1_loc_ss);
            auto file_1_loc_css = wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_1);

            try {
                wrench::StorageService::copyFile(file_1_loc_css, file_1_loc_ss);
                throw std::runtime_error("File doesn't exist on CSS, copy with CSS as src should be impossible");
            } catch (wrench::ExecutionException &e) {
                if (!std::dynamic_pointer_cast<wrench::NotAllowed>(e.getCause())) {
                    throw std::runtime_error("Exception cause should have been 'NotAllowed'");
                }
            }
            test->simple_storage_service_100->deleteFile(file_1_loc_ss);
        }

        // File copy, CSS as dst, file can't be allocated (no callback provided) - src file exists:
        {
            auto file_1_loc_ss = wrench::FileLocation::LOCATION(test->simple_storage_service_100, test->file_1);
            wrench::StorageService::createFileAtLocation(file_1_loc_ss);
            auto file_1_loc_css = wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_1);

            try {
                wrench::StorageService::copyFile(file_1_loc_ss, file_1_loc_css);
                throw std::runtime_error("File doesn't exist and can't be allocated on CSS, copy with CSS as dst should be impossible");
            } catch (wrench::ExecutionException &e) {
                if (!std::dynamic_pointer_cast<wrench::NotAllowed>(e.getCause())) {
                    throw std::runtime_error("Exception cause should have been 'NotAllowed' because no storage_selection callback was provided");
                }
            }
            test->simple_storage_service_100->deleteFile(file_1_loc_ss);
        }

        auto file_1_loc_ss = wrench::FileLocation::LOCATION(test->simple_storage_service_100, test->file_1);
        auto file_1_loc_css = wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_1);

        try {
            wrench::StorageService::deleteFileAtLocation(file_1_loc_css);
            throw std::runtime_error("Should not be able to delete file from a CompoundStorageService if it has not first been written / copied to it");
        } catch (wrench::ExecutionException &) {}

        try {
            wrench::StorageService::readFileAtLocation(file_1_loc_css);
            throw std::runtime_error("Should not be able to read file from a CompoundStorageService if it has not first been written / copied to it");
        } catch (wrench::ExecutionException &) {}


        try {
            wrench::StorageService::writeFileAtLocation(file_1_loc_css);
            throw std::runtime_error("Should not be able to write file on a CompoundStorageService because no selection callback was provided");
        } catch (wrench::ExecutionException &e) {}


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

    //    xbt_log_control_set("wrench_core_storage_service.thres:debug");
    //    xbt_log_control_set("wrench_core_file_transfer_thread.thres:info");
    //    xbt_log_control_set("wrench_core_compound_storage_system.thresh:debug");
    //    xbt_log_control_set("wrench_core_file_transfer_thread.thres:info");

    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    // argv[1] = strdup("--wrench-full-log");

    // Setting up simulation and platform
    ASSERT_NO_THROW(simulation->init(&argc, argv));
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Syntaxic sugar
    auto compute = "ComputeHost";
    auto simple_storage0 = "SimpleStorageHost0";
    auto simple_storage1 = "SimpleStorageHost1";
    auto compound_storage = "CompoundStorageHost";

    // Create a Compute Service
    ASSERT_NO_THROW(
            compute_service = simulation->add(
                    new wrench::BareMetalComputeService(compute,
                                                        {std::make_pair(compute, std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                 wrench::ComputeService::ALL_RAM))},
                                                        {})));

    // Create some simple storage services
    // Unbufferized
    ASSERT_NO_THROW(simple_storage_service_100 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(simple_storage0, {"/disk100"},
                                                                                     {}, {})));
    // Bufferized
    ASSERT_NO_THROW(simple_storage_service_510 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(simple_storage1, {"/disk510"},
                                                                                     {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "1000000"}}, {})));

    // Fail to create a Compound Storage Service (no storage services provided)
    ASSERT_THROW(compound_storage_service = simulation->add(
                         new wrench::CompoundStorageService(compound_storage, {})),
                 std::invalid_argument);

    // Fail to create a Compound Storage Service (one of the storage service is just a nullptr)
    ASSERT_THROW(compound_storage_service = simulation->add(
                         new wrench::CompoundStorageService(compound_storage, {simple_storage_service_1000, simple_storage_service_100})),
                 std::invalid_argument);

    // Create a valid Compound Storage Service, without user provided callback (no intercept capabilities)
    ASSERT_NO_THROW(compound_storage_service = simulation->add(
                            new wrench::CompoundStorageService(compound_storage, {simple_storage_service_100, simple_storage_service_510})));

    // Create a Controler
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
                            new CompoundStorageServiceBasicFunctionalityTestCtrl(this, compound_storage)));

    // A bogus staging (can't use CompoundStorageService for staging)
    ASSERT_THROW(simulation->stageFile(file_10, compound_storage_service), std::invalid_argument);

    // Tun the simulation
    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  BASIC INTERCEPT FUNCTIONALITY SIMULATION TEST                   **/
/**********************************************************************/

class CompoundStorageServiceInterceptFunctionalityTestCtrl : public wrench::ExecutionController {

public:
    CompoundStorageServiceInterceptFunctionalityTestCtrl(CompoundStorageServiceFunctionalTest *test,
                                                         std::string hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    CompoundStorageServiceFunctionalTest *test;

    int main() override {

        std::vector<std::shared_ptr<wrench::Action>> actions;

        // Create a job, and test that messages are correctly forwarded from the css to the underlying storage services
        auto job_manager = this->createJobManager();
        auto job1 = job_manager->createCompoundJob("job1");

        wrench::StorageService::createFileAtLocation(wrench::FileLocation::LOCATION(test->simple_storage_service_1000, "/disk1000/", test->file_500));

        // Doing a plain file copy just for kicks
        wrench::StorageService::copyFile(wrench::FileLocation::LOCATION(test->simple_storage_service_1000, "/disk1000/", test->file_500),
                                         wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_500));

        // Copy from buffered simple storage to CSS (which uses a non-buffered simplestorage service)
        auto fileCopyActionSS_CSS = job1->addFileCopyAction(
                "fileCopySrcBufDstCSSNBuff",
                wrench::FileLocation::LOCATION(test->simple_storage_service_1000, "/disk1000/", test->file_500),
                wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_500));
        actions.push_back(fileCopyActionSS_CSS);

        // Copy back from CSS (using non-buff SS) to some other place in a bufferized SimpleStorageService
        auto fileCopyActionCSS_SS = job1->addFileCopyAction(
                "fileCopySrcCSSNBuffDstBufSS",
                wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_500),
                wrench::FileLocation::LOCATION(test->simple_storage_service_1000, "/disk1000/copy_from_css/", test->file_500));
        job1->addActionDependency(fileCopyActionSS_CSS, fileCopyActionCSS_SS);
        actions.push_back(fileCopyActionCSS_SS);

        // Reading from CSS (previously copied file)
        auto fileReadAction = job1->addFileReadAction("fileRead1", wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_500));
        job1->addActionDependency(fileCopyActionCSS_SS, fileReadAction);
        actions.push_back(fileReadAction);

        // Write another file to the CSS
        auto fileWriteAction = job1->addFileWriteAction("fileWrite1", wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_10));
        job1->addActionDependency(fileReadAction, fileWriteAction);
        actions.push_back(fileWriteAction);

        // Read the file directly from underlying storage service (in this case we know the file will be on simple_storage_service_510, disk 100)
        auto readWrittenFile = job1->addFileReadAction("directReadFile", wrench::FileLocation::LOCATION(test->simple_storage_service_510, "/disk510/", test->file_10));
        job1->addActionDependency(fileWriteAction, readWrittenFile);
        actions.push_back(readWrittenFile);

        // Read it as well through the CSS
        auto readWrittenFile2 = job1->addFileReadAction("directReadFile2", wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_10));
        job1->addActionDependency(fileWriteAction, readWrittenFile2);
        actions.push_back(readWrittenFile2);

        // Delete file from CSS
        auto fileDeleteAction = job1->addFileDeleteAction("fileDelete", wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_10));
        job1->addActionDependency(readWrittenFile, fileDeleteAction);
        job1->addActionDependency(readWrittenFile2, fileDeleteAction);
        actions.push_back(fileDeleteAction);

        job_manager->submitJob(job1, test->compute_service, {});

        std::shared_ptr<wrench::ExecutionEvent> event = this->waitForNextEvent();

        //        for (auto const &action : job1->getActions()) {
        //            std::cerr << " - " << action->getName() << ": " << action->getStateAsString() << "\n";
        //            std::cerr << "     " << (action->getFailureCause() ? action->getFailureCause()->toString() : "") << "\n";
        //        }

        if (not std::dynamic_pointer_cast<wrench::CompoundJobCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        if (job1->getState() != wrench::CompoundJob::State::COMPLETED) {
            throw std::runtime_error("Unexpected job state: " + job1->getStateAsString());
        }

        for (auto const &a: actions) {
            if (a->getState() != wrench::Action::State::COMPLETED) {
                throw std::runtime_error("One of the actions did not complete");
            }
        }


        // lookup a deleted file
        if (wrench::StorageService::lookupFileAtLocation(wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_100))) {
            throw runtime_error("A file supposed to be deleted (on CSS) was not.");
        }

        // Check that file copy worked
        auto file_500_designated_loc = test->compound_storage_service->lookupFileLocation(test->file_500);
        if (!file_500_designated_loc) {
            throw std::runtime_error("Should have been able to lookup file_500 through CSS");
        } else if (file_500_designated_loc->getPath() != "/") {// TODO: HENRI CHANGED THIS TO "/" which is the logical path (used to say "/diskXXX/" which is physical)
            throw std::runtime_error("file_500 copy through CSS is not where it should be (got path: " + file_500_designated_loc->getPath());
        }

        return 0;
    }
};

TEST_F(CompoundStorageServiceFunctionalTest, BasicInterceptFunctionality) {
    DO_TEST_WITH_FORK(do_BasicInterceptFunctionality_test);
}

/* For testing purpose, dummy StorageSelectionStrategyCallback */
std::shared_ptr<wrench::FileLocation> defaultStorageServiceSelection(
        const std::shared_ptr<wrench::DataFile> &file,
        const std::set<std::shared_ptr<wrench::StorageService>> &resources,
        const std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>> &mapping) {

    auto capacity_req = file->getSize();

    std::shared_ptr<wrench::FileLocation> designated_location = nullptr;

    for (const auto &storage_service: resources) {

        auto free_space = storage_service->getTotalFreeSpace();
        if (free_space >= capacity_req) {
            designated_location = wrench::FileLocation::LOCATION(storage_service, file);// TODO: MAJOR CHANGE
            break;
        }
        //        for (const auto &free_space_entry : free_space) {
        //            if (free_space_entry.second >= capacity_req) {
        //                designated_location = wrench::FileLocation::LOCATION(storage_service, free_space_entry.first, file);
        //                break;
        //            }
        //        }
    }

    return designated_location;
}

void CompoundStorageServiceFunctionalTest::do_BasicInterceptFunctionality_test() {

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
    //    argv[2] = strdup("--log=wrench_core_mailbox.t=debug");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    auto compute = "ComputeHost";
    auto simple_storage0 = "SimpleStorageHost0";
    auto simple_storage1 = "SimpleStorageHost1";
    auto compound_storage = "CompoundStorageHost";

    // Create a Compute Service
    ASSERT_NO_THROW(
            compute_service = simulation->add(
                    new wrench::BareMetalComputeService(compute,
                                                        {std::make_pair(compute, std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                 wrench::ComputeService::ALL_RAM))},
                                                        {})));

    // Create some simple storage services
    // Bufferized
    ASSERT_NO_THROW(simple_storage_service_1000 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(simple_storage0, {"/disk1000"},
                                                                                     {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "1000000"}}, {})));

    // Non-bufferized
    ASSERT_NO_THROW(simple_storage_service_100 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(simple_storage0, {"/disk100"},
                                                                                     {}, {})));

    // Non-bufferized
    //    ASSERT_NO_THROW(simple_storage_service_510 = simulation->add(
    //            wrench::SimpleStorageService::createSimpleStorageService(simple_storage1, {"/disk100", "/disk510"},
    //                                                                     {}, {})));
    ASSERT_NO_THROW(simple_storage_service_510 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(simple_storage1, {"/disk510"},
                                                                                     {}, {})));

    // Create a valid Compound Storage Service (using a non-bufferized storage service in this case) with a user-provided callback
    ASSERT_NO_THROW(compound_storage_service = simulation->add(
                            new wrench::CompoundStorageService(compound_storage, {simple_storage_service_510}, defaultStorageServiceSelection)));

    // Create a Controller
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
                            new CompoundStorageServiceInterceptFunctionalityTestCtrl(this, compute)));

    // Running a "run a single task1" simulation
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
                wrench::FileLocation::LOCATION(test->simple_storage_service_1000, "/disk1000/copy_from_css/", test->file_500));

        job_manager->submitJob(jobCopyError, test->compute_service, {});

        // 2 - Copy from SS to CSS, using a file that is too big to be allocated
        auto jobCopySizeError = job_manager->createCompoundJob("jobCopySizeError");
        wrench::StorageService::createFileAtLocation(wrench::FileLocation::LOCATION(test->simple_storage_service_1000, "/disk1000/", test->file_1000));
        auto fileCopyActionSS_CSS = jobCopySizeError->addFileCopyAction(
                "fileCopySrcSS_DstCSS",
                wrench::FileLocation::LOCATION(test->simple_storage_service_1000, "/disk1000/", test->file_1000),
                wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_1000));

        job_manager->submitJob(jobCopySizeError, test->compute_service, {});

        // 3 - Read from CSS, using a file that was not written/copied to it beforehand
        auto jobReadError = job_manager->createCompoundJob("jobReadError");
        auto fileReadActionCSS = jobReadError->addFileReadAction(
                "fileReadActionCSS",
                wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_1000));

        job_manager->submitJob(jobReadError, test->compute_service, {});

        // 4 - Write to CSS, with a file too big to be allocated
        auto jobWriteError = job_manager->createCompoundJob("jobWriteError");
        auto fileWriteActionCSS = jobWriteError->addFileReadAction(
                "fileWriteActionCSS",
                wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_1000));

        job_manager->submitJob(jobWriteError, test->compute_service, {});

        // 5 - Write to CSS, with a file too big to be allocated
        auto jobDeleteError = job_manager->createCompoundJob("jobDeleteError");
        auto fileDeleteActionCSS = jobDeleteError->addFileReadAction(
                "fileDeleteActionCSS",
                wrench::FileLocation::LOCATION(test->compound_storage_service, test->file_1000));

        job_manager->submitJob(jobDeleteError, test->compute_service, {});


        // 1
        this->waitForNextEvent();
        if (!jobCopyError->hasFailed())
            throw std::runtime_error("Unexpected job state: " + jobCopyError->getStateAsString());
        if (!std::dynamic_pointer_cast<wrench::NotAllowed>(fileCopyActionCSS_SS->getFailureCause()))
            throw std::runtime_error("Did not receive a 'NotAllowed' failure cause as expected");

        // 2
        this->waitForNextEvent();
        if (!jobCopySizeError->hasFailed())
            throw std::runtime_error("Unexpected job state: " + jobCopySizeError->getStateAsString());
        if (!std::dynamic_pointer_cast<wrench::StorageServiceNotEnoughSpace>(fileCopyActionSS_CSS->getFailureCause()))
            throw std::runtime_error("Did not receive a 'StorageServiceNotEnoughSpace' failure cause as expected");

        // 3
        this->waitForNextEvent();
        if (!jobReadError->hasFailed())
            throw std::runtime_error("Unexpected job state: " + jobReadError->getStateAsString());
        if (!std::dynamic_pointer_cast<wrench::FileNotFound>(fileReadActionCSS->getFailureCause()))
            throw std::runtime_error("Did not receive a 'FileNotFound' failure cause as expected");

        // 4
        this->waitForNextEvent();
        if (!jobWriteError->hasFailed())
            throw std::runtime_error("Unexpected job state: " + jobWriteError->getStateAsString());
        if (!std::dynamic_pointer_cast<wrench::FileNotFound>(fileWriteActionCSS->getFailureCause()))
            throw std::runtime_error("Did not receive a 'FileNotFound' failure cause as expected");

        // 5
        this->waitForNextEvent();
        if (!jobDeleteError->hasFailed())
            throw std::runtime_error("Unexpected job state: " + jobDeleteError->getStateAsString());
        if (!std::dynamic_pointer_cast<wrench::FileNotFound>(fileDeleteActionCSS->getFailureCause()))
            throw std::runtime_error("Did not receive a 'FileNotFound' failure cause as expected");

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

    // Get a hostname
    auto compute = "ComputeHost";
    auto simple_storage0 = "SimpleStorageHost0";
    auto simple_storage1 = "SimpleStorageHost1";
    auto compound_storage = "CompoundStorageHost";

    // Create a Compute Service
    ASSERT_NO_THROW(
            compute_service = simulation->add(
                    new wrench::BareMetalComputeService(compute,
                                                        {std::make_pair(compute, std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                 wrench::ComputeService::ALL_RAM))},
                                                        {})));

    // Create some simple storage services
    // Bufferized
    ASSERT_NO_THROW(simple_storage_service_1000 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(simple_storage0, {"/disk1000"},
                                                                                     {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "1000000"}}, {})));

    // Non-bufferized
    ASSERT_NO_THROW(simple_storage_service_100 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(simple_storage0, {"/disk100"},
                                                                                     {}, {})));

    // Non-bufferized
    ASSERT_NO_THROW(simple_storage_service_510 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(simple_storage1, {"/disk510"},
                                                                                     {}, {})));

    // Create a valid Compound Storage Service (using a non-bufferized storage service in this case) with a user-provided callback
    // CAREFUL -> REUSING CALLBACK FROM PREVIOUS TEST
    ASSERT_NO_THROW(compound_storage_service = simulation->add(
                            new wrench::CompoundStorageService(compound_storage, {simple_storage_service_510}, defaultStorageServiceSelection)));

    // Create a Controller
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
                            new CompoundStorageServiceErrorTestCtrl(this, compute)));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
