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
#include <unistd.h>

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
              "       <route src=\"Host3\" dst=\"Host1\"> <link_ctn id=\"1\"/> </route>"
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
        std::vector<std::tuple<std::string, double, double, double, double, unsigned int>> trace_file_jobs;
        //Let's load the trace file
        try {
          trace_file_jobs = wrench::TraceFileLoader::loadFromTraceFile("../test/trace_files/NASA-iPSC-1993-3.swf", 0);
        } catch (std::invalid_argument &e) {
          // Ignore and try alternate path
          trace_file_jobs = wrench::TraceFileLoader::loadFromTraceFile("test/trace_files/NASA-iPSC-1993-3.swf", 0);
          // If this doesn't work, then we have a problem, so we simply let the exception be uncaught
        }


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
          // TODO: Should we use the "requested_ram" instead of 0 below?
          wrench::WorkflowTask *task = this->getWorkflow()->addTask(id, flops, min_num_cores, max_num_cores,
                                                               parallel_efficiency, 0);

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
          event = this->getWorkflow()->waitForNextExecutionEvent();
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

  ASSERT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = "Host1";

  // Create a Batch Service
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, 
                                   {"Host1", "Host2", "Host3", "Host4"}, {})));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(
          new BatchTraceFileReplayTestWMS(
                  this, {compute_service}, {}, hostname)));

  ASSERT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));

  // Running a "run a single task" simulation
  // Note that in these tests the WMS creates workflow tasks, which a user would
  // of course not be likely to do
  ASSERT_NO_THROW(simulation->launch());

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

      wrench::Simulation::sleep(10);
      // At this point, using the FCFS algorithm, a 2-node 30-min job should complete around t=1.5 hours
      // and 4-node 30-min job should complete around t=2.5 hours

      std::vector<wrench::WorkflowTask *> tasks;
      std::map<std::string, std::string> batch_job_args;

      // Create and submit a job that needs 2 nodes and 30 minutes
      for (size_t i = 0; i < 2; i++) {
        double time_fudge = 1; // 1 second seems to make it all work!
        double task_flops = 10 * (1 * (1800 - time_fudge));
        int num_cores = 10;
        double parallel_efficiency = 1.0;
        tasks.push_back(this->getWorkflow()->addTask("test_job_1_task_" + std::to_string(i),
                                          task_flops,
                                          num_cores, num_cores, parallel_efficiency,
                                          0.0));
      }

      // Create a Standard Job with only the tasks
      wrench::StandardJob *standard_job_2_nodes;
      standard_job_2_nodes = job_manager->createStandardJob(tasks, {});

      // Create the batch-specific argument
      batch_job_args["-N"] = std::to_string(2); // Number of nodes/tasks
      batch_job_args["-t"] = std::to_string(1800); // Time in minutes (at least 1 minute)
      batch_job_args["-c"] = std::to_string(10); //number of cores per task

      // Submit this job to the batch service
      job_manager->submitJob(standard_job_2_nodes, *(this->getAvailableComputeServices().begin()), batch_job_args);


      // Create and submit a job that needs 2 nodes and 30 minutes
      tasks.clear();
      for (size_t i = 0; i < 4; i++) {
        double time_fudge = 1; // 1 second seems to make it all work!
        double task_flops = 10 * (1 * (1800 - time_fudge));
        int num_cores = 10;
        double parallel_efficiency = 1.0;
        tasks.push_back(this->getWorkflow()->addTask("test_job_2_task_" + std::to_string(i),
                                          task_flops,
                                          num_cores, num_cores, parallel_efficiency,
                                          0.0));
      }


      // Create a Standard Job with only the tasks
      wrench::StandardJob *standard_job_4_nodes;
      standard_job_4_nodes = job_manager->createStandardJob(tasks, {});

      // Create the batch-specific argument
      batch_job_args.clear();
      batch_job_args["-N"] = std::to_string(4); // Number of nodes/tasks
      batch_job_args["-t"] = std::to_string(1800); // Time in minutes (at least 1 minute)
      batch_job_args["-c"] = std::to_string(10); //number of cores per task

      // Submit this job to the batch service
      job_manager->submitJob(standard_job_4_nodes, *(this->getAvailableComputeServices().begin()), batch_job_args);


      // Wait for the two execution events
      for (auto job : {standard_job_2_nodes, standard_job_4_nodes}) {
        // Wait for the workflow execution event
        WRENCH_INFO("Waiting for job completion of job %s", job->getName().c_str());
        std::unique_ptr<wrench::WorkflowExecutionEvent> event;
        try {
          event = this->getWorkflow()->waitForNextExecutionEvent();
          switch (event->type) {
            case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
              if (dynamic_cast<wrench::StandardJobCompletedEvent*>(event.get())->standard_job != job) {
                throw std::runtime_error("Wrong job completion order: got " +
                                                 dynamic_cast<wrench::StandardJobCompletedEvent*>(event.get())->standard_job->getName() + " but expected " + job->getName());
              }
              break;
            }
            default: {
              throw std::runtime_error(
                      "Unexpected workflow execution event: " + std::to_string((int) (event->type)));
            }
          }
        } catch (wrench::WorkflowExecutionException &e) {
          //ignore (network error or something)
        }

        double completion_time = this->simulation->getCurrentSimulatedDate();
        double expected_completion_time;
        if (job == standard_job_2_nodes) {
          expected_completion_time = 3600  + 1800;
        } else if (job == standard_job_4_nodes) {
          expected_completion_time = 3600 * 2 + 1800;
        } else {
          throw std::runtime_error("Phantom job completion!");
        }
        double delta = fabs(expected_completion_time - completion_time);
        double tolerance = 5;
        if (delta > tolerance) {
          throw std::runtime_error("Unexpected job completion time for job " + job->getName() + ": " +
                                   std::to_string(completion_time) + " (expected: " + std::to_string(expected_completion_time) + ")");
        }

      }
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

  ASSERT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = "Host1";

  // Create a Batch Service with a non-existing workload trace file, which should throw
  ASSERT_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, 
                                   {"Host1", "Host2", "Host3", "Host4"}, 0,
                                   {{wrench::BatchServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, "/not_there"}}
          )), std::invalid_argument);

  std::string trace_file_path = "/tmp/swf_trace";
  FILE *trace_file;

  // Create an invalid trace file
  trace_file = fopen(trace_file_path.c_str(), "w");
  fprintf(trace_file, "1 0 -1 3600 -1 -1 -1 4 3600\n");     // MISSING FIELD
  fprintf(trace_file, "1 0 -1 3600 -1 -1 -1 2 3600 -1\n");  // job that takes half the machine
  fclose(trace_file);

  // Create a Batch Service with a bogus trace file, which should throw
  ASSERT_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, 
                                   {"Host1", "Host2", "Host3", "Host4"}, 0,
                                   {{wrench::BatchServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path}}
          )), std::invalid_argument);

  // Create another invalid trace file
  trace_file  = fopen(trace_file_path.c_str(), "w");
  fprintf(trace_file, "1 0 -1 3600 -1 -1 -1 4 hello -1\n");     // INVALID FIELD
  fprintf(trace_file, "1 0 -1 3600 -1 -1 -1 2 3600 -1\n");  // job that takes half the machine
  fclose(trace_file);

  // Create a Batch Service with a bogus trace file, which should throw
  ASSERT_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, 
                                   {"Host1", "Host2", "Host3", "Host4"}, 0,
                                   {{wrench::BatchServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path}}
          )), std::invalid_argument);

  // Create another invalid trace file
  trace_file = fopen(trace_file_path.c_str(), "w");
  fprintf(trace_file, "1 0 -1 3600 -1 -1 -1 -1 3600 -1\n");     // MISSING NUM PROCS
  fprintf(trace_file, "1 0 -1 3600 -1 -1 -1 2 3600 -1\n");  // job that takes half the machine
  fclose(trace_file);


  // Create a Batch Service with a bogus trace file, which should throw
  ASSERT_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, 
                                   {"Host1", "Host2", "Host3", "Host4"}, 0,
                                   {{wrench::BatchServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path}}
          )), std::invalid_argument);

  // Create another invalid trace file
  trace_file = fopen(trace_file_path.c_str(), "w");
  fprintf(trace_file, "1 0 -1 -1 -1 -1 -1 4 -1 -1\n");     // MISSING TIME
  fprintf(trace_file, "1 0 -1 3600 -1 -1 -1 2 3600 -1\n");  // job that takes half the machine
  fclose(trace_file);


  // Create a Batch Service with a non-existing workload trace file, which should throw
  ASSERT_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, 
                                   {"Host1", "Host2", "Host3", "Host4"}, 0, 
                                   {{wrench::BatchServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path}}
          )), std::invalid_argument);


  // Create a Valid trace file
  trace_file = fopen(trace_file_path.c_str(), "w");
  fprintf(trace_file, "1 0 -1 3600 -1 -1 -1 4 3600 -1\n");  // job that takes the whole machine
  fprintf(trace_file, "2 1 -1 3600 -1 -1 -1 2 3600 -1\n");  // job that takes half the machine
  fclose(trace_file);


  // Create a Batch Service with a non-existing workload trace file, which should throw
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, 
                                   {"Host1", "Host2", "Host3", "Host4"}, 0,
                                   {{wrench::BatchServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path}}
          )));


  // Create a WMS
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(new WorkloadTraceFileTestWMS(
          this, {compute_service}, {}, hostname)));

  ASSERT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));

  // Running a "run a single task" simulation
  // Note that in these tests the WMS creates workflow tasks, which a user would
  // of course not be likely to do
  ASSERT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}

