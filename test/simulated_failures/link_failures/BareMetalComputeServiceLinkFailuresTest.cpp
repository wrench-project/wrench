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


WRENCH_LOG_CATEGORY(bare_metal_compute_service_link_failures_test, "Log category for BareMetalComputeServiceLinkFailuresTest");


class BareMetalComputeServiceLinkFailuresTest : public ::testing::Test {

public:

    std::shared_ptr<wrench::ComputeService> cs = nullptr;

    void do_ResourceInformationLinkFailure_test();

protected:
    BareMetalComputeServiceLinkFailuresTest() {

        // Create the simplest workflow
        workflow = new wrench::Workflow();

        // Create a one-host platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"10000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"101B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>  "
                          "       <host id=\"Host2\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"10000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"101B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>  "
                          "       <host id=\"Host3\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"10000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"101B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>  "
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
/**  LINK FAILURE TEST DURING RESOURCE INFORMATION                   **/
/**********************************************************************/

class BareMetalComputeServiceResourceInformationTestWMS : public wrench::WMS {

public:
    BareMetalComputeServiceResourceInformationTestWMS(BareMetalComputeServiceLinkFailuresTest *test,
                                                      std::string hostname) :
            wrench::WMS(nullptr, nullptr,  {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    BareMetalComputeServiceLinkFailuresTest *test;

    int main() {

        // Create a link switcher on/off er
        auto switcher = std::shared_ptr<wrench::ResourceRandomRepeatSwitcher>(
                new wrench::ResourceRandomRepeatSwitcher("Host1", 123, 1, 50, 1, 10,
                                                         "link1", wrench::ResourceRandomRepeatSwitcher::ResourceType::LINK));
        switcher->simulation = this->simulation;
        switcher->start(switcher, true, false); // Daemonized, no auto-restart

        // Do a bunch of resource requests
        unsigned long num_failures = 0;
        unsigned long num_trials = 2000;
        for (unsigned int i=0; i < num_trials; i++) {
            try {

                wrench::Simulation::sleep(25);
                switch (i % 9) {
                    case 0:
                        this->test->cs->getNumHosts();
                        break;
                    case 1:
                        this->test->cs->getCoreFlopRate();
                        break;
                    case 2:
                        this->test->cs->getTotalNumCores();
                        break;
                    case 3:
                        this->test->cs->getPerHostNumCores();
                        break;
                    case 4:
                        this->test->cs->getPerHostNumIdleCores();
                        break;
                    case 5:
                        this->test->cs->getTotalNumIdleCores();
                        break;
                    case 6:
                        this->test->cs->getMemoryCapacity();
                        break;
                    case 7:
                        this->test->cs->getPerHostAvailableMemoryCapacity();
                        break;
                    case 8:
                        this->test->cs->getTTL();
                        break;
                }

            } catch (wrench::WorkflowExecutionException &e) {
//                WRENCH_INFO("Got an exception");
                num_failures++;
                if (not std::dynamic_pointer_cast<wrench::NetworkError>(e.getCause())) {
                    throw std::runtime_error("Invalid failure cause: " + e.getCause()->toString() + " (was expecting NetworkError");
                }
            }
        }

        WRENCH_INFO("FAILURES %lu / %lu", num_failures, num_trials);

        return 0;
    }
};

TEST_F(BareMetalComputeServiceLinkFailuresTest, ResourceInformationTest) {
    DO_TEST_WITH_FORK(do_ResourceInformationLinkFailure_test);
}

void BareMetalComputeServiceLinkFailuresTest::do_ResourceInformationLinkFailure_test() {

    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));


    this->cs = simulation->add(new wrench::BareMetalComputeService(
            "Host2",
            (std::map<std::string, std::tuple<unsigned long, double>>){
                    std::make_pair("Host2", std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)),
            },
            "/scratch",
            {},
            {
                    {wrench::BareMetalComputeServiceMessagePayload::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD, 1},
                    {wrench::BareMetalComputeServiceMessagePayload::RESOURCE_DESCRIPTION_REQUEST_MESSAGE_PAYLOAD, 1},
            }));

    auto wms = simulation->add(
            new BareMetalComputeServiceResourceInformationTestWMS(
                    this, "Host1"));

    wms->addWorkflow(workflow);

    simulation->launch();

    delete simulation;

    for (int i=0; i < argc; i++)
     free(argv[i]);
    free(argv);
}
