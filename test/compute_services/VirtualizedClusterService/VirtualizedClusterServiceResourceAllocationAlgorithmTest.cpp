/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>
#include <wrench-dev.h>
#include <numeric>
#include <thread>
#include <chrono>

#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(virtualized_cluster_service_resource_allocation_test, "Log category for VirtualizedClusterServiceResourceAllocationTest");

#define EPSILON 0.001

class VirtualizedClusterServiceResourceAllocationTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::CloudComputeService> cloud_service_first_fit = nullptr;
    std::shared_ptr<wrench::CloudComputeService> cloud_service_best_fit_ram_first = nullptr;
    std::shared_ptr<wrench::CloudComputeService> cloud_service_best_fit_cores_first = nullptr;

    void do_VMResourceAllocationAlgorithm_test();


protected:
    VirtualizedClusterServiceResourceAllocationTest() {

        // Create the simplest workflow
        workflow_unique_ptr = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());
        workflow = workflow_unique_ptr.get();

        // Create a platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Gateway\" speed=\"1f\" > "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"40MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch_disk\" read_bw=\"100MBps\" write_bw=\"40MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host> "
                          "       <host id=\"4Cores10RAM\" speed=\"1f\" core=\"4\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"40MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch_disk\" read_bw=\"100MBps\" write_bw=\"40MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host> "
                          "         <prop id=\"ram\" value=\"10\"/> "
                          "       </host> "
                          "       <host id=\"2Cores20RAM\" speed=\"1f\" core=\"2\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"40MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch_disk\" read_bw=\"100MBps\" write_bw=\"40MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host> "
                          "         <prop id=\"ram\" value=\"20\"/> "
                          "       </host> "
                          "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                          "       <route src=\"Gateway\" dst=\"4Cores10RAM\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Gateway\" dst=\"2Cores20RAM\"> <link_ctn id=\"1\"/> </route>"
                          "   </zone> "
                          "</platform>";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
    wrench::Workflow *workflow;
    std::unique_ptr<wrench::Workflow> workflow_unique_ptr;
};


/**********************************************************************/
/**   VM RESOURCE ALLOCATION ALGORITHMS TEST                         **/
/**********************************************************************/

class VMResourceAllocationTestWMS : public wrench::WMS {

public:
    VMResourceAllocationTestWMS(VirtualizedClusterServiceResourceAllocationTest *test, std::string &hostname) :
            wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    VirtualizedClusterServiceResourceAllocationTest *test;

    int main() override {

        /*************************************************/
        /** FIRST FIT: FAIL CASE #1                     **/
        /*************************************************/
        this->test->cloud_service_first_fit->createVM(2, 1, "vm_1");
        this->test->cloud_service_first_fit->createVM(3, 1, "vm_2");
        this->test->cloud_service_first_fit->startVM("vm_1");
        try {
            this->test->cloud_service_first_fit->startVM("vm_2");
            throw std::runtime_error("FirstFit Fail Case #1: Starting the 2nd VM should have caused a NotEnoughResources error");
        } catch (wrench::WorkflowExecutionException &e) {
        }

        this->test->cloud_service_first_fit->shutdownVM("vm_1");
        this->test->cloud_service_first_fit->destroyVM("vm_1");
        this->test->cloud_service_first_fit->destroyVM("vm_2");

        /*************************************************/
        /** FIRST FIT: FAIL CASE #1                     **/
        /*************************************************/
        this->test->cloud_service_first_fit->createVM(1, 2, "vm_1");
        this->test->cloud_service_first_fit->createVM(1, 18, "vm_2");
        this->test->cloud_service_first_fit->createVM(1, 10, "vm_3");
        this->test->cloud_service_first_fit->startVM("vm_1");
        this->test->cloud_service_first_fit->startVM("vm_2");
        try {
            this->test->cloud_service_first_fit->startVM("vm_3");
            throw std::runtime_error("FirstFit Fail Case #2: Starting the 3rd VM should have caused a NotEnoughResources error");
        } catch (wrench::WorkflowExecutionException &e) {
        }

        this->test->cloud_service_first_fit->shutdownVM("vm_1");
        this->test->cloud_service_first_fit->shutdownVM("vm_2");
        this->test->cloud_service_first_fit->destroyVM("vm_1");
        this->test->cloud_service_first_fit->destroyVM("vm_2");
        this->test->cloud_service_first_fit->destroyVM("vm_3");

        /*************************************************/
        /** BEST FIT CORES FIRST: FAIL CASE #1          **/
        /*************************************************/
        this->test->cloud_service_best_fit_cores_first->createVM(2, 1, "vm_1");
        this->test->cloud_service_best_fit_cores_first->createVM(1, 20, "vm_2");
        this->test->cloud_service_best_fit_cores_first->startVM("vm_1");
        try {
            this->test->cloud_service_best_fit_cores_first->startVM("vm_2");
            throw std::runtime_error("BestFitCoresFirst Fail Case #1: Starting the 2nd VM should have caused a NotEnoughResources error");
        } catch (wrench::WorkflowExecutionException &e) {
        }

        this->test->cloud_service_best_fit_cores_first->shutdownVM("vm_1");
        this->test->cloud_service_best_fit_cores_first->destroyVM("vm_1");
        this->test->cloud_service_best_fit_cores_first->destroyVM("vm_2");

        /*************************************************/
        /** BEST FIT RAM FIRST: FAIL CASE #1            **/
        /*************************************************/
        this->test->cloud_service_best_fit_ram_first->createVM(2, 9, "vm_1");
        this->test->cloud_service_best_fit_ram_first->createVM(4, 1, "vm_2");
        this->test->cloud_service_best_fit_ram_first->startVM("vm_1");
        try {
            this->test->cloud_service_best_fit_ram_first->startVM("vm_2");
            throw std::runtime_error("BestFitRAMFirst Fail Case #1: Starting the 2nd VM should have caused a NotEnoughResources error");
        } catch (wrench::WorkflowExecutionException &e) {
        }

        this->test->cloud_service_best_fit_ram_first->shutdownVM("vm_1");
        this->test->cloud_service_best_fit_ram_first->destroyVM("vm_1");
        this->test->cloud_service_best_fit_ram_first->destroyVM("vm_2");


        return 0;
    }
};

TEST_F(VirtualizedClusterServiceResourceAllocationTest, VMResourceAllocationAlgorithm) {
    DO_TEST_WITH_FORK(do_VMResourceAllocationAlgorithm_test);
}

void VirtualizedClusterServiceResourceAllocationTest::do_VMResourceAllocationAlgorithm_test() {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("failure_test");


    simulation->init(&argc, argv);

    // Setting up the platform
    simulation->instantiatePlatform(platform_file_path);

    // Get a hostname
    std::string hostname = "Gateway";
    std::vector<std::string> compute_hosts;
    compute_hosts.push_back("4Cores10RAM");
    compute_hosts.push_back("2Cores20RAM");

    // Create a Compute Service that has access to two hosts
    cloud_service_first_fit = simulation->add(
            new wrench::CloudComputeService(hostname,
                                            compute_hosts,
                                            {"/scratch"},
                                            {{wrench::CloudComputeServiceProperty::VM_RESOURCE_ALLOCATION_ALGORITHM, "first-fit"}}));

    cloud_service_best_fit_ram_first = simulation->add(
            new wrench::CloudComputeService(hostname,
                                            compute_hosts,
                                            {"/scratch"},
                                            {{wrench::CloudComputeServiceProperty::VM_RESOURCE_ALLOCATION_ALGORITHM, "best-fit-ram-first"}}));

    cloud_service_best_fit_cores_first = simulation->add(
            new wrench::CloudComputeService(hostname,
                                            compute_hosts,
                                            {"/scratch"},
                                            {{wrench::CloudComputeServiceProperty::VM_RESOURCE_ALLOCATION_ALGORITHM, "best-fit-cores-first"}}));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    wms = simulation->add(new VMResourceAllocationTestWMS(this, hostname));

    wms->addWorkflow(workflow);

    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    free(argv[0]);
    free(argv);
}

