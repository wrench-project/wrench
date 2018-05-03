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

#include "../include/TestWithFork.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(job_manager_test, "Log category for JobManagerTest");


class JobManagerTest : public ::testing::Test {


public:
    wrench::Simulation *simulation;

    void do_JobManagerConstructorTest_test();
    void do_JobManagerCreateJobTest_test();
    void do_JobManagerSubmitJobTest_test();


protected:
    JobManagerTest() {

      // Create the simplest workflow
      workflow = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());

      std::string xml = "<?xml version='1.0'?>"
              "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
              "<platform version=\"4.1\"> "
              "   <zone id=\"AS0\" routing=\"Full\"> "
              "       <host id=\"Host1\" speed=\"1f\" core=\"10\"/> "
              "       <host id=\"Host2\" speed=\"1f\" core=\"10\"/> "
              "       <host id=\"Host3\" speed=\"1f\" core=\"10\"/> "
              "       <host id=\"Host4\" speed=\"1f\" core=\"10\">  "
              "         <prop id=\"ram\" value=\"1024\"/> "
              "       </host>  "
              "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
              "       <link id=\"2\" bandwidth=\"0.0001MBps\" latency=\"1000000us\"/>"
              "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\"/> </route>"
              "       <route src=\"Host3\" dst=\"Host4\"> <link_ctn id=\"2\"/> </route>"
              "       <route src=\"Host1\" dst=\"Host4\"> <link_ctn id=\"2\"/> </route>"
              "   </zone> "
              "</platform>";
      // Create a four-host 10-core platform file
      FILE *platform_file = fopen(platform_file_path.c_str(), "w");
      fprintf(platform_file, "%s", xml.c_str());
      fclose(platform_file);

    }

    std::unique_ptr<wrench::Workflow> workflow;
    std::string platform_file_path = "/tmp/platform.xml";

};


/**********************************************************************/
/**  DO CONSTRUCTOR TEST                                             **/
/**********************************************************************/

class BogusStandardJobScheduler: public wrench::StandardJobScheduler {
    void scheduleTasks(const std::set<wrench::ComputeService *> &compute_services,
                       const std::map<std::string, std::vector<wrench::WorkflowTask *>> &tasks) override;

};

void BogusStandardJobScheduler::scheduleTasks(const std::set<wrench::ComputeService *> &compute_services,
                                              const std::map<std::string, std::vector<wrench::WorkflowTask *>> &tasks) {}

class BogusPilotJobScheduler: public wrench::PilotJobScheduler {
    void schedulePilotJobs(const std::set<wrench::ComputeService *> &compute_services) override;
};

void BogusPilotJobScheduler::schedulePilotJobs(const std::set<wrench::ComputeService *> &compute_services) {}

class JobManagerConstructorTestWMS : public wrench::WMS {

public:
    JobManagerConstructorTestWMS(JobManagerTest *test,
                                 std::string hostname) :
            wrench::WMS(std::unique_ptr<BogusStandardJobScheduler>(new BogusStandardJobScheduler()),
                        std::unique_ptr<BogusPilotJobScheduler>(new BogusPilotJobScheduler()),
                        {}, {}, {}, nullptr, hostname, "test") {
      this->test = test;
    }


private:

    JobManagerTest *test;

    int main() {

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      // Kill the Job Manager abruptly, just for kicks
      job_manager->kill();

      return 0;
    }
};

TEST_F(JobManagerTest, ConstructorTest) {
  DO_TEST_WITH_FORK(do_JobManagerConstructorTest_test);
}

void JobManagerTest::do_JobManagerConstructorTest_test() {

  // Create and initialize a simulation
  simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_task_test");

  simulation->init(&argc, argv);

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new JobManagerConstructorTestWMS(
                  this, hostname)));

  EXPECT_NO_THROW(wms->addWorkflow(workflow.get()));

  EXPECT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}


/**********************************************************************/
/**  DO CREATE JOB TEST                                             **/
/**********************************************************************/

class JobManagerCreateJobTestWMS : public wrench::WMS {

public:
    JobManagerCreateJobTestWMS(JobManagerTest *test,
                                 std::string hostname) :
            wrench::WMS(nullptr, nullptr,
                        {}, {}, {}, nullptr, hostname, "test") {
      this->test = test;
    }


private:

    JobManagerTest *test;

    int main() {

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      bool success;


      success = true;
      try {
        job_manager->createStandardJob((std::vector<wrench::WorkflowTask *>){nullptr}, {});
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to create a standard job with a nullptr task in it");
      }

      success = true;
      try {
        job_manager->createStandardJob({}, {});
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to create a standard job with nothing in it");
      }

      success = true;
      try {
        job_manager->createPilotJob(10, 10, -12, 10);
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to create a standard job with a negative ram per host");
      }

      success = true;
      try {
        job_manager->createPilotJob(10, 10, 10, -12);
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to create a standard job with a negative duration");
      }

      wrench::WorkflowTask *t1 = workflow->addTask("t1", 1.0, 1, 1, 1.0, 0.0);
      wrench::WorkflowTask *t2 = workflow->addTask("t2", 1.0, 1, 1, 1.0, 0.0);
      wrench::WorkflowFile *f = workflow->addFile("f", 100);
      t1->addOutputFile(f);
      t2->addInputFile(f);

      // Create an "ok" job
      success = true;
      try {
        job_manager->createStandardJob({t1, t2}, {});
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (!success) {
        throw std::runtime_error("Should be able to create a standard job with two dependent tasks");
      }

      // Create a "not ok" job
      success = true;
      try {
        job_manager->createStandardJob((std::vector<wrench::WorkflowTask*>){t2}, {});
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to create a standard job with a not-self-contained task");
      }

      return 0;
    }
};

TEST_F(JobManagerTest, CreateJobTest) {
  DO_TEST_WITH_FORK(do_JobManagerCreateJobTest_test);
}

void JobManagerTest::do_JobManagerCreateJobTest_test() {

  // Create and initialize a simulation
  simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_task_test");

  simulation->init(&argc, argv);

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new JobManagerCreateJobTestWMS(
                  this, hostname)));

  EXPECT_NO_THROW(wms->addWorkflow(workflow.get()));

  EXPECT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}


/**********************************************************************/
/**  DO SUBMIT JOB TEST                                              **/
/**********************************************************************/

class JobManagerSubmitJobTestWMS : public wrench::WMS {

public:
    JobManagerSubmitJobTestWMS(JobManagerTest *test,
                               std::string hostname) :
            wrench::WMS(nullptr, nullptr,
                        {}, {}, {}, nullptr, hostname, "test") {
      this->test = test;
    }


private:

    JobManagerTest *test;

    int main() {

      // Submit a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      bool success;

      success = true;
      try {
        job_manager->submitJob(nullptr, (wrench::ComputeService *)(1234), {});
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to submit a job with a nullptr job");
      }

      success = true;
      try {
        job_manager->submitJob((wrench::WorkflowJob *)(1234), nullptr, {});
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to submit a job with a nullptr compute service");
      }

      success = true;
      try {
        job_manager->terminateJob(nullptr);
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to terminate a nullptr job");
      }

      success = true;
      try {
        job_manager->forgetJob(nullptr);
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to forget a nullptr job");
      }




      return 0;
    }
};

TEST_F(JobManagerTest, SubmitJobTest) {
  DO_TEST_WITH_FORK(do_JobManagerSubmitJobTest_test);
}

void JobManagerTest::do_JobManagerSubmitJobTest_test() {

  // Submit and initialize a simulation
  simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_task_test");

  simulation->init(&argc, argv);

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Submit a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new JobManagerSubmitJobTestWMS(
                  this, hostname)));

  EXPECT_NO_THROW(wms->addWorkflow(workflow.get()));


  EXPECT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}