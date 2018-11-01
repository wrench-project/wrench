/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <math.h>

#include <gtest/gtest.h>
#include <wrench-dev.h>

#include "wrench/workflow/job/PilotJob.h"
#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Log category for test");

#define EPSILON 0.005


class BareMetalComputeServiceTestScheduling : public ::testing::Test {


public:
    // Default
    wrench::ComputeService *cs = nullptr;

    // Old Default
    wrench::ComputeService *cs_fcfs_aggressive_maximum_maximum_flops_best_fit = nullptr;
    // "minimum" core allocation
    wrench::ComputeService *cs_fcfs_aggressive_minimum_maximum_flops_best_fit = nullptr;
    // "maximum_minimum_cores" task selection
    wrench::ComputeService *cs_fcfs_aggressive_maximum_maximum_minimum_cores_best_fit = nullptr;

    void do_OneJob_test();
    void do_MultiJob_test();
    void do_RAMPressure_test();


    static bool isJustABitGreater(double base, double variable) {
      return ((variable > base) && (variable < base + EPSILON));
    }

protected:
    BareMetalComputeServiceTestScheduling() {

      // Create the simplest workflow
      workflow = new wrench::Workflow();

      // Create a two-host quad-core platform file
      std::string xml = "<?xml version='1.0'?>"
              "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
              "<platform version=\"4.1\"> "
              "   <zone id=\"AS0\" routing=\"Full\"> "
              "       <host id=\"Host1\" speed=\"1f\" core=\"4\"> "
              "            <prop id=\"ram\" value=\"1000\"/> "
              "       </host> "
              "       <host id=\"Host2\" speed=\"1f\" core=\"4\"> "
              "            <prop id=\"ram\" value=\"1000\"/> "
              "       </host> "
              "        <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
              "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\"/> </route>"
              "   </zone> "
              "</platform>";
      FILE *platform_file = fopen(platform_file_path.c_str(), "w");
      fprintf(platform_file, "%s", xml.c_str());
      fclose(platform_file);

    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
    wrench::Workflow *workflow;
};


/**********************************************************************/
/**  ONEJOB SIMULATION TEST                                          **/
/**********************************************************************/

class OneJobTestWMS : public wrench::WMS {

public:
    OneJobTestWMS(BareMetalComputeServiceTestScheduling *test,
                  const std::set<wrench::ComputeService *> &compute_services,
                  const std::set<wrench::StorageService *> &storage_services,
                  std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }


private:

    BareMetalComputeServiceTestScheduling *test;

    int main() {

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();


      /**********************************************/
      /** DEFAULT_PROPERTIES / CASE 1              **/
      /**********************************************/
      {

        double now = wrench::S4U_Simulation::getClock();

        // Create a job with 3 tasks
        wrench::WorkflowTask *t1 = this->getWorkflow()->addTask("task1", 60.0000, 2, 3, 1.0, 0);
        wrench::WorkflowTask *t2 = this->getWorkflow()->addTask("task2", 60.0001, 1, 2, 1.0, 0);
        wrench::WorkflowTask *t3 = this->getWorkflow()->addTask("task3", 60.0002, 2, 4, 1.0, 0);

        std::vector<wrench::WorkflowTask *> tasks;

        tasks.push_back(t1);
        tasks.push_back(t2);
        tasks.push_back(t3);
        wrench::StandardJob *job = job_manager->createStandardJob(tasks, {}, {}, {}, {});

        job_manager->submitJob(job, this->test->cs_fcfs_aggressive_maximum_maximum_flops_best_fit);

        // Wait for a workflow execution event
        std::unique_ptr<wrench::WorkflowExecutionEvent> event;
        try {
          event = this->getWorkflow()->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
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

        // Check completion states and times
        if ((t1->getState() != wrench::WorkflowTask::COMPLETED) ||
            (t2->getState() != wrench::WorkflowTask::COMPLETED) ||
            (t3->getState() != wrench::WorkflowTask::COMPLETED)) {
          throw std::runtime_error("DEFAULT_PROPERTIES / 1: Unexpected task states (" +
                                   wrench::WorkflowTask::stateToString(t1->getState()) + ", " +
                                   wrench::WorkflowTask::stateToString(t2->getState()) + ", " +
                                   wrench::WorkflowTask::stateToString(t3->getState()) + ")"
          );
        }

        double task1_makespan = t1->getEndDate() - now;
        double task2_makespan = t2->getEndDate() - now;
        double task3_makespan = t3->getEndDate() - now;

//        WRENCH_INFO("t1:%lf t2:%lf t3:%lf", task1_makespan, task2_makespan, task3_makespan);


        if (!BareMetalComputeServiceTestScheduling::isJustABitGreater(30, task1_makespan) ||
            !BareMetalComputeServiceTestScheduling::isJustABitGreater(30, task2_makespan) ||
            !BareMetalComputeServiceTestScheduling::isJustABitGreater(15, task3_makespan)) {
          throw std::runtime_error("DEFAULT PROPERTIES / CASE 1: Unexpected task execution times "
                                           "t1: " + std::to_string(task1_makespan) +
                                   " t2: " + std::to_string(task2_makespan) +
                                   " t3: " + std::to_string(task3_makespan));
        }

        this->getWorkflow()->removeTask(t1);
        this->getWorkflow()->removeTask(t2);
        this->getWorkflow()->removeTask(t3);
      }



      /**********************************************/
      /** DEFAULT PROPERTIES / CASE 2              **/
      /**********************************************/
      {

        double now = wrench::S4U_Simulation::getClock();

        // Create a job with 3 tasks
        wrench::WorkflowTask *t1 = this->getWorkflow()->addTask("task1", 60.0001, 2, 3, 1.0, 0);
        wrench::WorkflowTask *t2 = this->getWorkflow()->addTask("task2", 60.0000, 1, 2, 1.0, 0);
        wrench::WorkflowTask *t3 = this->getWorkflow()->addTask("task3", 60.0002, 2, 4, 1.0, 0);

        std::vector<wrench::WorkflowTask *> tasks;

        tasks.push_back(t1);
        tasks.push_back(t2);
        tasks.push_back(t3);
        wrench::StandardJob *job = job_manager->createStandardJob(tasks, {}, {}, {}, {});

        job_manager->submitJob(job, this->test->cs_fcfs_aggressive_maximum_maximum_flops_best_fit);

        // Wait for a workflow execution event
        std::unique_ptr<wrench::WorkflowExecutionEvent> event;
        try {
          event = this->getWorkflow()->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
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

        // Check completion states and times
        if ((t1->getState() != wrench::WorkflowTask::COMPLETED) ||
            (t2->getState() != wrench::WorkflowTask::COMPLETED) ||
            (t3->getState() != wrench::WorkflowTask::COMPLETED)) {
          throw std::runtime_error("DEFAULT PROPERTIES / CASE 2: Unexpected task states (" +
                                   wrench::WorkflowTask::stateToString(t1->getState()) + ", " +
                                   wrench::WorkflowTask::stateToString(t2->getState()) + ", " +
                                   wrench::WorkflowTask::stateToString(t3->getState()) + ")"
          );
        }

        double task1_makespan = t1->getEndDate() - now;
        double task2_makespan = t2->getEndDate() - now;
        double task3_makespan = t3->getEndDate() - now;

//        WRENCH_INFO("t1:%lf t2:%lf t3:%lf", task1_makespan, task2_makespan, task3_makespan);

        if (!BareMetalComputeServiceTestScheduling::isJustABitGreater(20, task1_makespan) ||
            !BareMetalComputeServiceTestScheduling::isJustABitGreater(60, task2_makespan) ||
            !BareMetalComputeServiceTestScheduling::isJustABitGreater(15, task3_makespan)) {
          throw std::runtime_error("DEFAULT PROPERTIES / CASE 2: Unexpected task execution times "
                                           "t1: " + std::to_string(task1_makespan) +
                                   " t2: " + std::to_string(task2_makespan) +
                                   " t3: " + std::to_string(task3_makespan));
        }
        this->getWorkflow()->removeTask(t1);
        this->getWorkflow()->removeTask(t2);
        this->getWorkflow()->removeTask(t3);
      }



      /*******************************************************************/
      /** DEFAULT PROPERTIES BUT MIN CORE ALLOCATIONS / CASE 1          **/
      /*******************************************************************/
      {

        double now = wrench::S4U_Simulation::getClock();

        // Create a job with 3 tasks
        wrench::WorkflowTask *t1 = this->getWorkflow()->addTask("task1", 60.0000, 2, 3, 1.0, 0);
        wrench::WorkflowTask *t2 = this->getWorkflow()->addTask("task2", 60.0001, 1, 2, 1.0, 0);
        wrench::WorkflowTask *t3 = this->getWorkflow()->addTask("task3", 60.0002, 2, 4, 1.0, 0);

        std::vector<wrench::WorkflowTask *> tasks;

        tasks.push_back(t1);
        tasks.push_back(t2);
        tasks.push_back(t3);
        wrench::StandardJob *job = job_manager->createStandardJob(tasks, {}, {}, {}, {});

        job_manager->submitJob(job, this->test->cs_fcfs_aggressive_minimum_maximum_flops_best_fit);

        // Wait for a workflow execution event
        std::unique_ptr<wrench::WorkflowExecutionEvent> event;
        try {
          event = this->getWorkflow()->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
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

        // Check completion states and times
        if ((t1->getState() != wrench::WorkflowTask::COMPLETED) ||
            (t2->getState() != wrench::WorkflowTask::COMPLETED) ||
            (t3->getState() != wrench::WorkflowTask::COMPLETED)) {
          throw std::runtime_error("MIN CORE ALLOCATIONS / CASE 1: Unexpected task states (" +
                                   wrench::WorkflowTask::stateToString(t1->getState()) + ", " +
                                   wrench::WorkflowTask::stateToString(t2->getState()) + ", " +
                                   wrench::WorkflowTask::stateToString(t3->getState()) + ")"
          );
        }

        double task1_makespan = t1->getEndDate() - now;
        double task2_makespan = t2->getEndDate() - now;
        double task3_makespan = t3->getEndDate() - now;

//        WRENCH_INFO("t1:%lf t2:%lf t3:%lf", task1_makespan, task2_makespan, task3_makespan);

        if (!BareMetalComputeServiceTestScheduling::isJustABitGreater(30, task1_makespan) ||
            !BareMetalComputeServiceTestScheduling::isJustABitGreater(60, task2_makespan) ||
            !BareMetalComputeServiceTestScheduling::isJustABitGreater(30, task3_makespan)) {
          throw std::runtime_error(
                  "DEFAULT PROPERTIES BUT MINIMUM CORE ALLOCATION / CASE 1: Unexpected task execution times "
                          "t1: " + std::to_string(task1_makespan) +
                  " t2: " + std::to_string(task2_makespan) +
                  " t3: " + std::to_string(task3_makespan));
        }

        this->getWorkflow()->removeTask(t1);
        this->getWorkflow()->removeTask(t2);
        this->getWorkflow()->removeTask(t3);
      }

      /*******************************************************************/
      /** DEFAULT PROPERTIES BUT MAX_MIN_CORES TASK SELECTION / CASE 1  **/
      /*******************************************************************/
      {

        double now = wrench::S4U_Simulation::getClock();

        // Create a job with 3 tasks
        wrench::WorkflowTask *t1 = this->getWorkflow()->addTask("task1", 60.0000, 3, 4, 1.0, 0);
        wrench::WorkflowTask *t2 = this->getWorkflow()->addTask("task2", 60.0001, 1, 2, 1.0, 0);
        wrench::WorkflowTask *t3 = this->getWorkflow()->addTask("task3", 60.0002, 2, 4, 1.0, 0);

        std::vector<wrench::WorkflowTask *> tasks;

        tasks.push_back(t1);
        tasks.push_back(t2);
        tasks.push_back(t3);
        wrench::StandardJob *job = job_manager->createStandardJob(tasks, {}, {}, {}, {});

        job_manager->submitJob(job, this->test->cs_fcfs_aggressive_maximum_maximum_minimum_cores_best_fit);

        // Wait for a workflow execution event
        std::unique_ptr<wrench::WorkflowExecutionEvent> event;
        try {
          event = this->getWorkflow()->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
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

        // Check completion states and times
        if ((t1->getState() != wrench::WorkflowTask::COMPLETED) ||
            (t2->getState() != wrench::WorkflowTask::COMPLETED) ||
            (t3->getState() != wrench::WorkflowTask::COMPLETED)) {
          throw std::runtime_error("DEFAULT PROPERTIES BUT MAX_MIN_CORE_ALLOCATIONS / CASE 1: Unexpected task states (" +
              wrench::WorkflowTask::stateToString(t1->getState()) + ", " +
              wrench::WorkflowTask::stateToString(t2->getState()) + ", " +
              wrench::WorkflowTask::stateToString(t3->getState()) + ")"
          );
        }

        double task1_makespan = t1->getEndDate() - now;
        double task2_makespan = t2->getEndDate() - now;
        double task3_makespan = t3->getEndDate() - now;

//        WRENCH_INFO("t1:%lf t2:%lf t3:%lf", task1_makespan, task2_makespan, task3_makespan);

        if (!BareMetalComputeServiceTestScheduling::isJustABitGreater(15, task1_makespan) ||
            !BareMetalComputeServiceTestScheduling::isJustABitGreater(45, task2_makespan) ||
            !BareMetalComputeServiceTestScheduling::isJustABitGreater(15, task3_makespan)) {
          throw std::runtime_error(
                  "DEFAULT PROPERTIES BUT MAX_MIN_CORES TASK SELECTION / CASE 1 : Unexpected task execution times "
                          "t1: " + std::to_string(task1_makespan) +
                  " t2: " + std::to_string(task2_makespan) +
                  " t3: " + std::to_string(task3_makespan));
        }

        this->getWorkflow()->removeTask(t1);
        this->getWorkflow()->removeTask(t2);
        this->getWorkflow()->removeTask(t3);
      }


      return 0;
    }
};

TEST_F(BareMetalComputeServiceTestScheduling, DISABLED_OneJob) {
  DO_TEST_WITH_FORK(do_OneJob_test);
}

void BareMetalComputeServiceTestScheduling::do_OneJob_test() {

  // Create and initialize a simulation
  auto *simulation = new wrench::Simulation();
  int argc = 1;
  auto **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_task_test");

  simulation->init(&argc, argv);

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  std::set<wrench::ComputeService *> compute_services;
  // Create a Compute Service
  ASSERT_NO_THROW(cs_fcfs_aggressive_maximum_maximum_flops_best_fit = simulation->add(
          new wrench::BareMetalComputeService(
                  "Host1",
                  (std::set<std::string>){"Host1", "Host2"},
                  0,
                  {})));
  compute_services.insert(cs_fcfs_aggressive_maximum_maximum_flops_best_fit);

  // Create a Compute Service
  ASSERT_NO_THROW(cs_fcfs_aggressive_minimum_maximum_flops_best_fit = simulation->add(
          new wrench::BareMetalComputeService(
                  "Host1",
                  (std::set<std::string>){"Host1","Host2"},
                  0,
                  {})));
  compute_services.insert(cs_fcfs_aggressive_minimum_maximum_flops_best_fit);

  // Create a Compute Service
  ASSERT_NO_THROW(cs_fcfs_aggressive_maximum_maximum_minimum_cores_best_fit = simulation->add(
          new wrench::BareMetalComputeService(
                  "Host1",
                  (std::set<std::string>){"Host1", "Host2"},
                  0,
                  {})));
  compute_services.insert(cs_fcfs_aggressive_maximum_maximum_minimum_cores_best_fit);

  // Create a WMS
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(
          new OneJobTestWMS(
                  this,  compute_services, {}, "Host1")));

  ASSERT_NO_THROW(wms->addWorkflow(workflow));

  ASSERT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}


/**********************************************************************/
/**  MULTIJOB SIMULATION TEST                                         **/
/**********************************************************************/

class MultiJobTestWMS : public wrench::WMS {

public:
    MultiJobTestWMS(BareMetalComputeServiceTestScheduling *test,
                    const std::set<wrench::ComputeService *> &compute_services,
                    const std::set<wrench::StorageService *> &storage_services,
                    std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }


private:

    BareMetalComputeServiceTestScheduling *test;

    int main() {

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();


      /**********************************************/
      /** DEFAULT_PROPERTIES / CASE 1              **/
      /**********************************************/
      {

        double now = wrench::S4U_Simulation::getClock();

        // Submit a PilotJob that lasts 3600 seconds and takes 2 cores and 0 bytes per host
        wrench::PilotJob *pilot_job = job_manager->createPilotJob(2, 2, 0, 3600);

        job_manager->submitJob(pilot_job, this->test->cs_fcfs_aggressive_maximum_maximum_flops_best_fit);



        // Create and submit a job with 2 2-core tasks
        wrench::WorkflowTask *t1_1 = this->getWorkflow()->addTask("task1_1", 8000, 2, 2, 1.0, 0);
        wrench::WorkflowTask *t1_2 = this->getWorkflow()->addTask("task1_2", 8000, 2, 2, 1.0, 0);

        std::vector<wrench::WorkflowTask *> job1_tasks;
        job1_tasks.push_back(t1_1);
        job1_tasks.push_back(t1_2);
        wrench::StandardJob *job1 = job_manager->createStandardJob(job1_tasks, {}, {}, {}, {});

        job_manager->submitJob(job1, this->test->cs_fcfs_aggressive_maximum_maximum_flops_best_fit);

        // Create and submit a job with a 2-core task and a 1-core task
        wrench::WorkflowTask *t2_1 = this->getWorkflow()->addTask("task2_1", 60, 2, 2, 1.0, 0);
        wrench::WorkflowTask *t2_2 = this->getWorkflow()->addTask("task2_2", 60, 1, 1, 1.0, 0);

        std::vector<wrench::WorkflowTask *> job2_tasks;
        job2_tasks.push_back(t2_1);
        job2_tasks.push_back(t2_2);
        wrench::StandardJob *job2 = job_manager->createStandardJob(job2_tasks, {}, {}, {}, {});

        job_manager->submitJob(job2, this->test->cs_fcfs_aggressive_maximum_maximum_flops_best_fit);


        {
          // Wait for a PILOT JOB STARTED execution event
          std::unique_ptr<wrench::WorkflowExecutionEvent> event;
          try {
            event = this->getWorkflow()->waitForNextExecutionEvent();
          } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
          }
          switch (event->type) {
            case wrench::WorkflowExecutionEvent::PILOT_JOB_START: {
              // success, do nothing for now
              break;
            }
            default: {
              throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
            }
          }

          if (!BareMetalComputeServiceTestScheduling::isJustABitGreater(0,
                                                                                 wrench::S4U_Simulation::getClock())) {
            throw std::runtime_error("Pilot job should startDaemon at time 0");
          }
        }


        {
          // Wait for a PILOT JOB EXPIRED execution event
          std::unique_ptr<wrench::WorkflowExecutionEvent> event;
          try {
            event = this->getWorkflow()->waitForNextExecutionEvent();
          } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
          }
          switch (event->type) {
            case wrench::WorkflowExecutionEvent::PILOT_JOB_EXPIRATION: {
              // success do nothing
              break;
            }
            default: {
              throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
            }
          }

          if (!BareMetalComputeServiceTestScheduling::isJustABitGreater(3600,
                                                                                 wrench::S4U_Simulation::getClock())) {
            throw std::runtime_error("Pilot job should expire at time 3600");
          }
        }


        {
          // Wait for a STANDARD JOB COMPLETED execution event
          std::unique_ptr<wrench::WorkflowExecutionEvent> event;
          try {
            event = this->getWorkflow()->waitForNextExecutionEvent();
          } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
          }
          switch (event->type) {
            case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
              if (dynamic_cast<wrench::StandardJobCompletedEvent*>(event.get())->standard_job != job2) {
                throw std::runtime_error("Unexpected job completion (job2 should complete first!)");
              }
              // success, do nothing for now
              break;
            }
            default: {
              throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
            }
          }
          if (!BareMetalComputeServiceTestScheduling::isJustABitGreater(3600 + 60,
                                                                                 wrench::S4U_Simulation::getClock())) {
            throw std::runtime_error("Standard job #2 should complete at time 3600 + 60");
          }
        }

        {
          // Wait for a STANDARD JOB COMPLETION execution event
          std::unique_ptr<wrench::WorkflowExecutionEvent> event;
          try {
            event = this->getWorkflow()->waitForNextExecutionEvent();
          } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
          }
          switch (event->type) {
            case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
              if (dynamic_cast<wrench::StandardJobCompletedEvent*>(event.get())->standard_job != job1) {
                throw std::runtime_error("Unexpected job completion (job1 should complete last!)");
              }
              // success, do nothing for now
              break;
            }
            default: {
              throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
            }
          }

          if (!BareMetalComputeServiceTestScheduling::isJustABitGreater(4000,
                                                                                 wrench::S4U_Simulation::getClock())) {
            throw std::runtime_error("Standard job #1 should complete at time 4000");
          }
        }

        this->getWorkflow()->removeTask(t1_1);
        this->getWorkflow()->removeTask(t1_2);
        this->getWorkflow()->removeTask(t2_1);
        this->getWorkflow()->removeTask(t2_2);
      }

      return 0;
    }
};

TEST_F(BareMetalComputeServiceTestScheduling, DISABLED_MultiJob) {
  DO_TEST_WITH_FORK(do_MultiJob_test);
}

void BareMetalComputeServiceTestScheduling::do_MultiJob_test() {

  // Create and initialize a simulation
  auto *simulation = new wrench::Simulation();
  int argc = 1;
  auto **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_task_test");

  simulation->init(&argc, argv);

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Create a Compute Service
  ASSERT_NO_THROW(cs_fcfs_aggressive_maximum_maximum_flops_best_fit = simulation->add(
          new wrench::BareMetalComputeService("Host1",
                                                       (std::set<std::string>){"Host1", "Host2"}, 0.0,
                                                       {})));
  std::set<wrench::ComputeService *> compute_services;
  compute_services.insert(cs_fcfs_aggressive_maximum_maximum_flops_best_fit);

  // Create a WMS
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(
          new MultiJobTestWMS(
                  this, compute_services, {}, "Host1")));

  ASSERT_NO_THROW(wms->addWorkflow(workflow));

  ASSERT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}



/**********************************************************************/
/**  RAM PRESSURE TEST                                               **/
/**********************************************************************/

class RAMPressureTestWMS : public wrench::WMS {

public:
    RAMPressureTestWMS(BareMetalComputeServiceTestScheduling *test,
                    const std::set<wrench::ComputeService *> &compute_services,
                    const std::set<wrench::StorageService *> &storage_services,
                    std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }


private:

    BareMetalComputeServiceTestScheduling *test;

    int main() {

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      // Create a few tasks
      std::vector<wrench::WorkflowTask *> tasks;
      tasks.push_back(this->getWorkflow()->addTask("task1", 60, 1, 1, 1.0, 500));
      tasks.push_back(this->getWorkflow()->addTask("task2", 60, 1, 1, 1.0, 600));
      tasks.push_back(this->getWorkflow()->addTask("task3", 60, 1, 1, 1.0, 500));
      tasks.push_back(this->getWorkflow()->addTask("task4", 60, 1, 1, 1.0, 000));

      // Submit them in order
      for (auto const & t : tasks) {
        wrench::StandardJob *j = job_manager->createStandardJob(t, {});
        std::map<std::string, std::string> cs_specific_args;
        cs_specific_args.insert(std::make_pair(t->getID(), "Host1:1"));
        job_manager->submitJob(j, this->test->cs, cs_specific_args);
      }

      // Wait for completions
      std::map<wrench::WorkflowTask*, std::tuple<double,double>> times;
      for (int i=0; i < 4; i++) {
        std::unique_ptr<wrench::WorkflowExecutionEvent> event;
        try {
          event = this->getWorkflow()->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
        }
        if (event->type != wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION) {
          throw std::runtime_error("Unexpected execution event: " + std::to_string((int) (event->type)));
        }

        wrench::StandardJob *job = dynamic_cast<wrench::StandardJobCompletedEvent *>(event.get())->standard_job;
        wrench::WorkflowTask *task = *(job->getTasks().begin());
        double start_time = task->getStartDate();
        double end_time = task->getEndDate();
        times.insert(std::make_pair(task, std::make_tuple(start_time, end_time)));
      }

      // Inspect times
      // TASK #1
      if (std::get<0>(times[tasks.at(0)]) > 1.0) {
        throw std::runtime_error("Unexpected start time for task1: " + std::to_string(std::get<0>(times[tasks.at(0)])));
      }
      if ((std::get<1>(times[tasks.at(0)]) < 60.0) || (std::get<1>(times[tasks.at(0)])  > 61.0)) {
        throw std::runtime_error("Unexpected end time for task1: " + std::to_string(std::get<1>(times[tasks.at(0)])));
      }
      // TASK #4
      if (std::get<0>(times[tasks.at(3)]) > 1.0) {
        throw std::runtime_error("Unexpected start time for task4: " + std::to_string(std::get<0>(times[tasks.at(3)])));
      }
      if ((std::get<1>(times[tasks.at(3)]) < 60.0) || (std::get<1>(times[tasks.at(3)])  > 61.0)) {
        throw std::runtime_error("Unexpected end time for task4: " + std::to_string(std::get<1>(times[tasks.at(3)])));
      }
      // TASK #2
      if ((std::get<0>(times[tasks.at(1)]) < 60.0) || (std::get<0>(times[tasks.at(1)]) > 61.0)) {
        throw std::runtime_error("Unexpected start time for task2: " + std::to_string(std::get<0>(times[tasks.at(1)])));
      }
      if ((std::get<1>(times[tasks.at(1)]) < 120.0) || (std::get<1>(times[tasks.at(1)])  > 121.0)) {
        throw std::runtime_error("Unexpected end time for task2: " + std::to_string(std::get<1>(times[tasks.at(1)])));
      }
      // TASK #3
      if ((std::get<0>(times[tasks.at(2)]) < 120.0) || (std::get<0>(times[tasks.at(2)]) > 121.0)) {
        throw std::runtime_error("Unexpected start time for task3: " + std::to_string(std::get<0>(times[tasks.at(2)])));
      }
      if ((std::get<1>(times[tasks.at(2)]) < 180.0) || (std::get<1>(times[tasks.at(2)])  > 181.0)) {
        throw std::runtime_error("Unexpected end time for task3: " + std::to_string(std::get<1>(times[tasks.at(2)])));
      }




      return 0;
    }


};

TEST_F(BareMetalComputeServiceTestScheduling, RAMPressure) {
  DO_TEST_WITH_FORK(do_RAMPressure_test);
}

void BareMetalComputeServiceTestScheduling::do_RAMPressure_test() {

  // Create and initialize a simulation
  auto *simulation = new wrench::Simulation();
  int argc = 1;
  auto **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_task_test");

  simulation->init(&argc, argv);

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Create a Compute Service
  ASSERT_NO_THROW(cs = simulation->add(
          new wrench::BareMetalComputeService("Host1",
                                                       (std::set<std::string>){"Host1", "Host2"}, 0.0,
                                                       {}, {})));
  std::set<wrench::ComputeService *> compute_services;
  compute_services.insert(cs);

  // Create a WMS
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(
          new RAMPressureTestWMS(
                  this, compute_services, {}, "Host1")));

  ASSERT_NO_THROW(wms->addWorkflow(workflow));

  ASSERT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}
