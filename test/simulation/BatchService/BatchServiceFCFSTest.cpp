/**
 * Copyright (c) 2017. The WRENCH Team.
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

#include "../../include/TestWithFork.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(batch_service_fcfs_test, "Log category for BatchServiceFCFSTest");

#define EPSILON 0.05

class BatchServiceFCFSTest : public ::testing::Test {

public:
    wrench::ComputeService *compute_service = nullptr;
    wrench::Simulation *simulation;

    void do_SimpleFCFS_test();

protected:
    BatchServiceFCFSTest() {

      // Create the simplest workflow
      workflow = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());

      // Create a four-host 10-core platform file
      std::string xml = "<?xml version='1.0'?>"
              "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
              "<platform version=\"4.1\"> "
              "   <zone id=\"AS0\" routing=\"Full\"> "
              "       <host id=\"Host1\" speed=\"1f\" core=\"10\"/> "
              "       <host id=\"Host2\" speed=\"1f\" core=\"10\"/> "
              "       <host id=\"Host3\" speed=\"1f\" core=\"10\"/> "
              "       <host id=\"Host4\" speed=\"1f\" core=\"10\"/> "
              "       <link id=\"1\" bandwidth=\"50000GBps\" latency=\"0us\"/>"
              "       <link id=\"2\" bandwidth=\"0.0001MBps\" latency=\"1000000us\"/>"
              "       <link id=\"3\" bandwidth=\"0.0001MBps\" latency=\"1000000us\"/>"
              "       <route src=\"Host3\" dst=\"Host1\"> <link_ctn id=\"1\"/> </route>"
              "       <route src=\"Host3\" dst=\"Host4\"> <link_ctn id=\"1\"/> </route>"
              "       <route src=\"Host4\" dst=\"Host1\"> <link_ctn id=\"1\"/> </route>"
              "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\""
              "/> </route>"
              "   </zone> "
              "</platform>";
      FILE *platform_file = fopen(platform_file_path.c_str(), "w");
      fprintf(platform_file, "%s", xml.c_str());
      fclose(platform_file);

    }

    std::string platform_file_path = "/tmp/platform.xml";
    std::unique_ptr<wrench::Workflow> workflow;

};

/**********************************************************************/
/**  SIMPLE FCFS TEST                                                **/
/**********************************************************************/

class SimpleFCFSTestWMS : public wrench::WMS {

public:
    SimpleFCFSTestWMS(BatchServiceFCFSTest *test,
                      const std::set<wrench::ComputeService *> &compute_services,
                      std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, {}, {}, nullptr, hostname,
                        "test") {
      this->test = test;
    }

private:

    BatchServiceFCFSTest *test;

    int main() {
      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      // Create 4 tasks and submit them as three various shaped jobs

      wrench::WorkflowTask *tasks[8];
      wrench::StandardJob *jobs[8];
      for (int i=0; i < 8; i++) {
        tasks[i] = this->workflow->addTask("task" + std::to_string(i), 60, 1, 1, 1.0);
        jobs[i] = job_manager->createStandardJob(tasks[i], {});
      }

      std::map<std::string, std::string>
              two_hosts_ten_cores,
              two_hosts_five_cores,
              one_hosts_five_cores,
              four_hosts_five_cores;

      two_hosts_ten_cores["-N"] = "2";
      two_hosts_ten_cores["-t"] = "2";
      two_hosts_ten_cores["-c"] = "10";

      two_hosts_five_cores["-N"] = "2";
      two_hosts_five_cores["-t"] = "2";
      two_hosts_five_cores["-c"] = "5";

      one_hosts_five_cores["-N"] = "1";
      one_hosts_five_cores["-t"] = "2";
      one_hosts_five_cores["-c"] = "5";

      four_hosts_five_cores["-N"] = "4";
      four_hosts_five_cores["-t"] = "2";
      four_hosts_five_cores["-c"] = "5";

      std::map<std::string, std::string> job_args[8] = {
              two_hosts_ten_cores,
              four_hosts_five_cores,
              two_hosts_ten_cores,
              two_hosts_ten_cores,
              four_hosts_five_cores,
              two_hosts_five_cores,
              one_hosts_five_cores,
              four_hosts_five_cores
      };

      double expected_completion_times[8] = {
              60,
              120,
              180,
              180,
              240,
              240,
              240,
              300
      };

      // Submit jobs
      try {
        for (int i=0; i < 8; i++) {
          job_manager->submitJob(jobs[i], this->test->compute_service, job_args[i]);
        }
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error(
                "Unexpected exception while submitting job"
        );
      }

      double actual_completion_times[8];
      for (int i=0; i < 8; i++) {
        // Wait for a workflow execution event
        std::unique_ptr<wrench::WorkflowExecutionEvent> event;
        try {
          event = this->workflow->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        switch (event->type) {
          case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
            actual_completion_times[i] =  this->simulation->getCurrentSimulatedDate();
            break;
          }
          default: {
            throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
          }
        }
      }

      // Check
      for (int i=0; i < 8; i++) {
        double delta = fabs(actual_completion_times[i] - expected_completion_times[i]);
        if (delta > EPSILON) {
          throw std::runtime_error("Unexpected job completion time for the job containing task " +
                                   tasks[i]->getId() +
                                   ": " +
                                   std::to_string(actual_completion_times[i]) +
                                   "(expected: " +
                                   std::to_string(expected_completion_times[i]) +
                                   ")");
        }
      }

      return 0;
    }
};

#ifdef ENABLED_BATSCHED
TEST_F(BatchServiceFCFSTest, DISABLED_SimpleFCFSTest) {
  DO_TEST_WITH_FORK(do_SimpleFCFS_test);
}
#else
TEST_F(BatchServiceFCFSTest, SimpleFCFSTest) {
  DO_TEST_WITH_FORK(do_SimpleFCFS_test);
}
#endif


void BatchServiceFCFSTest::do_SimpleFCFS_test() {


  // Create and initialize a simulation
  auto simulation = new wrench::Simulation();
  int argc = 1;
  auto argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("batch_service_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Batch Service with a FCFS scheduling algorithm
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true, simulation->getHostnameList(),
                                   nullptr,
                                   {{wrench::BatchServiceProperty::BATCH_SCHEDULING_ALGORITHM, "FCFS"}})));

  simulation->setFileRegistryService(new wrench::FileRegistryService(hostname));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new SimpleFCFSTestWMS(
                  this,  {compute_service}, hostname)));

  EXPECT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));

  EXPECT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}



