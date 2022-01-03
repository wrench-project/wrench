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
#include <gtest/gtest.h>
#include <wrench/services/compute/batch/BatchComputeService.h>
#include <wrench/services/compute/batch/BatchComputeServiceMessage.h>
#include <wrench/job/PilotJob.h>

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"

#include <istream>
#include <fstream>


#define EPSILON 0.05

WRENCH_LOG_CATEGORY(batch_service_output_csv_file_test, "Log category for BatchServiceOutputCSVFileTest");


class BatchServiceOutputCSVFileTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::Workflow> workflow;

    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;
    std::shared_ptr<wrench::Simulation> simulation;

    void do_SimpleOutputCSVFile_test();

protected:

    ~BatchServiceOutputCSVFileTest() {
        workflow->clear();
    }

    BatchServiceOutputCSVFileTest() {

      // Create the simplest workflow
      workflow = wrench::Workflow::createWorkflow();

      // Create a four-host 10-core platform file
      std::string xml = "<?xml version='1.0'?>"
              "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
              "<platform version=\"4.1\"> "
              "   <zone id=\"AS0\" routing=\"Full\"> "
              "       <host id=\"Host1\" speed=\"1f\" core=\"10\"/> "
              "       <host id=\"Host2\" speed=\"1f\" core=\"10\"/> "
              "       <host id=\"Host3\" speed=\"1f\" core=\"10\"/> "
              "       <host id=\"Host4\" speed=\"1f\" core=\"10\"/> "
              "       <link id=\"1\" bandwidth=\"50000GBps\" latency=\"0us\"/>"
              "       <link id=\"2\" bandwidth=\"50000GBps\" latency=\"0us\"/>"
              "       <link id=\"3\" bandwidth=\"50000GBps\" latency=\"0us\"/>"
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

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";

};

/**********************************************************************/
/**  SIMPLE TEST                                                **/
/**********************************************************************/

class SimpleOutputCSVFileTestWMS : public wrench::ExecutionController {

public:
    SimpleOutputCSVFileTestWMS(BatchServiceOutputCSVFileTest *test,
                      std::string hostname) :
            wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:

    BatchServiceOutputCSVFileTest *test;

    int main() {
      // Create a job manager
      auto job_manager = this->createJobManager();

      // Create 4 tasks and submit them as various shaped jobs
      std::shared_ptr<wrench::WorkflowTask> tasks[8];
      std::shared_ptr<wrench::StandardJob> jobs[8];
      for (int i=0; i < 8; i++) {
        tasks[i] = this->test->workflow->addTask("task1" + std::to_string(i), 60, 1, 1, 0);
        jobs[i] = job_manager->createStandardJob(tasks[i]);
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

      // Submit jobs
      try {
        for (int i=0; i < 8; i++) {
          job_manager->submitJob(jobs[i], this->test->compute_service, job_args[i]);
        }
      } catch (wrench::ExecutionException &e) {
        throw std::runtime_error(
                "Unexpected exception while submitting job"
        );
      }

      for (int i=0; i < 8; i++) {
        // Wait for a workflow execution event
        std::shared_ptr<wrench::ExecutionEvent> event;
        try {
          event = this->waitForNextEvent();
        } catch (wrench::ExecutionException &e) {
          throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
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
  auto simulation = wrench::Simulation::createSimulation();
  int argc = 1;
  auto argv = (char **) calloc(argc, sizeof(char *));
  argv[0] = strdup("unit_test");

  ASSERT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = "Host1";

  // Create a Valid trace file
  std::string trace_file_path = UNIQUE_TMP_PATH_PREFIX + "batch_trace.swf";
  FILE *trace_file = fopen(trace_file_path.c_str(), "w");
  fprintf(trace_file, "1 0 -1 60 -1 -1 -1 2 60 -1\n");  // job that takes 2 procs
  fprintf(trace_file, "2 1 -1 60 -1 -1 -1 1 60 -1\n");  // job that takes 1 proc
  fclose(trace_file);


  // Create a Batch Service
  std::string output_csv_file = UNIQUE_TMP_PATH_PREFIX + "batch_log.csv";

  // Bogus output file
  ASSERT_THROW(compute_service = simulation->add(
          new wrench::BatchComputeService(hostname, {"Host1", "Host2", "Host3", "Host4"}, "",
                                   {{wrench::BatchComputeServiceProperty::OUTPUT_CSV_JOB_LOG, "/bogus"},
                                    {wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path}})), std::invalid_argument);

  // OK output file
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::BatchComputeService(hostname, {"Host1", "Host2", "Host3", "Host4"}, "",
                                   {{wrench::BatchComputeServiceProperty::OUTPUT_CSV_JOB_LOG, output_csv_file},
                                    {wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path}})));

  // Create a WMS
  std::shared_ptr<wrench::ExecutionController> wms = nullptr;;
  ASSERT_NO_THROW(wms = simulation->add(
          new SimpleOutputCSVFileTestWMS(
                  this, hostname)));

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



    for (int i=0; i < argc; i++)
        free(argv[i]);
  free(argv);
}

