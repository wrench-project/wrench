/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <math.h>
#include <gtest/gtest.h>
#include <wrench-dev.h>

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(bare_metal_compute_service_actions_that_communicate_test, "Log category for BareMetalComputeServiceActionsThatCommunicateTest test");

class BareMetalComputeServiceActionsThatCommunicateTest : public ::testing::Test {
public:
    void do_TwoCommunicatingActions_test();
    void do_MPICollectives_test();

protected:
    ~BareMetalComputeServiceActionsThatCommunicateTest() override {
    }

    BareMetalComputeServiceActionsThatCommunicateTest() {

        // Create a platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk1\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/disk1/\"/>"
                          "          </disk>"
                          "          <disk id=\"large_disk2\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/disk2/\"/>"
                          "          </disk>"

                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Host2\" speed=\"1f\" core=\"1\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Host3\" speed=\"1f\" core=\"1\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Host4\" speed=\"1f\" core=\"1\">  "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "         <prop id=\"ram\" value=\"1024B\"/> "
                          "       </host>  "
                          "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                          "       <link id=\"2\" bandwidth=\"10MBps\" latency=\"0us\"/>"
                          "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host2\" dst=\"Host3\"> <link_ctn id=\"2\"/> </route>"
                          "       <route src=\"Host2\" dst=\"Host4\"> <link_ctn id=\"2\"/> </route>"
                          "       <route src=\"Host3\" dst=\"Host4\"> <link_ctn id=\"2\"/> </route>"
                          "       <route src=\"Host1\" dst=\"Host4\"> <link_ctn id=\"2\"/> </route>"
                          "       <route src=\"Host1\" dst=\"Host3\"> <link_ctn id=\"2\"/> </route>"
                          "   </zone> "
                          "</platform>";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};

/**********************************************************************/
/**  TWO COMMUNICATING ACTIONS TEST                                  **/
/**********************************************************************/

class BareMetalTwoCommunicatingActionsTestExecutionController : public wrench::ExecutionController {
public:
    BareMetalTwoCommunicatingActionsTestExecutionController(BareMetalComputeServiceActionsThatCommunicateTest *test,
                                                            std::string hostname,
                                                            const std::shared_ptr<wrench::BareMetalComputeService> &compute_service) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
        this->compute_service = compute_service;
    }

private:
    BareMetalComputeServiceActionsThatCommunicateTest *test;
    std::shared_ptr<wrench::BareMetalComputeService> compute_service;


    int main() override {

        auto job_manager = this->createJobManager();

        // Create a compound job
        auto job = job_manager->createCompoundJob("job");

        // Create a communicator of size 2
        try {
            auto communicator = wrench::Communicator::createCommunicator(0);
            throw std::runtime_error("Should not be able to create a zero-sized communicator");
        } catch (std::invalid_argument &ignore) {}

        auto communicator = wrench::Communicator::createCommunicator(2);

        // Coverage
        try {
            communicator->join(10);
            throw std::runtime_error("Should not be able to ask for a bogus rank");
        } catch (std::invalid_argument &ignore) {}

        // Create two actions
        auto lambda_execute = [communicator](const std::shared_ptr<wrench::ActionExecutor> &action_executor) {
            auto num_procs = communicator->getNumRanks();
            auto my_rank = communicator->join();
            WRENCH_INFO("I am in a communicator with rank %lu/%lu", my_rank, num_procs);
            communicator->sendAndReceive({{1 - my_rank, 1000.0}}, 1);
        };
        auto lambda_terminate = [](const std::shared_ptr<wrench::ActionExecutor> &action_executor) {};

        auto action1 = job->addCustomAction("action1", 0, 0, lambda_execute, lambda_terminate);
        auto action2 = job->addCustomAction("action2", 0, 0, lambda_execute, lambda_terminate);

        // Submit the job
        job_manager->submitJob(job, this->compute_service);

        // Wait for the job completion
        auto event = this->waitForNextEvent();

        if (not std::dynamic_pointer_cast<wrench::CompoundJobCompletedEvent>(event)) {
            throw std::runtime_error("Did not receive the expected CompoundJobCompletedEvent (instead got " + event->toString() + ")");
        }

        // Inspect actions
        if (action1->getState() != wrench::Action::COMPLETED or action1->getFailureCause() != nullptr) {
            throw std::runtime_error("Action 1 should have completed");
        }
        if (action2->getState() != wrench::Action::COMPLETED or action1->getFailureCause() != nullptr) {
            throw std::runtime_error("Action 2 should have completed");
        }

        return 0;
    }
};

TEST_F(BareMetalComputeServiceActionsThatCommunicateTest, TwoCommunicatingTasks) {
    DO_TEST_WITH_FORK(do_TwoCommunicatingActions_test);
}

void BareMetalComputeServiceActionsThatCommunicateTest::do_TwoCommunicatingActions_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");
    //    argv[2] = strdup("--log=wrench_core_mailbox.threshold:debug");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    auto compute_service = simulation->add(
            new wrench::BareMetalComputeService("Host1",
                                                {std::make_pair("Host2",
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM)),
                                                 std::make_pair("Host3",
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                "", {}, {}));

    // Create an Execution Controller
    auto controller = simulation->add(new BareMetalTwoCommunicatingActionsTestExecutionController(this, "Host1", compute_service));

    // Run a do nothing simulation, because why not
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  MPI TEST                                                        **/
/**********************************************************************/

class BareMetalMPIAllToAllTestExecutionController : public wrench::ExecutionController {
public:
    BareMetalMPIAllToAllTestExecutionController(BareMetalComputeServiceActionsThatCommunicateTest *test,
                                                std::string hostname,
                                                const std::shared_ptr<wrench::BareMetalComputeService> &compute_service) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
        this->compute_service = compute_service;
    }

private:
    BareMetalComputeServiceActionsThatCommunicateTest *test;
    std::shared_ptr<wrench::BareMetalComputeService> compute_service;


    int main() override {

        auto job_manager = this->createJobManager();

        // Create a compound job
        auto job = job_manager->createCompoundJob("job");

        // Create a communicator of size 2
        auto communicator = wrench::Communicator::createCommunicator(2);

        // Create two actions
        auto lambda_execute = [communicator](const std::shared_ptr<wrench::ActionExecutor> &action_executor) {
            auto num_procs = communicator->getNumRanks();
            auto my_rank = communicator->join();
            WRENCH_INFO("I am in a communicator with rank %lu/%lu", my_rank, num_procs);

            WRENCH_INFO("Doing an Alltoall");
            communicator->MPI_Alltoall(10000000);
            WRENCH_INFO("Done with the Alltoall");

            communicator->barrier();

            WRENCH_INFO("Doing an Alltoall");
            communicator->MPI_Alltoall(10000000, "mpich");
            WRENCH_INFO("Done with the Alltoall");

            communicator->barrier();

            WRENCH_INFO("Doing a Bcast");
            communicator->MPI_Bcast(0, 10000000);
            WRENCH_INFO("Done with the Bcast");

            communicator->barrier();

            WRENCH_INFO("Doing a Barrier");
            communicator->MPI_Barrier();
            WRENCH_INFO("Done with the Barrier");
        };

        auto lambda_terminate = [](const std::shared_ptr<wrench::ActionExecutor> &action_executor) {};

        auto action1 = job->addCustomAction("action1", 0, 1, lambda_execute, lambda_terminate);
        auto action2 = job->addCustomAction("action2", 0, 1, lambda_execute, lambda_terminate);

        // Submit the job
        job_manager->submitJob(job, this->compute_service);

        // Wait for the job completion
        auto event = this->waitForNextEvent();

        if (not std::dynamic_pointer_cast<wrench::CompoundJobCompletedEvent>(event)) {
            throw std::runtime_error("Did not receive the expected CompoundJobCompletedEvent (instead got " + event->toString() + ")");
        }

        // Inspect actions
        if (action1->getState() != wrench::Action::COMPLETED or action1->getFailureCause() != nullptr) {
            throw std::runtime_error("Action 1 should have completed");
        }
        if (action2->getState() != wrench::Action::COMPLETED or action1->getFailureCause() != nullptr) {
            throw std::runtime_error("Action 2 should have completed");
        }

        return 0;
    }
};

TEST_F(BareMetalComputeServiceActionsThatCommunicateTest, MPICollectives) {
    DO_TEST_WITH_FORK(do_MPICollectives_test);
}

void BareMetalComputeServiceActionsThatCommunicateTest::do_MPICollectives_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");
    //        argv[2] = strdup("--cfg=smpi/host-speed:0.001");
    //        argv[2] = strdup("--log=wrench_core_mailbox.threshold:debug");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    auto compute_service = simulation->add(
            new wrench::BareMetalComputeService("Host1",
                                                {std::make_pair("Host2",
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM)),
                                                 std::make_pair("Host3",
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                "", {}, {}));

    // Create an Execution Controller
    auto controller = simulation->add(new BareMetalMPIAllToAllTestExecutionController(this, "Host1", compute_service));

    // Run a do nothing simulation, because why not
    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
