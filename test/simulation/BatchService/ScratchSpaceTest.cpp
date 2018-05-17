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

XBT_LOG_NEW_DEFAULT_CATEGORY(scratch_service_test, "Log category for ScratchServiceTest");


class ScratchSpaceTest : public ::testing::Test {

public:
    wrench::StorageService *storage_service1 = nullptr;
    wrench::StorageService *storage_service2 = nullptr;
    wrench::ComputeService *compute_service = nullptr;
    wrench::Simulation *simulation;


    void do_SimpleScratchSpace_test();

protected:
    ScratchSpaceTest() {

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
/**  ONE STANDARD JOB SUBMISSION TASK SIMULATION TEST ON ONE HOST    **/
/**********************************************************************/

class SimpleScratchSpaceTestWMS : public wrench::WMS {

public:
    SimpleScratchSpaceTestWMS(ScratchSpaceTest *test,
                                    const std::set<wrench::ComputeService *> &compute_services,
                                    std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, {}, {}, nullptr, hostname,
                        "test") {
      this->test = test;
    }

private:

    ScratchSpaceTest *test;

    int main() {
      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      {
//        // Create a sequential task that lasts one min and requires 2 cores
//        wrench::WorkflowTask *task = this->workflow->addTask("task", 60, 2, 2, 1.0);
//        task->addInputFile(this->workflow->getFileById("input_file"));
//        task->addOutputFile(this->workflow->getFileById("output_file"));
//
//
//        // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)
//
//        wrench::StandardJob *job = job_manager->createStandardJob(
//                {task},
//                {
//                        {*(task->getInputFiles().begin()),  this->test->storage_service1},
//                        {*(task->getOutputFiles().begin()), this->test->storage_service1}
//                },
//                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
//                        this->workflow->getFileById("input_file"), this->test->storage_service1,
//                        this->test->storage_service2)},
//                {},
//                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(this->workflow->getFileById("input_file"),
//                                                                              this->test->storage_service2)});
//
//        std::map<std::string, std::string> batch_job_args;
//        batch_job_args["-N"] = "2";
//        batch_job_args["-t"] = "5"; //time in minutes
//        batch_job_args["-c"] = "4"; //number of cores per node
//        try {
//          job_manager->submitJob(job, this->test->compute_service, batch_job_args);
//        } catch (wrench::WorkflowExecutionException &e) {
//          throw std::runtime_error(
//                  "Exception: " + std::string(e.what())
//          );
//        }
//
//
//        // Wait for a workflow execution event
//        std::unique_ptr<wrench::WorkflowExecutionEvent> event;
//        try {
//          event = this->workflow->waitForNextExecutionEvent();
//        } catch (wrench::WorkflowExecutionException &e) {
//          throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
//        }
//        switch (event->type) {
//          case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
//            // success, do nothing for now
//            break;
//          }
//          default: {
//            throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
//          }
//        }
//        this->workflow->removeTask(task);
      }

      return 0;
    }
};

TEST_F(ScratchSpaceTest, SimpleScratchSpaceTest) {
  DO_TEST_WITH_FORK(do_SimpleScratchSpace_test);
}


void ScratchSpaceTest::do_SimpleScratchSpace_test() {


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

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));


  // Create a Batch Service with a bogus scheduling algorithm
  ASSERT_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true, simulation->getHostnameList(),
                                   storage_service1,
                                   {{wrench::BatchServiceProperty::BATCH_SCHEDULING_ALGORITHM, "BOGUS"}})), std::invalid_argument);


  // Create a Batch Service
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true, simulation->getHostnameList(),
                                   storage_service1, {})));

  simulation->add(new wrench::FileRegistryService(hostname));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new SimpleScratchSpaceTestWMS(
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

