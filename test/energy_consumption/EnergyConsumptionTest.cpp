/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench-dev.h>
#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <wrench/simulation/SimulationMessage.h>
#include "services/compute/standard_job_executor/StandardJobExecutorMessage.h"
#include <gtest/gtest.h>
#include <wrench/services/compute/batch/BatchComputeService.h>
#include <wrench/services/compute/batch/BatchComputeServiceMessage.h>
#include "wrench/workflow/job/PilotJob.h"
#include <algorithm>
#include <simgrid/plugins/energy.h>

#include "../include/TestWithFork.h"
#include "../include/UniqueTmpPathPrefix.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(energy_consumption_test, "Log category for EnergyConsumptionTest");


class EnergyConsumptionTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::StorageService> storage_service1 = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service2 = nullptr;
    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;
    std::shared_ptr<wrench::ComputeService> compute_service1 = nullptr;
    std::shared_ptr<wrench::ComputeService> compute_service2 = nullptr;
    wrench::Simulation *simulation = nullptr;

    void do_AccessEnergyApiExceptionTests_test();

    void do_EnergyConsumption_test();

    void do_EnergyConsumptionPStateChange_test();

    void do_SimpleApiChecksEnergy_test();

    void do_PluginNotActive_test();

    std::unique_ptr<wrench::Workflow> workflow;

protected:
    EnergyConsumptionTest():workflow(std::unique_ptr<wrench::Workflow>(new wrench::Workflow())) {

        // Create a four-host 1-core platform file along with different pstates
        std::string xml = "<?xml version='1.0'?>"
                "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                "<platform version=\"4.1\">"
                "<zone id=\"AS0\" routing=\"Full\">"
                "<host id=\"MyHost1\" speed=\"100.0Mf,50.0Mf,20.0Mf\" pstate=\"0\" core=\"1\" >"
                "          <disk id=\"large_disk1\" read_bw=\"100MBps\" write_bw=\"40MBps\">"
                "             <prop id=\"size\" value=\"10000000000000B\"/>"
                "             <prop id=\"mount\" value=\"/disk1\"/>"
                "          </disk>"
                "          <disk id=\"large_disk2\" read_bw=\"100MBps\" write_bw=\"40MBps\">"
                "             <prop id=\"size\" value=\"10000000000000B\"/>"
                "             <prop id=\"mount\" value=\"/disk2\"/>"
                "          </disk>"
                "          <disk id=\"large_disk3\" read_bw=\"100MBps\" write_bw=\"40MBps\">"
                "             <prop id=\"size\" value=\"100B\"/>"
                "             <prop id=\"mount\" value=\"/scratch\"/>"
                "          </disk>"
                "          <prop id=\"watt_per_state\" value=\"100.0:200.0, 93.0:170.0, 90.0:150.0\" />"
                "          <prop id=\"watt_off\" value=\"10B\" />"
                "</host>"

                "<host id=\"MyHost2\" speed=\"100.0Mf,50.0Mf,20.0Mf\" pstate=\"0\" core=\"1\" >"
                "          <disk id=\"large_disk1\" read_bw=\"100MBps\" write_bw=\"40MBps\">"
                "             <prop id=\"size\" value=\"10000000000000B\"/>"
                "             <prop id=\"mount\" value=\"/disk1\"/>"
                "          </disk>"
                "          <disk id=\"large_disk2\" read_bw=\"100MBps\" write_bw=\"40MBps\">"
                "             <prop id=\"size\" value=\"10000000000000B\"/>"
                "             <prop id=\"mount\" value=\"/disk2\"/>"
                "          </disk>"
                "          <disk id=\"large_disk3\" read_bw=\"100MBps\" write_bw=\"40MBps\">"
                "             <prop id=\"size\" value=\"100B\"/>"
                "             <prop id=\"mount\" value=\"/scratch\"/>"
                "          </disk>"
                "          <prop id=\"watt_per_state\" value=\"100.0:200.0, 93.0:170.0, 90.0:150.0\" />"
                "          <prop id=\"watt_off\" value=\"10B\" />"
                "</host>"

                "<host id=\"MyHost3\" speed=\"100.0Mf,50.0Mf,20.0Mf\" pstate=\"0\" core=\"1\" >"
                "          <disk id=\"large_disk1\" read_bw=\"100MBps\" write_bw=\"40MBps\">"
                "             <prop id=\"size\" value=\"10000000000000B\"/>"
                "             <prop id=\"mount\" value=\"/disk2\"/>"
                "          </disk>"
                "          <disk id=\"large_disk2\" read_bw=\"100MBps\" write_bw=\"40MBps\">"
                "             <prop id=\"size\" value=\"10000000000000B\"/>"
                "             <prop id=\"mount\" value=\"/disk2\"/>"
                "          </disk>"
                "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"40MBps\">"
                "             <prop id=\"size\" value=\"100B\"/>"
                "             <prop id=\"mount\" value=\"/scratch\"/>"
                "          </disk>"
                "          <prop id=\"watt_per_state\" value=\"100.0:200.0, 93.0:170.0, 90.0:150.0\" />"
                "          <prop id=\"watt_off\" value=\"10B\" />"
                "</host>"

                "<link id=\"bus\" bandwidth=\"100kBps\" latency=\"0\" sharing_policy=\"SHARED\">"
                "<prop id=\"watt_range\" value=\"1:3\" />"
                "</link>"
                "<route src=\"MyHost1\" dst=\"MyHost2\">"
                "<link_ctn id=\"bus\"/>"
                "</route>"
                "<route src=\"MyHost1\" dst=\"MyHost3\">"
                "<link_ctn id=\"bus\"/>"
                "</route>"
                "<route src=\"MyHost2\" dst=\"MyHost3\">"
                "<link_ctn id=\"bus\"/>"
                "</route>"
                "</zone>"
                "</platform>";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);

    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";

};


/**********************************************************************/
/**         ENERGY API TEST WITHOUT ENABLING ENERGY PLUGIN           **/
/**********************************************************************/

class EnergyApiAccessExceptionsTestWMS : public wrench::WMS {

public:
    EnergyApiAccessExceptionsTestWMS(EnergyConsumptionTest *test,
                                     const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                     std::string& hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, {}, {}, nullptr, hostname,
                        "test") {
        this->test = test;
    }

private:

    EnergyConsumptionTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        {

            std::vector<std::string> simulation_hosts = wrench::Simulation::getHostnameList();

            //Now based on this default speed, (100MF), execute a job requiring 10^10 flops and check the time
            wrench::WorkflowTask *task = this->getWorkflow()->addTask("task1", 10000000000, 1, 1, 1.0, 1.0);

            // Create a StandardJob
            wrench::StandardJob *job = job_manager->createStandardJob(
                    task,
                    {
                    });
            //sleep for 10 seconds
            wrench::S4U_Simulation::sleep(10);
            //let's execute the job, this should take ~100 sec based on the 100MF speed
            std::string my_mailbox = "test_callback_mailbox";


            // Create a StandardJobExecutor that will run stuff on one host and 6 core
            std::shared_ptr<wrench::StandardJobExecutor> executor = std::unique_ptr<wrench::StandardJobExecutor>(
                    new wrench::StandardJobExecutor(
                            test->simulation,
                            my_mailbox,
                            wrench::Simulation::getHostnameList()[1],
                            job,
                            {std::make_pair(wrench::Simulation::getHostnameList()[1], std::make_tuple(1, wrench::ComputeService::ALL_RAM))},
                            nullptr,
                            false,
                            nullptr,
                            {},
                            {}
                    ));
            executor->start(executor, true, false); // Daemonized, no auto-restart

            // Wait for a message on my mailbox_name
            std::shared_ptr<wrench::SimulationMessage> message;
            try {
                message = wrench::S4U_Mailbox::getMessage(my_mailbox);
            } catch (std::shared_ptr<wrench::NetworkError> &cause) {
                throw std::runtime_error("Network error while getting reply from StandardJobExecutor!" + cause->toString());
            }

            // Did we get the expected message?
            auto msg = std::dynamic_pointer_cast<wrench::StandardJobExecutorDoneMessage>(message);
            if (!msg) {
                throw std::runtime_error("Unexpected '" + message->getName() + "' message");
            }

            try {
                double value = this->simulation->getEnergyConsumed("dummy_unavailable_host");
                throw std::runtime_error("Should not have been able to read the energy for dummy hosts");
            } catch (std::exception &e) {
                WRENCH_INFO("Expected exception as we were trying to measure the energy for a dummy host that is not available");
            }

            try {
                std::map<std::string, double> value = this->simulation->getEnergyConsumed(std::vector<std::string>({"dummy_unavailable_host"}));
                throw std::runtime_error(
                        "Should not have been able to read the energy for dummy hosts"
                );
            } catch (std::exception &e) {
                WRENCH_INFO("Expected exception as we were trying to measure the energy for a dummy host that is not available");
            }

            try {
                double value = this->simulation->getNumberofPstates("dummy_unavailable_host");
                throw std::runtime_error(
                        "Should not have been able to read the energy for dummy hosts"
                );
            } catch (std::exception &e) {
                WRENCH_INFO("Expected exception as we were trying to measure the energy for a dummy host that is not available");
            }

            try {
                double value = this->simulation->getCurrentPstate("dummy_unavailable_host");
                throw std::runtime_error(
                        "Should not have been able to read the energy for dummy hosts"
                );
            } catch (std::exception &e) {
                WRENCH_INFO("Expected exception as we were trying to measure the energy for a dummy host that is not available");
            }

            try {
                double value = this->simulation->getMinPowerConsumption("dummy_unavailable_host");
                throw std::runtime_error(
                        "Should not have been able to read the energy for dummy hosts"
                );
            } catch (std::exception &e) {
                WRENCH_INFO("Expected exception as we were trying to measure the energy for a dummy host that is not available");
            }

            try {
                double value = this->simulation->getMaxPowerConsumption("dummy_unavailable_host");
                throw std::runtime_error(
                        "Should not have been able to read the energy for dummy hosts"
                );
            } catch (std::exception &e) {
                WRENCH_INFO("Expected exception as we were trying to access energy plugin for a dummy host that is not available");
            }

            try {
                this->simulation->setPstate("dummy_unavailable_host",1);
                throw std::runtime_error(
                        "Should not have been able to read the energy for dummy hosts"
                );
            } catch (std::exception &e) {
                WRENCH_INFO("Expected exception as we were trying to access energy plugin for a dummy host that is not available");
            }

            try {
                this->simulation->setPstate(simulation_hosts[1],2);
                throw std::runtime_error(
                        "Should not have been able to read the energy for dummy hosts"
                );
            } catch (std::exception &e) {
                WRENCH_INFO("Unexpected exception as we were trying to access energy plugin for a correct host that is available");
            }

        }

        return 0;
    }
};

TEST_F(EnergyConsumptionTest, EnergyApiAccessExceptionsTest) {
    DO_TEST_WITH_FORK(do_AccessEnergyApiExceptionTests_test);
}


void EnergyConsumptionTest::do_AccessEnergyApiExceptionTests_test() {


    // Create and initialize a simulation
    simulation = new wrench::Simulation();
    int argc = 2;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--activate-energy");

    EXPECT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Storage Service
    EXPECT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/disk1"})));

    // Create a Storage Service
    EXPECT_NO_THROW(storage_service2 = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/disk2"})));


    // Create a Compute Service
    EXPECT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair(hostname, std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                                                "/scratch", {})));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    EXPECT_NO_THROW(wms = simulation->add(
            new EnergyApiAccessExceptionsTestWMS(
                    this,  {compute_service}, hostname)));

    EXPECT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));


    // Create two workflow files
    wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000.0);

    // Staging the input_file on the storage service
    EXPECT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "run a single task" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    EXPECT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv[1]);
    free(argv);
}

/**********************************************************************/
/**                    ENERGY CONSUMPTION TEST                       **/
/**********************************************************************/

class EnergyConsumptionTestWMS : public wrench::WMS {

public:
    EnergyConsumptionTestWMS(EnergyConsumptionTest *test,
                             const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                             std::string& hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, {}, {}, nullptr, hostname,
                        "test") {
        this->test = test;
    }

private:

    EnergyConsumptionTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        {

            //Now based on this default speed, (100MF), execute a job requiring 10^10 flops and check the time
            wrench::WorkflowTask *task = this->getWorkflow()->addTask("task1", 10000000000, 1, 1, 1.0, 1.0);

            // Create a StandardJob
            wrench::StandardJob *job = job_manager->createStandardJob(
                    task,
                    {
                    });
            //sleep for 10 seconds
            wrench::S4U_Simulation::sleep(10);
            //let's execute the job, this should take ~100 sec based on the 100MF speed
            std::string my_mailbox = "test_callback_mailbox";
            double before = wrench::S4U_Simulation::getClock();


            // Create a StandardJobExecutor that will run stuff on one host and 6 core
            std::shared_ptr<wrench::StandardJobExecutor> executor = std::unique_ptr<wrench::StandardJobExecutor>(
                    new wrench::StandardJobExecutor(
                            test->simulation,
                            my_mailbox,
                            wrench::Simulation::getHostnameList()[1],
                            job,
                            {std::make_pair(wrench::Simulation::getHostnameList()[1],
                                            std::make_tuple(1, wrench::ComputeService::ALL_RAM))},
                            nullptr,
                            false,
                            nullptr,
                            {},
                            {}
                    ));
            executor->start(executor, true, false); // Daemonized, no auto-restart

            // Wait for a message on my mailbox_name
            std::shared_ptr<wrench::SimulationMessage> message;
            try {
                message = wrench::S4U_Mailbox::getMessage(my_mailbox);
            } catch (std::shared_ptr<wrench::NetworkError> &cause) {
                throw std::runtime_error("Network error while getting reply from StandardJobExecutor!" + cause->toString());
            }

            // Did we get the expected message?
            auto msg = std::dynamic_pointer_cast<wrench::StandardJobExecutorDoneMessage>(message);
            if (!msg) {
                throw std::runtime_error("Unexpected '" + message->getName() + "' message");
            }

            double after = wrench::S4U_Simulation::getClock();

            double observed_duration = after - before;
            double expected_duration = 100;
            double EPSILON = 1;
            if (std::abs(observed_duration-expected_duration) > EPSILON) {
                throw std::runtime_error (
                        "EnergyConsumptionTest::SimpleEnergyConsumptionTest(): Took more time to compute than expected with the current speed of the host"
                );
            }

        }

        return 0;
    }
};

TEST_F(EnergyConsumptionTest, SimpleEnergyConsumptionTest) {
    DO_TEST_WITH_FORK(do_EnergyConsumption_test);
}


void EnergyConsumptionTest::do_EnergyConsumption_test() {


    // Create and initialize a simulation
    simulation = new wrench::Simulation();
    int argc = 2;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--activate-energy");

    EXPECT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Storage Service
    EXPECT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/disk1"})));

    // Create a Storage Service
    EXPECT_NO_THROW(storage_service2 = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/disk2"})));


    // Create a Compute Service
    EXPECT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair(hostname, std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                                                "/scratch", {})));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    EXPECT_NO_THROW(wms = simulation->add(
            new EnergyConsumptionTestWMS(
                    this,  {compute_service}, hostname)));

    EXPECT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));


    // Create two workflow files
    wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000.0);

    // Staging the input_file on the storage service
    EXPECT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "run a single task" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    EXPECT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv[1]);
    free(argv);
}


/**********************************************************************/
/**                 SIMPLE ENERGY API CHECK TEST                     **/
/**********************************************************************/

class EnergyAPICheckTestWMS : public wrench::WMS {

public:
    EnergyAPICheckTestWMS(EnergyConsumptionTest *test,
                          const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                          std::string& hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, {}, {}, nullptr, hostname,
                        "test") {
        this->test = test;
    }

private:

    EnergyConsumptionTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();
        {
            std::vector<std::string> simulation_hosts = wrench::Simulation::getHostnameList();

            int cur_pstate = this->simulation->getCurrentPstate(simulation_hosts[1]);
            double cur_max_possible = this->simulation->getMaxPowerConsumption(simulation_hosts[1]);
            double cur_min_possible = this->simulation->getMinPowerConsumption(simulation_hosts[1]);
            //switch pstates right off the bat
            std::vector<int> list_of_pstates = this->simulation->getListOfPstates(simulation_hosts[1]);
            int max_num_pstate = list_of_pstates.size();
            int pstate = std::max(0,max_num_pstate-1);
            this->simulation->setPstate(simulation_hosts[1],pstate);

            //check if the changed pstate is not equal to the current pstate
            if (cur_pstate == this->simulation->getCurrentPstate(simulation_hosts[1])) {
                throw std::runtime_error(
                        "The pstate should have changed but it did not change"
                );
            }

            //check if the max power possible/min power available in this pstate is different than the maximum power possible/min power available in the previous state
            for (auto host:simulation_hosts) {
                std::vector<int> states = this->simulation->getListOfPstates(host);
                int prev_max_power = -1;
                int prev_min_power = -1;
                for (auto state:states) {
                    //check if max power is different in all the states as is in xml
                    this->simulation->setPstate(host,state);
                    if (prev_max_power == this->simulation->getMaxPowerConsumption(host)) {
                        throw std::runtime_error(
                                "The max power from the xml and the APIs do not match"
                        );
                    }
                    prev_max_power = this->simulation->getMaxPowerConsumption(host);

                    //check if the min power is different in all the states as is in xml
                    this->simulation->setPstate(host,state);
                    if (prev_min_power == this->simulation->getMinPowerConsumption(host)) {
                        throw std::runtime_error(
                                "The min power from the xml and the APIs do not match"
                        );
                    }
                    prev_min_power = this->simulation->getMinPowerConsumption(host);
                }
            }
            //lets check if the energy consumed by host1 is less than the energy consumed by host1 + host2
            double energy_consumed_1 = this->simulation->getEnergyConsumed(simulation_hosts[1]);
            std::map<std::string, double> energy_consumed_2_map = this->simulation->getEnergyConsumed(std::vector<std::string>({simulation_hosts[1],simulation_hosts[2]}));
            double energy_consumed_2 = 0.0;
            for (auto const &h : energy_consumed_2_map) {
                energy_consumed_2 += h.second;
            }
            if (energy_consumed_1 > energy_consumed_2) {
                throw std::runtime_error(
                        "Energy consumed by host X is greater than the combined energy consumed by host X and host Y"
                );
            }

        }

        return 0;
    }
};

TEST_F(EnergyConsumptionTest, SimpleEnergyApiCheckTest) {
    DO_TEST_WITH_FORK(do_SimpleApiChecksEnergy_test);
}


void EnergyConsumptionTest::do_SimpleApiChecksEnergy_test() {


    // Create and initialize a simulation
    simulation = new wrench::Simulation();
    int argc = 2;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--activate-energy");

    EXPECT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Storage Service
    EXPECT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/disk1"})));

    // Create a Storage Service
    EXPECT_NO_THROW(storage_service2 = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/disk2"})));


    // Create a Compute Service
    EXPECT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair(hostname, std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                                                "/scratch", {})));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    EXPECT_NO_THROW(wms = simulation->add(
            new EnergyAPICheckTestWMS(
                    this,  {compute_service}, hostname)));

    EXPECT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));


    // Create two workflow files
    wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000.0);

    // Staging the input_file on the storage service
    EXPECT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "run a single task" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    EXPECT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv[1]);
    free(argv);
}


/**********************************************************************/
/**          ENERGY CONSUMPTION TEST WITH CHANGE IN PSTSATES         **/
/**********************************************************************/

class EnergyConsumptionPStateChangeTestWMS : public wrench::WMS {

public:
    EnergyConsumptionPStateChangeTestWMS(EnergyConsumptionTest *test,
                                         const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                         std::string& hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, {}, {}, nullptr, hostname,
                        "test") {
        this->test = test;
    }

private:

    EnergyConsumptionTest *test;

    int main() {

        //The tests is just to switch pstate and check if energy consumed is +ve and we don't have any segfaults or something
        // Create a job manager
        auto job_manager = this->createJobManager();

        {
            std::vector<std::string> simulation_hosts = wrench::Simulation::getHostnameList();

            //Now based on this default speed, (100MF), execute a job requiring 10^10 flops and check the time
            wrench::WorkflowTask *task1 = this->getWorkflow()->addTask("task1", 10000000000, 1, 1, 1.0, 1.0);

            // Create a StandardJob
            wrench::StandardJob *job1 = job_manager->createStandardJob(
                    task1,
                    {
                    });

            //Now based on this default speed, (100MF), execute a job requiring 10^10 flops and check the time
            wrench::WorkflowTask *task2 = this->getWorkflow()->addTask("task2", 10000000000, 1, 1, 1.0, 1.0);

            // Create a StandardJob
            wrench::StandardJob *job2 = job_manager->createStandardJob(
                    task2,
                    {
                    });


            //First energy consumption test
            double before_current_energy_consumed_by_host1 = this->simulation->getEnergyConsumed(simulation_hosts[1]);
            //run a new job
            //let's execute the job, this should take ~100 sec based on the 100MF speed
            std::string my_mailbox = "test_callback_mailbox";

            // Create a StandardJobExecutor
            std::shared_ptr<wrench::StandardJobExecutor> executor = std::unique_ptr<wrench::StandardJobExecutor>(
                    new wrench::StandardJobExecutor(
                            test->simulation,
                            my_mailbox,
                            wrench::Simulation::getHostnameList()[1],
                            job1,
                            {std::make_pair(wrench::Simulation::getHostnameList()[1], std::make_tuple(1, wrench::ComputeService::ALL_RAM))},
                            nullptr,
                            false,
                            nullptr,
                            {},
                            {}
                    ));
            executor->start(executor, true, false); // Daemonized, no auto-restart

            // Wait for a message on my mailbox_name
            std::shared_ptr<wrench::SimulationMessage> message;
            try {
                message = wrench::S4U_Mailbox::getMessage(my_mailbox);
            } catch (std::shared_ptr<wrench::NetworkError> &cause) {
                throw std::runtime_error("Network error while getting reply from StandardJobExecutor!" + cause->toString());
            }

            // Did we get the expected message?
            auto msg = std::dynamic_pointer_cast<wrench::StandardJobExecutorDoneMessage>(message);
            if (!msg) {
                throw std::runtime_error("Unexpected '" + message->getName() + "' message");
            }

            double after_current_energy_consumed_by_host1 = this->simulation->getEnergyConsumed(simulation_hosts[1]);
            double energy_consumed_while_running_with_higher_speed = after_current_energy_consumed_by_host1 - before_current_energy_consumed_by_host1;
            double higher_speed_compuation_time = wrench::S4U_Simulation::getClock();


            if (energy_consumed_while_running_with_higher_speed <= 0) {
                throw std::runtime_error("Unexpectedly the energy consumed is less than 0 for the max speed??");
            }

            //switch pstate
            int max_pstate_possible = this->simulation->getNumberofPstates(simulation_hosts[1]);
            //let's directly switch to pstate 2
            int pstate = 2;
            this->simulation->setPstate(simulation_hosts[1],pstate);

            //Second energy consumption test
            double before_current_energy_consumed_by_host2 = this->simulation->getEnergyConsumed(simulation_hosts[1]);
            //run a new job
            //let's execute the job, this should take ~100 sec based on the 100MF speed
            my_mailbox = "test_callback_mailbox";

            // Create a StandardJobExecutor
            executor = std::unique_ptr<wrench::StandardJobExecutor>(
                    new wrench::StandardJobExecutor(
                            test->simulation,
                            my_mailbox,
                            wrench::Simulation::getHostnameList()[1],
                            job2,
                            {std::make_pair(wrench::Simulation::getHostnameList()[1], std::make_tuple(1, wrench::ComputeService::ALL_RAM))},
                            nullptr,
                            false,
                            nullptr,
                            {},
                            {}
                    ));
            executor->start(executor, true, false); // Daemonized, no auto-restart

            try {
                message = wrench::S4U_Mailbox::getMessage(my_mailbox);
            } catch (std::shared_ptr<wrench::NetworkError> &cause) {
                throw std::runtime_error("Network error while getting reply from StandardJobExecutor!" + cause->toString());
            }

            // Did we get the expected message?
            msg = std::dynamic_pointer_cast<wrench::StandardJobExecutorDoneMessage>(message);
            if (!msg) {
                throw std::runtime_error("Unexpected '" + message->getName() + "' message");
            }

            double after_current_energy_consumed_by_host2 = this->simulation->getEnergyConsumed(simulation_hosts[1]);
            double energy_consumed_while_running_with_lower_speed = after_current_energy_consumed_by_host2 - before_current_energy_consumed_by_host2;
            double lower_speed_computation_time = wrench::S4U_Simulation::getClock() - higher_speed_compuation_time;

            if (energy_consumed_while_running_with_lower_speed <= 0) {
                throw std::runtime_error("Unexpectedly the energy consumed is less than 0 for a lower speed ??");
            }

            //check if the power states and times map with energy consumed
            //100.0:200.0, 93.0:170.0, 90.0:150.0
            //in pstate 0, the min_power, according to xml is 100.0 and the max power is 200.0
            //in pstate 2, the min_power, according to xml is 90.0 and the max power is 150.0
            //so, energy_consumed/time_taken might give us an approximate wattage power which should be in between these ranges
            //in fact, we are using these hosts to the full power, so the power wattage should be near the max values

            double exact_max_wattage_power_1 = this->simulation->getMaxPowerConsumption(simulation_hosts[1]);
            double exact_max_wattage_power_2 = this->simulation->getMaxPowerConsumption(simulation_hosts[1]);
            double EPSILON = 1.0;
            double computed_wattage_power_1 = energy_consumed_while_running_with_higher_speed/higher_speed_compuation_time;
            double computed_wattage_power_2 = energy_consumed_while_running_with_lower_speed/lower_speed_computation_time;

            if (std::abs(exact_max_wattage_power_1-computed_wattage_power_1) > EPSILON && std::abs(exact_max_wattage_power_2-computed_wattage_power_2) > EPSILON) {
                throw std::runtime_error(
                        "Something wrong with the computed energy and the expected energy consumption"
                );
            }


        }

        return 0;
    }
};

TEST_F(EnergyConsumptionTest, EnergyConsumptionPStateChangeTest) {
    DO_TEST_WITH_FORK(do_EnergyConsumptionPStateChange_test);
}


void EnergyConsumptionTest::do_EnergyConsumptionPStateChange_test() {


    // Create and initialize a simulation
    simulation = new wrench::Simulation();
    int argc = 2;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--activate-energy");

    EXPECT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Storage Service
    EXPECT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/disk1"})));

    // Create a Storage Service
    EXPECT_NO_THROW(storage_service2 = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/disk2"})));


    // Create a Compute Service
    EXPECT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair(hostname, std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                                                "/scratch", {})));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    EXPECT_NO_THROW(wms = simulation->add(
            new EnergyConsumptionPStateChangeTestWMS(
                    this,  {compute_service}, hostname)));

    EXPECT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));


    // Create two workflow files
    wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000.0);

    // Staging the input_file on the storage service
    EXPECT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "run a single task" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    EXPECT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv[1]);
    free(argv);
}



/**********************************************************************/
/**                     PLUGIN NOT ACTIVATED TEST                    **/
/**********************************************************************/

class PluginNotActivatedTestWMS : public wrench::WMS {

public:
    PluginNotActivatedTestWMS(EnergyConsumptionTest *test,
                              const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                              std::string& hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, {}, {}, nullptr, hostname,
                        "test") {
        this->test = test;
    }

private:

    EnergyConsumptionTest *test;

    int main() {

        //The tests is just to switch pstate and check if energy consumed is +ve and we don't have any segfaults or something
        // Create a job manager
        auto job_manager = this->createJobManager();

        {
            std::vector<std::string> simulation_hosts = wrench::Simulation::getHostnameList();

            double after_current_energy_consumed_by_host1 = this->simulation->getEnergyConsumed(simulation_hosts[1]);

            //switch pstate
            int max_pstate_possible = this->simulation->getNumberofPstates(simulation_hosts[1]);
            //let's directly switch to pstate 2

            //Second energy consumption test
            wrench::Simulation::sleep(10);
            double before_current_energy_consumed_by_host2 = this->simulation->getEnergyConsumed(simulation_hosts[1]);

        }

        return 0;
    }
};

TEST_F(EnergyConsumptionTest, DISABLED_PluginNotActivated) {
    DO_TEST_WITH_FORK(do_PluginNotActive_test);
}


void EnergyConsumptionTest::do_PluginNotActive_test() {


    // Create and initialize a simulation
    simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    EXPECT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Compute Service
    EXPECT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair(hostname, std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                                                "/scratch", {})));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    EXPECT_NO_THROW(wms = simulation->add(
            new PluginNotActivatedTestWMS(this,  {compute_service}, hostname)));

    EXPECT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));

    EXPECT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}

