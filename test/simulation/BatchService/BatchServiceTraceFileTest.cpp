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
#include <wrench/util/TraceFileLoader.h>
#include "wrench/workflow/job/PilotJob.h"

#include "../../include/TestWithFork.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(batch_service_trace_file_test, "Log category for BatchServiceTest");


class BatchServiceTest : public ::testing::Test {

public:
    wrench::StorageService *storage_service1 = nullptr;
    wrench::StorageService *storage_service2 = nullptr;
    wrench::ComputeService *compute_service = nullptr;
    wrench::Simulation *simulation;

    void do_BatchTraceFileReplayTest_test();

    void do_WorkloadTraceFileTest_test();

protected:
    BatchServiceTest() {

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
              "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
              "       <link id=\"2\" bandwidth=\"0.0001MBps\" latency=\"1000000us\"/>"
              "       <link id=\"3\" bandwidth=\"0.0001MBps\" latency=\"1000000us\"/>"
              "       <route src=\"Host3\" dst=\"Host1\"> <link_ctn id=\"2\"/> </route>"
              "       <route src=\"Host4\" dst=\"Host1\"> <link_ctn id=\"2\"/> </route>"
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
/**  BATCH TRACE FILE REPLAY SIMULATION TEST **/
/**********************************************************************/

class BatchTraceFileReplayTestWMS : public wrench::WMS {

public:
    BatchTraceFileReplayTestWMS(BatchServiceTest *test,
                                const std::set<wrench::ComputeService *> &compute_services,
                                const std::set<wrench::StorageService *> &storage_services,
                                std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services,
                        {}, nullptr, hostname, "test") {
      this->test = test;
    }


private:

    BatchServiceTest *test;

    int main() {
      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();


      // Submit the jobs
      unsigned long num_submitted_jobs = 0;
      {
        //Let's load the trace file
        std::vector<std::tuple<std::string, double, double, double, double, unsigned int>>
                trace_file_jobs = wrench::TraceFileLoader::loadFromTraceFile("test/trace_files/NASA-iPSC-1993-3.swf",
                                                                             0);
        for (auto const &job : trace_file_jobs) {
          double sub_time = std::get<1>(job);
          double curtime = wrench::S4U_Simulation::getClock();
          double sleeptime = sub_time - curtime;
          if (sleeptime > 0)
            wrench::S4U_Simulation::sleep(sleeptime);
          std::string id = std::get<0>(job);
          double flops = std::get<2>(job);
          double requested_flops = std::get<3>(job);
          double requested_ram = std::get<4>(job);
          int num_nodes = std::get<5>(job);
          int min_num_cores = 10;
          int max_num_cores = 10;
          double parallel_efficiency = 1.0;
          // Ignore jobs that are too big
          if (num_nodes > 4) {
            continue;
          }
//          std::cerr << "SUBMITTING " << "sub="<< sub_time << "num_nodes=" << num_nodes << " id="<<id << " flops="<<flops << " rflops="<<requested_flops << " ram="<<requested_ram << "\n";
          wrench::WorkflowTask *task = this->workflow->addTask(id, flops, min_num_cores, max_num_cores,
                                                               parallel_efficiency);

          wrench::StandardJob *standard_job = job_manager->createStandardJob(
                  {task},
                  {},
                  {},
                  {},
                  {});


          std::map<std::string, std::string> batch_job_args;
          batch_job_args["-N"] = std::to_string(num_nodes);
          batch_job_args["-t"] = std::to_string(requested_flops);
          batch_job_args["-c"] = std::to_string(max_num_cores); //use all cores
          try {
            job_manager->submitJob(standard_job, this->test->compute_service, batch_job_args);
          } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Failed to submit a job");
          }

          num_submitted_jobs++;

        }
      }

      // Wait for the execution events
      for (unsigned long i=0; i < num_submitted_jobs; i++) {
        // Wait for a workflow execution event
        std::unique_ptr<wrench::WorkflowExecutionEvent> event;
        try {
          event = this->workflow->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        switch (event->type) {
          case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
            // success, do nothing for now
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

TEST_F(BatchServiceTest, BatchTraceFileReplayTest) {
  DO_TEST_WITH_FORK(do_BatchTraceFileReplayTest_test);
}

void BatchServiceTest::do_BatchTraceFileReplayTest_test() {

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

  // Create a Batch Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true,
                                   simulation->getHostnameList(), storage_service1, {
                                   })));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new BatchTraceFileReplayTestWMS(
                  this, {compute_service}, {}, hostname)));

  EXPECT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));

  // Running a "run a single task" simulation
  // Note that in these tests the WMS creates workflow tasks, which a user would
  // of course not be likely to do
  EXPECT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}

/**********************************************************************/
/**  WORKLOAD TRACE FILE TEST                                        **/
/**********************************************************************/

class WorkloadTraceFileTestWMS : public wrench::WMS {

public:
    WorkloadTraceFileTestWMS(BatchServiceTest *test,
                             const std::set<wrench::ComputeService *> &compute_services,
                             const std::set<wrench::StorageService *> &storage_services,
                             std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr,
                        hostname, "test") {
      this->test = test;
    }


private:

    BatchServiceTest *test;

    int main() {
      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      return 0;
    }
};

TEST_F(BatchServiceTest, WorkloadTraceFileTest) {
  DO_TEST_WITH_FORK(do_WorkloadTraceFileTest_test);
}


void BatchServiceTest::do_WorkloadTraceFileTest_test() {

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

  // Create a Batch Service with a non-existing workload trace file, which should throw
  ASSERT_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true,
                                   simulation->getHostnameList(), storage_service1,
                                   {{wrench::BatchServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, "/tmp/not_there"}}
          )), std::invalid_argument);

  std::string trace_file_path = "/tmp/swf_trace";
  FILE *trace_file;

  // Create an invalid trace file
  trace_file = fopen(trace_file_path.c_str(), "w");
  fprintf(trace_file, "1 0 -1 3600 -1 -1 -1 4 3600 ");     // MISSING FIELD
  fprintf(trace_file, "1 0 -1 3600 -1 -1 -1 2 3600 -1 ");  // job that takes half the machine
  fclose(trace_file);

  // Create a Batch Service with a bogus trace file, which should throw
  ASSERT_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true,
                                   simulation->getHostnameList(), storage_service1,
                                   {{wrench::BatchServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path}}
          )), std::invalid_argument);

  // Create another invalid trace file
  trace_file = fopen(trace_file_path.c_str(), "w");
  fprintf(trace_file, "1 0 -1 3600 -1 -1 -1 4 hello -1");     // INVALID FIELD
  fprintf(trace_file, "1 0 -1 3600 -1 -1 -1 2 3600 -1 ");  // job that takes half the machine
  fclose(trace_file);

  // Create a Batch Service with a bogus trace file, which should throw
  ASSERT_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true,
                                   simulation->getHostnameList(), storage_service1,
                                   {{wrench::BatchServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path}}
          )), std::invalid_argument);

  // Create another invalid trace file
  trace_file = fopen(trace_file_path.c_str(), "w");
  fprintf(trace_file, "1 0 -1 3600 -1 -1 -1 -1 3600 -1");     // MISSING NUM PROCS
  fprintf(trace_file, "1 0 -1 3600 -1 -1 -1 2 3600 -1 ");  // job that takes half the machine
  fclose(trace_file);


  // Create a Batch Service with a bogus trace file, which should throw
  ASSERT_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true,
                                   simulation->getHostnameList(), storage_service1,
                                   {{wrench::BatchServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path}}
          )), std::invalid_argument);

  // Create another invalid trace file
  trace_file = fopen(trace_file_path.c_str(), "w");
  fprintf(trace_file, "1 0 -1 -1 -1 -1 -1 4 -1 -1");     // MISSING TIME
  fprintf(trace_file, "1 0 -1 3600 -1 -1 -1 2 3600 -1 ");  // job that takes half the machine
  fclose(trace_file);


  // Create a Batch Service with a non-existing workload trace file, which should throw
  ASSERT_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true,
                                   simulation->getHostnameList(), storage_service1,
                                   {{wrench::BatchServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path}}
          )), std::invalid_argument);


  // Create a Valid trace file
  trace_file = fopen(trace_file_path.c_str(), "w");
  fprintf(trace_file, "1 0 -1 3600 -1 -1 -1 4 3600 -1 ");  // job that takes the whole machine
  fprintf(trace_file, "2 1 -1 3600 -1 -1 -1 2 3600 -1 ");  // job that takes half the machine
  fclose(trace_file);

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(new WorkloadTraceFileTestWMS(
          this, {compute_service}, {}, hostname)));

  EXPECT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));

  // Running a "run a single task" simulation
  // Note that in these tests the WMS creates workflow tasks, which a user would
  // of course not be likely to do
  EXPECT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}

