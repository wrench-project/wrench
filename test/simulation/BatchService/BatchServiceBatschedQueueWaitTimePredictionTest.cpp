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

XBT_LOG_NEW_DEFAULT_CATEGORY(batch_service_queue_wait_time_prediction_test, "Log category for BatchServiceTest");


class BatchServiceTest : public ::testing::Test {

public:
    wrench::StorageService *storage_service1 = nullptr;
    wrench::StorageService *storage_service2 = nullptr;
    wrench::ComputeService *compute_service = nullptr;
    wrench::Simulation *simulation;


    void do_BatchJobBrokenEstimateWaitingTimeTest_test();
    
    void do_BatchJobBasicEstimateWaitingTimeTest_test();

    void do_BatchJobEstimateWaitingTimeTest_test();

    void do_BatchJobLittleComplexEstimateWaitingTimeTest_test();


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
/**  BROKEN QUERY/ANSWER BATCH_JOB_ESTIMATE_WAITING_TIME **/
/**********************************************************************/

class BatchJobBrokenEstimateWaitingTimeTestWMS : public wrench::WMS {

public:
    BatchJobBrokenEstimateWaitingTimeTestWMS(BatchServiceTest *test,
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

      {
        // Create a sequential task that lasts one min and requires 2 cores
        wrench::WorkflowTask *task = this->workflow->addTask("task", 299, 1, 1, 1.0);
        task->addInputFile(this->workflow->getFileById("input_file"));
        task->addOutputFile(this->workflow->getFileById("output_file"));


        // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

        wrench::StandardJob *job = job_manager->createStandardJob(
                {task},
                {
                        {*(task->getInputFiles().begin()),  this->test->storage_service1},
                        {*(task->getOutputFiles().begin()), this->test->storage_service1}
                },
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                        this->workflow->getFileById("input_file"), this->test->storage_service1,
                        this->test->storage_service2)},
                {},
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(this->workflow->getFileById("input_file"),
                                                                              this->test->storage_service2)});

        std::map<std::string, std::string> batch_job_args;
        batch_job_args["-N"] = "4";
        batch_job_args["-t"] = "5"; //time in minutes
        batch_job_args["-c"] = "1"; //number of cores per node
        try {
          job_manager->submitJob(job, this->test->compute_service, batch_job_args);
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error(
                  "Got some exception"
          );
        }
        double first_job_running = this->simulation->getCurrentSimulatedDate();

        auto *batch_service = dynamic_cast<wrench::BatchService *>((*this->compute_services.begin()));
        std::string job_id = "my_tentative_job";
        unsigned int nodes = 2;
        double walltime_seconds = 1000;
        //std::tuple<std::string,unsigned int,double> my_job = {job_id,nodes,walltime_seconds};
        std::tuple<std::string,unsigned int,unsigned int, double> my_job = std::make_tuple(job_id,nodes,1, walltime_seconds);
        std::set<std::tuple<std::string,unsigned int,unsigned int, double>> set_of_jobs = {my_job};

        std::map<std::string,double> jobs_estimated_waiting_time;

        bool success = true;
        try {
          jobs_estimated_waiting_time = batch_service->getQueueWaitingTimeEstimate(set_of_jobs);
        } catch (wrench::WorkflowExecutionException &e) {
          success = false;
          if (e.getCause()->getCauseType() != wrench::FailureCause::FUNCTIONALITY_NOT_AVAILABLE) {
            throw std::runtime_error("Got an exception, as expected, but not the right failure cause");
          }
          auto real_cause = (wrench::FunctionalityNotAvailable *) e.getCause().get();
          if (real_cause->getService() != batch_service) {
            throw std::runtime_error(
                    "Got the expected exception, but the failure cause does not point to the right service");
          }
        }
        if (success) {
          throw std::runtime_error("Should not be able to get a queue waiting time estimate");
        }

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
        this->workflow->removeTask(task);

      }


      return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceTest, BatchJobBrokenEstimateWaitingTimeTest) {
  DO_TEST_WITH_FORK(do_BatchJobBrokenEstimateWaitingTimeTest_test);
}

#else
TEST_F(BatchServiceTest, DISABLED_BatchJobBrokenEstimateWaitingTimeTest) {
  DO_TEST_WITH_FORK(do_BatchJobBrokenEstimateWaitingTimeTest_test);
}
#endif


void BatchServiceTest::do_BatchJobBrokenEstimateWaitingTimeTest_test() {

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

  // Create a Batch Service
//  #ifdef ENABLE_BATSCHED
  EXPECT_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true,
                                   simulation->getHostnameList(), storage_service1, {
                                           {wrench::BatchServiceProperty::BATCH_SCHEDULING_ALGORITHM, "BOGUS"}
                                   })), std::invalid_argument);
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true,
                                   simulation->getHostnameList(), storage_service1, {
                                           {wrench::BatchServiceProperty::BATCH_SCHEDULING_ALGORITHM, "easy_bf"}
                                   })));
//  #else
//  EXPECT_NO_THROW(compute_service = simulation->add(
//          new wrench::BatchService(hostname, true, true,
//                                   simulation->getHostnameList(), storage_service1, {
//                                           {wrench::BatchServiceProperty::BATCH_SCHEDULING_ALGORITHM, "FCFS"}
//                                   })));
//  #endif


  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(new BatchJobBrokenEstimateWaitingTimeTestWMS(
          this, {compute_service}, {storage_service1, storage_service2}, hostname)));

  EXPECT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));

//  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
//          new wrench::FileRegistryService(hostname));

  simulation->add(new wrench::FileRegistryService(hostname));

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


/**********************************************************************/
/**  BASIC QUERY/ANSWER BATCH_JOB_ESTIMATE_WAITING_TIME **/
/**********************************************************************/

class BatchJobBasicEstimateWaitingTimeTestWMS : public wrench::WMS {

public:
    BatchJobBasicEstimateWaitingTimeTestWMS(BatchServiceTest *test,
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

      {

        // Create a sequential task that lasts one min and requires 2 cores
        wrench::WorkflowTask *task = this->workflow->addTask("task", 299, 1, 1, 1.0);
        task->addInputFile(this->workflow->getFileById("input_file"));
        task->addOutputFile(this->workflow->getFileById("output_file"));


        // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

        wrench::StandardJob *job = job_manager->createStandardJob(
                {task},
                {
                        {*(task->getInputFiles().begin()),  this->test->storage_service1},
                        {*(task->getOutputFiles().begin()), this->test->storage_service1}
                },
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                        this->workflow->getFileById("input_file"), this->test->storage_service1,
                        this->test->storage_service2)},
                {},
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(this->workflow->getFileById("input_file"),
                                                                              this->test->storage_service2)});

        std::map<std::string, std::string> batch_job_args;
        batch_job_args["-N"] = "4";
        batch_job_args["-t"] = "5"; //time in minutes
        batch_job_args["-c"] = "1"; //number of cores per node
        try {
          job_manager->submitJob(job, this->test->compute_service, batch_job_args);
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error(
                  "Got some exception"
          );
        }
        double first_job_running = this->simulation->getCurrentSimulatedDate();

        auto *batch_service = dynamic_cast<wrench::BatchService *>((*this->compute_services.begin()));
        std::string job_id = "my_tentative_job";
        unsigned int nodes = 2;
        double walltime_seconds = 1000;
        //std::tuple<std::string,unsigned int,double> my_job = {job_id,nodes,walltime_seconds};
        std::tuple<std::string,unsigned int,unsigned int,double> my_job = std::make_tuple(job_id,nodes,1,walltime_seconds);
        std::set<std::tuple<std::string,unsigned int,unsigned int, double>> set_of_jobs = {my_job};

          std::map<std::string,double> jobs_estimated_waiting_time;
//        #ifdef ENABLE_BATSCHED
        try {
          jobs_estimated_waiting_time = batch_service->getQueueWaitingTimeEstimate(set_of_jobs);
        } catch (std::runtime_error &e) {
          throw std::runtime_error("Exception while getting queue waiting time estimate: " + std::string(e.what()));
        }
        double expected_wait_time = 300 - first_job_running;
        double tolerance = 1; // in seconds
        double delta = fabs(expected_wait_time - (jobs_estimated_waiting_time[job_id] - tolerance));
        if (delta > 1) {
          throw std::runtime_error("Estimated queue wait time incorrect (expected: " + std::to_string(expected_wait_time) + ", got: " + std::to_string(jobs_estimated_waiting_time[job_id]) + ")");
        }

//        #else
//        bool success = true;
//        try {
//          jobs_estimated_waiting_time = batch_service->getQueueWaitingTimeEstimate(set_of_jobs);
//        } catch (wrench::WorkflowExecutionException &e) {
//          success = false;
//          if (e.getCause()->getCauseType() != wrench::FailureCause::FUNCTIONALITY_NOT_AVAILABLE) {
//            throw std::runtime_error("Got an exception, as expected, but not the right failure cause");
//          }
//          auto real_cause = (wrench::FunctionalityNotAvailable *) e.getCause().get();
//          if (real_cause->getService() != batch_service) {
//            throw std::runtime_error(
//                    "Got the expected exception, but the failure cause does not point to the right service");
//          }
//        }
//        if (success) {
//          throw std::runtime_error("Should not be able to get a queue waiting time estimate");
//        }
//        #endif

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
        this->workflow->removeTask(task);

      }


      return 0;
    }
};

#ifdef ENABLE_BATSCHED

TEST_F(BatchServiceTest, BatchJobBasicEstimateWaitingTimeTest) {
  DO_TEST_WITH_FORK(do_BatchJobBasicEstimateWaitingTimeTest_test);
}

#else

TEST_F(BatchServiceTest, DISABLED_BatchJobBasicEstimateWaitingTimeTest) {
  DO_TEST_WITH_FORK(do_BatchJobBasicEstimateWaitingTimeTest_test);
}

#endif


void BatchServiceTest::do_BatchJobBasicEstimateWaitingTimeTest_test() {

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

  // Create a Batch Service
//  #ifdef ENABLE_BATSCHED
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true,
                                   simulation->getHostnameList(), storage_service1, {
                                           {wrench::BatchServiceProperty::BATCH_SCHEDULING_ALGORITHM, "conservative_bf"}
                                   })));
//  #else
//  EXPECT_NO_THROW(compute_service = simulation->add(
//          new wrench::BatchService(hostname, true, true,
//                                   simulation->getHostnameList(), storage_service1, {
//                                           {wrench::BatchServiceProperty::BATCH_SCHEDULING_ALGORITHM, "FCFS"}
//                                   })));
//  #endif


  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(new BatchJobBasicEstimateWaitingTimeTestWMS(
          this, {compute_service}, {storage_service1, storage_service2}, hostname)));

  EXPECT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));

//  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
//          new wrench::FileRegistryService(hostname));

  simulation->add(new wrench::FileRegistryService(hostname));

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


/**********************************************************************/
/**  QUERY/ANSWER BATCH_JOB_ESTIMATE_WAITING_TIME **/
/**********************************************************************/

class BatchJobEstimateWaitingTimeTestWMS : public wrench::WMS {

public:
    BatchJobEstimateWaitingTimeTestWMS(BatchServiceTest *test,
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

      {

        // Create a sequential task that lasts 5 minutes and requires 2 cores
        wrench::WorkflowTask *task = this->workflow->addTask("task", 299, 1, 1, 1.0);
        task->addInputFile(this->workflow->getFileById("input_file"));
        task->addOutputFile(this->workflow->getFileById("output_file"));


        // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

        wrench::StandardJob *job = job_manager->createStandardJob(
                {task},
                {
                        {*(task->getInputFiles().begin()),  this->test->storage_service1},
                        {*(task->getOutputFiles().begin()), this->test->storage_service1}
                },
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                        this->workflow->getFileById("input_file"), this->test->storage_service1,
                        this->test->storage_service2)},
                {},
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(this->workflow->getFileById("input_file"),
                                                                              this->test->storage_service2)});

        std::map<std::string, std::string> batch_job_args;
        batch_job_args["-N"] = "4";
        batch_job_args["-t"] = "5"; //time in minutes
        batch_job_args["-c"] = "1"; //number of cores per node
        try {
          job_manager->submitJob(job, this->test->compute_service, batch_job_args);
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error(
                  "Got some exception"
          );
        }
        double first_job_running = this->simulation->getCurrentSimulatedDate();


        auto *batch_service = dynamic_cast<wrench::BatchService *>((*this->compute_services.begin()));
        std::string job_id = "my_job1";
        unsigned int nodes = 2;
        double walltime_seconds = 1000;
        std::tuple<std::string,unsigned int,unsigned int,double> my_job = std::make_tuple(job_id,nodes,1,walltime_seconds);
        std::set<std::tuple<std::string,unsigned int,unsigned int,double>> set_of_jobs = {my_job};
        std::map<std::string,double> jobs_estimated_waiting_time = batch_service->getQueueWaitingTimeEstimate(set_of_jobs);
        double expected_wait_time = 300 - first_job_running; // in seconds
        double delta = fabs(expected_wait_time - (jobs_estimated_waiting_time[job_id] - 1));
        if (delta > 1) { // 1 second accuracy threshold
          throw std::runtime_error("Estimated queue wait time incorrect (expected: " + std::to_string(expected_wait_time) + ", got: " + std::to_string(jobs_estimated_waiting_time[job_id]) + ")");
        }


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
        this->workflow->removeTask(task);

      }

      {

        // Create a sequential task that lasts one min and requires 2 cores
        wrench::WorkflowTask *task = this->workflow->addTask("task", 60, 2, 2, 1.0);
        task->addInputFile(this->workflow->getFileById("input_file"));
        task->addOutputFile(this->workflow->getFileById("output_file"));


        // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

        wrench::StandardJob *job = job_manager->createStandardJob(
                {task},
                {
                        {*(task->getInputFiles().begin()),  this->test->storage_service1},
                        {*(task->getOutputFiles().begin()), this->test->storage_service1}
                },
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                        this->workflow->getFileById("input_file"), this->test->storage_service1,
                        this->test->storage_service2)},
                {},
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(this->workflow->getFileById("input_file"),
                                                                              this->test->storage_service2)});

        std::map<std::string, std::string> batch_job_args;
        batch_job_args["-N"] = "4";
        batch_job_args["-t"] = "5"; //time in minutes
        batch_job_args["-c"] = "4"; //number of cores per node
        try {
          job_manager->submitJob(job, this->test->compute_service, batch_job_args);
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error(
                  "Got some exception"
          );
        }

        auto *batch_service = dynamic_cast<wrench::BatchService *>((*this->compute_services.begin()));
        std::set<std::tuple<std::string, unsigned int, unsigned int, double>> set_of_jobs = {};
        for (int i=0; i<10; i++) {
          std::string job_id = "new_job"+std::to_string(i);
          unsigned int nodes = rand() % 4 + 1;
          double walltime_seconds = nodes * (rand() % 10 + 1);
          std::tuple<std::string, unsigned int, unsigned int, double> my_job = std::make_tuple(job_id, nodes,1, walltime_seconds);
          set_of_jobs.insert(my_job);
        }
        std::map<std::string, double> jobs_estimated_waiting_time = batch_service->getQueueWaitingTimeEstimate(
                set_of_jobs);

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
        this->workflow->removeTask(task);

      }

      return 0;
    }
};

#ifdef ENABLE_BATSCHED

TEST_F(BatchServiceTest, BatchJobEstimateWaitingTimeTest) {
  DO_TEST_WITH_FORK(do_BatchJobEstimateWaitingTimeTest_test);
}

#else

TEST_F(BatchServiceTest, DISABLED_BatchJobEstimateWaitingTimeTest) {
  DO_TEST_WITH_FORK(do_BatchJobEstimateWaitingTimeTest_test);
}

#endif


void BatchServiceTest::do_BatchJobEstimateWaitingTimeTest_test() {

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

  // Create a Batch Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true,
                                   simulation->getHostnameList(), storage_service1, {
                                           {wrench::BatchServiceProperty::BATCH_SCHEDULING_ALGORITHM, "conservative_bf"},
                                           {wrench::BatchServiceProperty::BATCH_RJMS_DELAY, "0"}
                                   })));

  simulation->add(new wrench::FileRegistryService(hostname));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(new BatchJobEstimateWaitingTimeTestWMS(
          this, {compute_service}, {storage_service1, storage_service2}, hostname)));

  // Create two workflow files
  wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000.0);
  wrench::WorkflowFile *output_file = this->workflow->addFile("output_file", 20000.0);

  EXPECT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));

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
/**  QUERY/ANSWER BATCH_JOB_LITTLE_COMPLEX_ESTIMATE_WAITING_TIME **/
/**********************************************************************/

class BatchJobLittleComplexEstimateWaitingTimeTestWMS : public wrench::WMS {

public:
    BatchJobLittleComplexEstimateWaitingTimeTestWMS(BatchServiceTest *test,
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

      {

        // Submit the first job for 300 seconds and using 4 full cores
        wrench::WorkflowTask *task = this->workflow->addTask("task", 299, 1, 1, 1.0);
        task->addInputFile(this->workflow->getFileById("input_file"));
        task->addOutputFile(this->workflow->getFileById("output_file"));


        // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

        wrench::StandardJob *job = job_manager->createStandardJob(
                {task},
                {
                        {*(task->getInputFiles().begin()),  this->test->storage_service1},
                        {*(task->getOutputFiles().begin()), this->test->storage_service1}
                },
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                        this->workflow->getFileById("input_file"), this->test->storage_service1,
                        this->test->storage_service2)},
                {},
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(this->workflow->getFileById("input_file"),
                                                                              this->test->storage_service2)});


        std::map<std::string, std::string> batch_job_args;
        batch_job_args["-N"] = "4";
        batch_job_args["-t"] = "5"; //time in minutes
        batch_job_args["-c"] = "1"; //number of cores per node
        try {
          job_manager->submitJob(job, this->test->compute_service, batch_job_args);
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error(
                  "Got some exception"
          );
        }


        // Submit the second job for next 300 seconds and using 2 cores
        wrench::WorkflowTask *task1 = this->workflow->addTask("task1", 299, 1, 1, 1.0);
        task1->addInputFile(this->workflow->getFileById("input_file"));
        task1->addOutputFile(this->workflow->getFileById("output_file"));


        // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

        wrench::StandardJob *job1 = job_manager->createStandardJob(
                {task1},
                {
                        {*(task1->getInputFiles().begin()),  this->test->storage_service1},
                        {*(task1->getOutputFiles().begin()), this->test->storage_service1}
                },
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                        this->workflow->getFileById("input_file"), this->test->storage_service1,
                        this->test->storage_service2)},
                {},
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(this->workflow->getFileById("input_file"),
                                                                              this->test->storage_service2)});

        std::map<std::string, std::string> batch_job_args1;
        batch_job_args1["-N"] = "2";
        batch_job_args1["-t"] = "5"; //time in minutes
        batch_job_args1["-c"] = "4"; //number of cores per node
        try {
          job_manager->submitJob(job1, this->test->compute_service, batch_job_args1);
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error(
                  "Got some exception"
          );
        }


        // Submit the third job for next 300 seconds and using 4 cores
        wrench::WorkflowTask *task2 = this->workflow->addTask("task2", 299, 1, 1, 1.0);
        task2->addInputFile(this->workflow->getFileById("input_file"));
        task2->addOutputFile(this->workflow->getFileById("output_file"));


        // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

        wrench::StandardJob *job2 = job_manager->createStandardJob(
                {task2},
                {
                        {*(task2->getInputFiles().begin()),  this->test->storage_service1},
                        {*(task2->getOutputFiles().begin()), this->test->storage_service1}
                },
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                        this->workflow->getFileById("input_file"), this->test->storage_service1,
                        this->test->storage_service2)},
                {},
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(this->workflow->getFileById("input_file"),
                                                                              this->test->storage_service2)});

        std::map<std::string, std::string> batch_job_args2;
        batch_job_args2["-N"] = "4";
        batch_job_args2["-t"] = "5"; //time in minutes
        batch_job_args2["-c"] = "4"; //number of cores per node
        try {
          job_manager->submitJob(job2, this->test->compute_service, batch_job_args2);
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error(
                  "Got some exception"
          );
        }

        auto *batch_service = dynamic_cast<wrench::BatchService *>((*this->compute_services.begin()));

        std::string job_id = "my_job1";
        unsigned int nodes = 1;
        double walltime_seconds = 400;
        std::tuple<std::string,unsigned int,unsigned int,double> my_job = std::make_tuple(job_id,nodes,1,walltime_seconds);
        std::set<std::tuple<std::string,unsigned int,unsigned int, double>> set_of_jobs = {my_job};
        std::map<std::string,double> jobs_estimated_waiting_time = batch_service->getQueueWaitingTimeEstimate(set_of_jobs);

        if ((jobs_estimated_waiting_time[job_id] - 900) > 1) {
          throw std::runtime_error("Estimated queue wait time incorrect (expected: " + std::to_string(900) + ", got: " + std::to_string(jobs_estimated_waiting_time[job_id]) + ")");
        }

        job_id = "my_job2";
        nodes = 1;
        walltime_seconds = 299;
        my_job = std::make_tuple(job_id,nodes,1, walltime_seconds);
        set_of_jobs = {my_job};
        jobs_estimated_waiting_time = batch_service->getQueueWaitingTimeEstimate(set_of_jobs);

        if (fabs(jobs_estimated_waiting_time[job_id] - 300) > 1) {
          throw std::runtime_error("Estimated queue wait time incorrect (expected: " + std::to_string(300) + ", got: " + std::to_string(jobs_estimated_waiting_time[job_id]) + ")");
        }


        job_id = "my_job3";
        nodes = 2;
        walltime_seconds = 299;
        my_job = std::make_tuple(job_id,nodes,1, walltime_seconds);
        set_of_jobs = {my_job};
        jobs_estimated_waiting_time = batch_service->getQueueWaitingTimeEstimate(set_of_jobs);

        if (fabs(jobs_estimated_waiting_time[job_id] - 300) > 1) {
          throw std::runtime_error("Estimated queue wait time incorrect (expected: " + std::to_string(300) + ", got: " + std::to_string(jobs_estimated_waiting_time[job_id]) + ")");
        }


        job_id = "my_job4";
        nodes = 3;
        walltime_seconds = 299;
        my_job = std::make_tuple(job_id,nodes,1, walltime_seconds);
        set_of_jobs = {my_job};
        jobs_estimated_waiting_time = batch_service->getQueueWaitingTimeEstimate(set_of_jobs);

        if (fabs(jobs_estimated_waiting_time[job_id] - 900) > 1) {
          throw std::runtime_error("Estimated queue wait time incorrect (expected: " + std::to_string(900) + ", got: " + std::to_string(jobs_estimated_waiting_time[job_id]) + ")");
        }



        for (int i=0; i<3; i++) {

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

        this->workflow->removeTask(task);
        this->workflow->removeTask(task1);
        this->workflow->removeTask(task2);

      }

      return 0;
    }
};

#ifdef ENABLE_BATSCHED

TEST_F(BatchServiceTest, BatchJobLittleComplexEstimateWaitingTimeTest) {
  DO_TEST_WITH_FORK(do_BatchJobLittleComplexEstimateWaitingTimeTest_test);
}

#else

TEST_F(BatchServiceTest, DISABLED_BatchJobLittleComplexEstimateWaitingTimeTest) {
  DO_TEST_WITH_FORK(do_BatchJobLittleComplexEstimateWaitingTimeTest_test);
}

#endif


void BatchServiceTest::do_BatchJobLittleComplexEstimateWaitingTimeTest_test() {

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

  // Create a Batch Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true,
                                   simulation->getHostnameList(), storage_service1, {
                                           {wrench::BatchServiceProperty::BATCH_SCHEDULING_ALGORITHM, "conservative_bf"},
                                           {wrench::BatchServiceProperty::BATCH_RJMS_DELAY, "0"}
                                   })));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(new BatchJobLittleComplexEstimateWaitingTimeTestWMS(
          this, {compute_service}, {storage_service1, storage_service2}, hostname)));

  EXPECT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));

//  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
//          new wrench::FileRegistryService(hostname));

  simulation->add(new wrench::FileRegistryService(hostname));

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



