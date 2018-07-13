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

#include <istream>
#include <fstream>


#define EPSILON 0.05

XBT_LOG_NEW_DEFAULT_CATEGORY(batch_service_output_csv_file_test, "Log category for BatchServiceOutputCSVFileTest");


class BatchServiceOutputCSVFileTest : public ::testing::Test {

public:
    wrench::ComputeService *compute_service = nullptr;
    wrench::Simulation *simulation;

    void do_SimpleOutputCSVFile_test();

protected:
    BatchServiceOutputCSVFileTest() {

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
/**  SIMPLE TEST                                                **/
/**********************************************************************/

class SimpleOutputCSVFileTestWMS : public wrench::WMS {

public:
    SimpleOutputCSVFileTestWMS(BatchServiceOutputCSVFileTest *test,
                      const std::set<wrench::ComputeService *> &compute_services,
                      std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, {}, {}, nullptr, hostname,
                        "test") {
      this->test = test;
    }

private:

    BatchServiceOutputCSVFileTest *test;

    int main() {
      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      // Create 4 tasks and submit them as various shaped jobs
      wrench::WorkflowTask *tasks[8];
      wrench::StandardJob *jobs[8];
      for (int i=0; i < 8; i++) {
        tasks[i] = this->getWorkflow()->addTask("task" + std::to_string(i), 60, 1, 1, 1.0, 0);
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
          event = this->getWorkflow()->waitForNextExecutionEvent();
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

      return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceOutputCSVFileTest, SimpleOutputCSVFileTest)
#else
TEST_F(BatchServiceOutputCSVFileTest, DISABLED_SimpleOutputCSVFileTest)
#endif
{
  DO_TEST_WITH_FORK(do_SimpleOutputCSVFile_test);
}


void BatchServiceOutputCSVFileTest::do_SimpleOutputCSVFile_test() {


  // Create and initialize a simulation
  auto simulation = new wrench::Simulation();
  int argc = 1;
  auto argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("batch_service_test");

  ASSERT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = "Host1";

  // Create a Valid trace file
  std::string trace_file_path = "/tmp/batch_trace.swf";
  FILE *trace_file = fopen(trace_file_path.c_str(), "w");
  fprintf(trace_file, "1 0 -1 60 -1 -1 -1 2 60 -1\n");  // job that takes 2 procs
  fprintf(trace_file, "2 1 -1 60 -1 -1 -1 1 60 -1\n");  // job that takes 1 proc
  fclose(trace_file);


  // Create a Batch Service
  std::string output_csv_file = "/tmp/batch_log.csv";

  // Bogus output file
  ASSERT_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, {"Host1", "Host2", "Host3", "Host4"}, 0,
                                   {{wrench::BatchServiceProperty::OUTPUT_CSV_JOB_LOG, "/bogus"},
                                    {wrench::BatchServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path}})), std::invalid_argument);

  // OK output file
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, {"Host1", "Host2", "Host3", "Host4"}, 0,
                                   {{wrench::BatchServiceProperty::OUTPUT_CSV_JOB_LOG, output_csv_file},
                                    {wrench::BatchServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path}})));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(
          new SimpleOutputCSVFileTestWMS(
                  this,  {compute_service}, hostname)));

  ASSERT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));

  ASSERT_NO_THROW(simulation->launch());

  // Check the file content (not too thorough though)
  std::ifstream infile(output_csv_file);
  ASSERT_EQ((bool)infile, true);

    std::string line;
    unsigned long red_count = 0, green_count = 0;
    while (std::getline(infile, line)) {
      if (line.find("green") != std::string::npos) {
        green_count++;
      } else if (line.find("red") != std::string::npos) {
        red_count++;
      }
    }

  ASSERT_EQ(green_count, 2);
  ASSERT_EQ(red_count, 8);

  delete simulation;

  free(argv[0]);
  free(argv);
}

