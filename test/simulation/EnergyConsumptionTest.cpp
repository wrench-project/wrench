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
#include <wrench/services/compute/batch/BatchService.h>
#include <wrench/services/compute/batch/BatchServiceMessage.h>
#include "wrench/workflow/job/PilotJob.h"

#include "../include/TestWithFork.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(energy_consumption_test, "Log category for EnergyConsumptionTest");


class EnergyConsumptionTest : public ::testing::Test {

public:
    wrench::StorageService *storage_service1 = nullptr;
    wrench::StorageService *storage_service2 = nullptr;
    wrench::ComputeService *compute_service = nullptr;
    wrench::ComputeService *compute_service1 = nullptr;
    wrench::ComputeService *compute_service2 = nullptr;
    wrench::Simulation *simulation = nullptr;

    void do_EnergyConsumption_test();

    void do_EnergyConsumptionPStateChange_test();


protected:
    EnergyConsumptionTest() {

      // Create the simplest workflow
      workflow = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());

      // Create a four-host 1-core platform file along with different pstates
      std::string xml = "<?xml version='1.0'?>"
      "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
      "<platform version=\"4.1\">"
              "<zone id=\"AS0\" routing=\"Full\">"
              "<host id=\"MyHost1\" speed=\"100.0Mf,50.0Mf,20.0Mf\" pstate=\"0\" core=\"1\" >"
              "<prop id=\"watt_per_state\" value=\"100.0:200.0, 93.0:170.0, 90.0:150.0\" />"
              "<prop id=\"watt_off\" value=\"10\" />"
              "</host>"

              "<host id=\"MyHost2\" speed=\"100.0Mf,50.0Mf,20.0Mf\" pstate=\"0\" core=\"1\" >"
              "<prop id=\"watt_per_state\" value=\"100.0:200.0, 93.0:170.0, 90.0:150.0\" />"
              "<prop id=\"watt_off\" value=\"10\" />"
              "</host>"

              "<host id=\"MyHost3\" speed=\"100.0Mf,50.0Mf,20.0Mf\" pstate=\"0\" core=\"1\" >"
              "<prop id=\"watt_per_state\" value=\"100.0:200.0, 93.0:170.0, 90.0:150.0\" />"
              "<prop id=\"watt_off\" value=\"10\" />"
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

    std::string platform_file_path = "/tmp/platform.xml";
    std::unique_ptr<wrench::Workflow> workflow;

};

/**********************************************************************/
/**                SIMPLE ENERGY CONSUMPTION TEST                    **/
/**********************************************************************/

class EnergyConsumptionTestWMS : public wrench::WMS {

public:
    EnergyConsumptionTestWMS(EnergyConsumptionTest *test,
                              const std::set<wrench::ComputeService *> &compute_services,
                              std::string& hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, {}, {}, nullptr, hostname,
                        "test") {
      this->test = test;
    }

private:

    EnergyConsumptionTest *test;

    int main() {
      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      {
        std::vector<std::string> simulation_hosts = test->simulation->getHostnameList();

        //Now based on this default speed, (100MF), execute a job requiring 10^10 flops and check the time
        wrench::WorkflowTask *task = this->workflow->addTask("task1", 10000000000, 1, 1, 1.0);

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
                        test->simulation->getHostnameList()[1],
                        job,
                        {std::make_tuple(test->simulation->getHostnameList()[1], 1, wrench::ComputeService::ALL_RAM)},
                        nullptr,
                        false,
                        {}
                ));
        executor->start(executor, true);

        // Wait for a message on my mailbox_name
        std::unique_ptr<wrench::SimulationMessage> message;
        try {
          message = wrench::S4U_Mailbox::getMessage(my_mailbox);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
          throw std::runtime_error("Network error while getting reply from StandardJobExecutor!" + cause->toString());
        }

        // Did we get the expected message?
        auto *msg = dynamic_cast<wrench::StandardJobExecutorDoneMessage *>(message.get());
        if (!msg) {
          throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        double after = wrench::S4U_Simulation::getClock();

        double observed_duration = after - before;
        double expected_duration = 100;
        double EPSILON = 1;
        if (abs(observed_duration-expected_duration) > EPSILON) {
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
  argv[0] = strdup("energy_consumption_test");
  argv[1] = strdup("activate_energy");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));


  // Create a Compute Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname, true, true,
                                                       {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                       {}, 10000000000000.0)));

  simulation->add(new wrench::FileRegistryService(hostname));

  // Create a WMS
  wrench::WMS *wms = nullptr;
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
  free(argv);
}


/**********************************************************************/
/**          ENERGY CONSUMPTION TEST WITH CHANGE IN PSTSATES         **/
/**********************************************************************/

class EnergyConsumptionPStateChangeTestWMS : public wrench::WMS {

public:
    EnergyConsumptionPStateChangeTestWMS(EnergyConsumptionTest *test,
                             const std::set<wrench::ComputeService *> &compute_services,
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
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      {
        std::vector<std::string> simulation_hosts = test->simulation->getHostnameList();

        //Now based on this default speed, (100MF), execute a job requiring 10^10 flops and check the time
        wrench::WorkflowTask *task1 = this->workflow->addTask("task1", 10000000000, 1, 1, 1.0);

        // Create a StandardJob
        wrench::StandardJob *job1 = job_manager->createStandardJob(
                task1,
                {
                });

        //Now based on this default speed, (100MF), execute a job requiring 10^10 flops and check the time
        wrench::WorkflowTask *task2 = this->workflow->addTask("task2", 10000000000, 1, 1, 1.0);

        // Create a StandardJob
        wrench::StandardJob *job2 = job_manager->createStandardJob(
                task2,
                {
                });


        //First energy consumption test
        double before_current_energy_consumed_by_host1 = wrench::S4U_Simulation::getEnergyConsumedByHost(simulation_hosts[1]);
        //run a new job
        //let's execute the job, this should take ~100 sec based on the 100MF speed
        std::string my_mailbox = "test_callback_mailbox";

        // Create a StandardJobExecutor
        std::shared_ptr<wrench::StandardJobExecutor> executor = std::unique_ptr<wrench::StandardJobExecutor>(
                new wrench::StandardJobExecutor(
                        test->simulation,
                        my_mailbox,
                        test->simulation->getHostnameList()[1],
                        job1,
                        {std::make_tuple(test->simulation->getHostnameList()[1], 1, wrench::ComputeService::ALL_RAM)},
                        nullptr,
                        false,
                        {}
                ));
        executor->start(executor, true);

        // Wait for a message on my mailbox_name
        std::unique_ptr<wrench::SimulationMessage> message;
        try {
          message = wrench::S4U_Mailbox::getMessage(my_mailbox);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
          std::string error_msg = cause->toString();
          throw std::runtime_error("Network error while getting reply from StandardJobExecutor!" + cause->toString());
        }

        // Did we get the expected message?
        auto *msg = dynamic_cast<wrench::StandardJobExecutorDoneMessage *>(message.get());
        if (!msg) {
          throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        double after_current_energy_consumed_by_host1 = wrench::S4U_Simulation::getEnergyConsumedByHost(simulation_hosts[1]);
        double power_consumed_while_running_with_higher_speed = after_current_energy_consumed_by_host1 - before_current_energy_consumed_by_host1;

        if (power_consumed_while_running_with_higher_speed <= 0) {
          throw std::runtime_error("Unexpectedly the energy consumed is less than 0 for the max speed??");
        }

        //switch pstate
        int max_pstate_possible = wrench::S4U_Simulation::getNumberofPstates(simulation_hosts[1]);
        //generate a rand pstate between 1 and max_pstate_possible
        int rand_pstate = rand()%(max_pstate_possible) + 1;
        wrench::S4U_Simulation::setPstate(simulation_hosts[1],rand_pstate);

        //Second energy consumption test
        double before_current_energy_consumed_by_host2 = wrench::S4U_Simulation::getEnergyConsumedByHost(simulation_hosts[1]);
        //run a new job
        //let's execute the job, this should take ~100 sec based on the 100MF speed
        my_mailbox = "test_callback_mailbox";

        // Create a StandardJobExecutor
        executor = std::unique_ptr<wrench::StandardJobExecutor>(
                new wrench::StandardJobExecutor(
                        test->simulation,
                        my_mailbox,
                        test->simulation->getHostnameList()[1],
                        job2,
                        {std::make_tuple(test->simulation->getHostnameList()[1], 1, wrench::ComputeService::ALL_RAM)},
                        nullptr,
                        false,
                        {}
                ));
        executor->start(executor, true);

        try {
          message = wrench::S4U_Mailbox::getMessage(my_mailbox);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
          std::string error_msg = cause->toString();
          throw std::runtime_error("Network error while getting reply from StandardJobExecutor!" + cause->toString());
        }

        // Did we get the expected message?
        msg = dynamic_cast<wrench::StandardJobExecutorDoneMessage *>(message.get());
        if (!msg) {
          throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        double after_current_energy_consumed_by_host2 = wrench::S4U_Simulation::getEnergyConsumedByHost(simulation_hosts[1]);
        double power_consumed_while_running_with_lower_speed = after_current_energy_consumed_by_host2 - before_current_energy_consumed_by_host2;

        if (power_consumed_while_running_with_lower_speed <= 0) {
          throw std::runtime_error("Unexpectedly the energy consumed is less than 0 for a lower speed ??");
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
  argv[0] = strdup("energy_consumption_test");
  argv[1] = strdup("--activate_energy");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));


  // Create a Compute Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname, true, true,
                                                       {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                       {}, 10000000000000.0)));

  simulation->add(new wrench::FileRegistryService(hostname));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new EnergyConsumptionPStateChangeTestWMS(
                  this,  {compute_service}, hostname)));

  EXPECT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));


  // Create two workflow files
  wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000.0);
  wrench::WorkflowFile *output_file = this->workflow->addFile("output_file", 20000.0);

  // Staging the input_file on the storage service
  EXPECT_NO_THROW(simulation->stageFile(input_file, storage_service1));

  // Running a "run a single task" simulation
  // Note that in these tests the WMS creates workflow tasks, which a user would
  // of course not be likely to do
  EXPECT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}

