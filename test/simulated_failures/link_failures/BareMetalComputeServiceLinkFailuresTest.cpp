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
#include <memory>
#include <utility>

#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"

#include "../failure_test_util/ResourceRandomRepeatSwitcher.h"


WRENCH_LOG_CATEGORY(bare_metal_compute_service_link_failures_test, "Log category for BareMetalComputeServiceLinkFailuresTest");


class BareMetalComputeServiceLinkFailuresTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::ComputeService> cs = nullptr;

    void do_ResourceInformationLinkFailure_test();
    void do_MultiActionJobLinkFailure_test();

protected:
    ~BareMetalComputeServiceLinkFailuresTest() {
        workflow->clear();
        wrench::Simulation::removeAllFiles();
    }

    BareMetalComputeServiceLinkFailuresTest() {

        // Create the simplest workflow
        workflow = wrench::Workflow::createWorkflow();

        // Create a one-host platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
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
    std::shared_ptr<wrench::Workflow> workflow;
};

/**********************************************************************/
/**  LINK FAILURE TEST DURING RESOURCE INFORMATION                   **/
/**********************************************************************/

class BareMetalComputeServiceResourceInformationTestWMS : public wrench::ExecutionController {

public:
    BareMetalComputeServiceResourceInformationTestWMS(BareMetalComputeServiceLinkFailuresTest *test,
                                                      const std::string &hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:
    BareMetalComputeServiceLinkFailuresTest *test;

    int main() override {

        // Create a link switcher on/off er
        auto switcher = std::make_shared<wrench::ResourceRandomRepeatSwitcher>(
                "Host1", 123, 1, 50, 1, 10,
                "link1", wrench::ResourceRandomRepeatSwitcher::ResourceType::LINK);
        switcher->setSimulation(this->getSimulation());
        switcher->start(switcher, true, false);// Daemonized, no auto-restart

        // Do a bunch of resource requests
        unsigned long num_failures = 0;
        unsigned long num_trials = 2000;
        for (unsigned int i = 0; i < num_trials; i++) {
            try {

                wrench::Simulation::sleep(25);
                switch (i % 8) {
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
                        this->test->cs->getPerHostMemoryCapacity();
                        break;
                    case 7:
                        this->test->cs->getPerHostAvailableMemoryCapacity();
                        break;
                }

            } catch (wrench::ExecutionException &e) {
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
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 3;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-link-shutdown-simulation");
    argv[2] = strdup("--wrench-default-control-message-size=10");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));


    this->cs = simulation->add(new wrench::BareMetalComputeService(
            "Host2",
            (std::map<std::string, std::tuple<unsigned long, sg_size_t>>){
                    std::make_pair("Host2", std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)),
            },
            "/scratch",
            {{wrench::BareMetalComputeServiceProperty::SCRATCH_SPACE_BUFFER_SIZE, "10MB"}},
            {
                    {wrench::BareMetalComputeServiceMessagePayload::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD, 1},
                    {wrench::BareMetalComputeServiceMessagePayload::RESOURCE_DESCRIPTION_REQUEST_MESSAGE_PAYLOAD, 1},
            }));

    auto wms = simulation->add(
            new BareMetalComputeServiceResourceInformationTestWMS(this, "Host1"));

    simulation->launch();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  LINK FAILURE TEST BEFORE MULTI-ACTION JOB SUBMISSION            **/
/**********************************************************************/

class MultiActionJobLinkFailureTestWMS : public wrench::ExecutionController {
public:
    MultiActionJobLinkFailureTestWMS(BareMetalComputeServiceLinkFailuresTest *test,
                                     std::shared_ptr<wrench::Workflow> workflow,
                                     std::shared_ptr<wrench::BareMetalComputeService> compute_service,
                                     std::string &hostname) : wrench::ExecutionController(hostname, "test"),
                                                              test(test), workflow(std::move(workflow)),
                                                              compute_service(compute_service) {
    }

private:
    BareMetalComputeServiceLinkFailuresTest *test;
    std::shared_ptr<wrench::Workflow> workflow;
    std::shared_ptr<wrench::BareMetalComputeService> compute_service;

    int main() override {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a compound job
        auto job = job_manager->createCompoundJob("my_job");

        unsigned long first_chain_length = 10;
        unsigned long fork_width = 15;
        unsigned long second_chain_length = 6;

        std::vector<std::shared_ptr<wrench::SleepAction>> first_chain_tasks;

        for (unsigned long i = 0; i < first_chain_length; i++) {
            first_chain_tasks.push_back(job->addSleepAction("chain1_sleep_" + std::to_string(i), 10));
            if (i > 0) {
                job->addActionDependency(first_chain_tasks[i - 1], first_chain_tasks[i]);
            }
        }

        // Turn off the network :)
        wrench::Simulation::turnOffLink("link1");

        // Submit the job
        job_manager->submitJob(job, this->compute_service, {});

        auto event = this->waitForNextEvent();

        if (job->getState() != wrench::CompoundJob::DISCONTINUED) {
            throw std::runtime_error("Invalid job state");
        }

        for (const auto &a: job->getActions()) {
            if (a->getState() != wrench::Action::FAILED) {
                throw std::runtime_error("Invalid action state " + a->getStateAsString() + " (expecting FAILED)");
            }
        }

        return 0;
    }
};

TEST_F(BareMetalComputeServiceLinkFailuresTest, MultiActionJob) {
    DO_TEST_WITH_FORK(do_MultiActionJobLinkFailure_test);
}

void BareMetalComputeServiceLinkFailuresTest::do_MultiActionJobLinkFailure_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

    int argc = 3;
    auto argv = (char **) calloc(argc, sizeof(char *));
    int argi = 0;
    argv[argi++] = strdup("multi_action_test");
    argv[argi++] = strdup("--wrench-link-shutdown-simulation");
    argv[argi++] = strdup("--wrench-default-control-message-size=1024");
    //    argv[3] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a Compute Service
    std::shared_ptr<wrench::BareMetalComputeService> compute_service;
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BareMetalComputeService("Host3",
                                                                {std::make_pair("Host3",
                                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                wrench::ComputeService::ALL_RAM))},
                                                                {"/scratch"},
                                                                {{wrench::BareMetalComputeServiceProperty::SCRATCH_SPACE_BUFFER_SIZE, "10MB"}})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
                            new MultiActionJobLinkFailureTestWMS(this, workflow, compute_service, hostname)));

    // Running a "do nothing" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}