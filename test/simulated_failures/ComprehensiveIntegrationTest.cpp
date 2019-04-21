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
#include <wrench/services/helpers/ServiceTerminationDetectorMessage.h>

#include "../include/TestWithFork.h"
#include "../include/UniqueTmpPathPrefix.h"
#include "test_util/HostSwitcher.h"
#include "wrench/services/helpers/ServiceTerminationDetector.h"
#include "./test_util/SleeperVictim.h"
#include "./test_util/ComputerVictim.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(comprehensive_failure_integration_test, "Log category for ComprehesiveIntegrationFailureTest");

#define NUM_TASKS 10

class IntegrationSimulatedFailuresTest : public ::testing::Test {

public:
    wrench::Workflow *workflow;
    std::unique_ptr<wrench::Workflow> workflow_unique_ptr;

    wrench::StorageService *storage_service1;
    wrench::StorageService *storage_service2;
    wrench::ComputeService *cloud_service;
    wrench::ComputeService *baremetal_service;

    void do_IntegrationFailureTestTest_test();


protected:

    std::string createRoute(std::string src, std::string dst, std::vector<std::string> links) {
        std::string to_return = "<route src=\"" +  src + "\" ";
        to_return += "dst=\"" + dst + "\">\n";
        for (auto const &l : links) {
            to_return += "<link_ctn id=\"" + l + "\"/>\n";
        }
        to_return += "</route>\n";
        return to_return;
    }

    IntegrationSimulatedFailuresTest() {
        // Create the simplest workflow
        workflow_unique_ptr = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());
        workflow = workflow_unique_ptr.get();

        // Create a platform file
        std::string xml = "<?xml version='1.0'?>\n"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">\n"
                          "<platform version=\"4.1\">\n"
                          "   <zone id=\"AS0\" routing=\"Full\">\n";

        std::vector<std::string> hostnames = {"WMSHost",
                                              "StorageHost1", "StorageHost2",
                                              "CloudHead", "CloudHost1", "CloudHost2", "CloudHost3",
                                              "BareMetalHead", "BareMetalHost1", "BareMetalHost2"};

        for (auto const &h : hostnames) {
            xml += "<host id=\"" + h + "\"  speed=\"1f\" core=\"4\"/>\n";
        }

        xml += "<link id=\"link\" bandwidth=\"1kBps\" latency=\"0\"/>\n";
        xml += "<link id=\"linkcloud\" bandwidth=\"1kBps\" latency=\"0\"/>\n";
        xml += "<link id=\"linkcloudinternal\" bandwidth=\"1kBps\" latency=\"0\"/>\n";
        xml += "<link id=\"linkbaremetal\" bandwidth=\"1kBps\" latency=\"0\"/>\n";
        xml += "<link id=\"linkbaremetalinternal\" bandwidth=\"1kBps\" latency=\"0\"/>\n";
        xml += "<link id=\"linkstorage1\" bandwidth=\"1kBps\" latency=\"0\"/>\n";
        xml += "<link id=\"linkstorage2\" bandwidth=\"1kBps\" latency=\"0\"/>\n";

        xml += createRoute("WMSHost", "CloudHead", {"link", "linkcloud"});
        xml += createRoute("WMSHost", "BareMetalHead", {"link", "linkbaremetal"});
        xml += createRoute("WMSHost", "StorageHost1", {"link", "linkstorage1"});
        xml += createRoute("WMSHost", "StorageHost2", {"link", "linkstorage2"});

        xml += createRoute("CloudHead", "CloudHost1", {"link", "linkcloudinternal"});
        xml += createRoute("CloudHead", "CloudHost2", {"link", "linkcloudinternal"});
        xml += createRoute("CloudHead", "CloudHost3", {"link", "linkcloudinternal"});

        xml += createRoute("BareMetalHead", "BareMetalHost1", {"link", "linkbaremetalinternal"});
        xml += createRoute("BareMetalHead", "BareMetalHost2", {"link", "linkbaremetalinternal"});

        xml += createRoute("StorageHost1", "CloudHost1", {"linkstorage1", "linkcloud", "linkcloudinternal"});
        xml += createRoute("StorageHost1", "CloudHost2", {"linkstorage1", "linkcloud", "linkcloudinternal"});
        xml += createRoute("StorageHost1", "CloudHost3", {"linkstorage1", "linkcloud", "linkcloudinternal"});

        xml += createRoute("StorageHost1", "BareMetalHost1", {"linkstorage1", "linkbaremetal", "linkbaremetalinternal"});
        xml += createRoute("StorageHost1", "BareMetalHost2", {"linkstorage1", "linkbaremetal", "linkbaremetalinternal"});

        xml += "   </zone> \n"
               "</platform>\n";
#if 0

        "       <host id=\"StorageHost1\"   speed=\"1f\" core=\"1\"/> "
                          "       <host id=\"StorageHost2\"   speed=\"1f\" core=\"1\"/> "
                          "       <host id=\"CloudHead\"      speed=\"1f\" core=\"1\"/> "
                          "       <host id=\"CloudHost1\"     speed=\"1f\" core=\"1\"/> "
                          "       <host id=\"CloudHost2\"     speed=\"1f\" core=\"1\"/> "
                          "       <host id=\"CloudHost3\"     speed=\"1f\" core=\"1\"/> "
                          "       <host id=\"BareMetalHead\"  speed=\"1f\" core=\"1\"/> "
                          "       <host id=\"BareMetalHost1\" speed=\"1f\" core=\"1\"/> "
                          "       <host id=\"BareMetalHost2\" speed=\"1f\" core=\"1\"/> "
                          "       <host id=\"WMSHost\"        speed=\"1f\" core=\"1\"/> "
                          "       <link id=\"link1\" bandwidth=\"1kBps\" latency=\"0\"/>"
                          "       <route src=\"FailedHost\" dst=\"StableHost\">"
                          "           <link_ctn id=\"link1\"/>"
                          "       </route>"
                          "   </zone> "
                          "</platform>";

#endif
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};


/**********************************************************************/
/**          INTEGRATION TEST                                        **/
/**********************************************************************/

class IntegrationFailureTestTestWMS : public wrench::WMS {

public:
    IntegrationFailureTestTestWMS(IntegrationSimulatedFailuresTest *test,
                                  std::string &hostname,
                                  std::set<wrench::ComputeService *> compute_services,
                                  std::set<wrench::StorageService *> storage_services
    ) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    IntegrationSimulatedFailuresTest *test;

    int main() override {


        return 0;
    }
};

TEST_F(IntegrationSimulatedFailuresTest, StorageServiceReStartTest) {
    DO_TEST_WITH_FORK(do_IntegrationFailureTestTest_test);
}

void IntegrationSimulatedFailuresTest::do_IntegrationFailureTestTest_test() {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("failure_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create Storage Services
    this->storage_service1 = simulation->add(new wrench::SimpleStorageService("StorageHost1", 10000000000000.0));
    this->storage_service2 = simulation->add(new wrench::SimpleStorageService("StorageHost2", 10000000000000.0));

    // Create BareMetal Service
    this->baremetal_service = simulation->add(new wrench::BareMetalComputeService(
            "BareMetalHead",
            (std::map<std::string, std::tuple<unsigned long, double>>){
                    std::make_pair("BareMetalHost1", std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)),
                    std::make_pair("BareMetalHost2", std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)),
            },
            100.0,
            {}, {}));

    // Create Cloud Service
    std::string cloudhead = "CloudHead";
    std::vector<std::string> cloudhosts;
    cloudhosts.push_back("CloudHost1");
    cloudhosts.push_back("CloudHost2");
    cloudhosts.push_back("CloudHost3");
    this->cloud_service = simulation->add(new wrench::CloudService(
//            "CloudHead",
            cloudhead,
            cloudhosts,
            1000000.0,
            {}, {}));

    // Create a FileRegistryService
    simulation->add(new wrench::FileRegistryService("WMSHost"));

    // Create workflow tasks and stage input file
    for (int i=0; i < NUM_TASKS; i++) {
        auto task = workflow->addTask("task_" + std::to_string(i), 1 + rand() % 3600, 1, 3, 1.0, 0);
        auto input_file = workflow->addFile(task->getID() + ".input", 1 + rand() % 100);
        auto output_file = workflow->addFile(task->getID() + ".output", 1 + rand() % 100);
        task->addInputFile(input_file);
        task->addOutputFile(output_file);
        simulation->stageFiles({{input_file->getID(), input_file}}, storage_service1);
        simulation->stageFiles({{input_file->getID(), input_file}}, storage_service2);
    }


    // Create a WMS
    std::string wms_host = "WMSHost";
    auto wms = simulation->add(
            new IntegrationFailureTestTestWMS(
                    this,
                    wms_host,
                    {this->baremetal_service, this->cloud_service},
                    {this->storage_service1, this->storage_service2}));

    wms->addWorkflow(workflow);


    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    free(argv[0]);
    free(argv);
}




