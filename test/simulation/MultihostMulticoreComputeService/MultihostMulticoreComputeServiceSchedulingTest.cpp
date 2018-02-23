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
#include "../NoopScheduler.h"
#include "../TestWithFork.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Log category for test");

#define EPSILON 0.005


class MultihostMulticoreComputeServiceTestScheduling : public ::testing::Test {


public:
    // Default
    wrench::ComputeService *cs_fcfs_aggressive_maximum_maximum_flops_best_fit = nullptr;
    // "minimum" core allocation
    wrench::ComputeService *cs_fcfs_aggressive_minimum_maximum_flops_best_fit = nullptr;
    // "maximum_minimum_cores" task selection
    wrench::ComputeService *cs_fcfs_aggressive_maximum_maximum_minimum_cores_best_fit = nullptr;

    void do_OneJob_test();

    void do_MultiJob_test();


    static bool isJustABitGreater(double base, double variable) {
      return ((variable > base) && (variable < base + EPSILON));
    }

protected:
    MultihostMulticoreComputeServiceTestScheduling() {

      // Create the simplest workflow
      workflow = new wrench::Workflow();

      // Create a two-host quad-core platform file
      std::string xml = "<?xml version='1.0'?>"
              "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
              "<platform version=\"4.1\"> "
              "   <zone id=\"AS0\" routing=\"Full\"> "
              "       <host id=\"Host1\" speed=\"1f\" core=\"4\"/> "
              "       <host id=\"Host2\" speed=\"1f\" core=\"4\"/> "
              "        <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
              "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\"/> </route>"
              "   </zone> "
              "</platform>";
      FILE *platform_file = fopen(platform_file_path.c_str(), "w");
      fprintf(platform_file, "%s", xml.c_str());
      fclose(platform_file);

    }

    std::string platform_file_path = "/tmp/platform.xml";
    wrench::Workflow *workflow;
};


/**********************************************************************/
/**  ONEJOB SIMULATION TEST                                          **/
/**********************************************************************/

class OneJobTestWMS : public wrench::WMS {

public:
    OneJobTestWMS(MultihostMulticoreComputeServiceTestScheduling *test,
                  std::unique_ptr<wrench::Scheduler> scheduler,
                  const std::set<wrench::ComputeService *> &compute_services,
                  const std::set<wrench::StorageService *> &storage_services,
                  std::string hostname) :
            wrench::WMS(std::move(scheduler), compute_services, storage_services, hostname, "test") {
      this->test = test;
    }


private:

    MultihostMulticoreComputeServiceTestScheduling *test;

    int main() {

      // Create a job manager
      std::unique_ptr<wrench::JobManager> job_manager =
              std::unique_ptr<wrench::JobManager>(new wrench::JobManager(this->workflow));


      /**********************************************/
      /** DEFAULT_PROPERTIES / CASE 1              **/
      /**********************************************/
      {

        double now = wrench::S4U_Simulation::getClock();

        // Create a job with 3 tasks
        wrench::WorkflowTask *t1 = this->workflow->addTask("task1", 60.0000, 2, 3, 1.0);
        wrench::WorkflowTask *t2 = this->workflow->addTask("task2", 60.0001, 1, 2, 1.0);
        wrench::WorkflowTask *t3 = this->workflow->addTask("task3", 60.0002, 2, 4, 1.0);

        std::vector<wrench::WorkflowTask *> tasks;

        tasks.push_back(t1);
        tasks.push_back(t2);
        tasks.push_back(t3);
        wrench::StandardJob *job = job_manager->createStandardJob(tasks, {}, {}, {}, {});

        job_manager->submitJob(job, this->test->cs_fcfs_aggressive_maximum_maximum_flops_best_fit);

        // Wait for a workflow execution event
        std::unique_ptr<wrench::WorkflowExecutionEvent> event;
        try {
          event = workflow->waitForNextExecutionEvent();
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

        // Check completion states and times
        if ((t1->getState() != wrench::WorkflowTask::COMPLETED) ||
            (t2->getState() != wrench::WorkflowTask::COMPLETED) ||
            (t3->getState() != wrench::WorkflowTask::COMPLETED)) {
          throw std::runtime_error("Unexpected task states");
        }

        double task1_makespan = t1->getEndDate() - now;
        double task2_makespan = t2->getEndDate() - now;
        double task3_makespan = t3->getEndDate() - now;

//        WRENCH_INFO("t1:%lf t2:%lf t3:%lf", task1_makespan, task2_makespan, task3_makespan);


        if (!MultihostMulticoreComputeServiceTestScheduling::isJustABitGreater(30, task1_makespan) ||
            !MultihostMulticoreComputeServiceTestScheduling::isJustABitGreater(30, task2_makespan) ||
            !MultihostMulticoreComputeServiceTestScheduling::isJustABitGreater(15, task3_makespan)) {
          throw std::runtime_error("DEFAULT PROPERTIES / CASE 1: Unexpected task execution times "
                                           "t1: " + std::to_string(task1_makespan) +
                                   " t2: " + std::to_string(task2_makespan) +
                                   " t3: " + std::to_string(task3_makespan));
        }

        workflow->removeTask(t1);
        workflow->removeTask(t2);
        workflow->removeTask(t3);
      }



      /**********************************************/
      /** DEFAULT PROPERTIES / CASE 2              **/
      /**********************************************/
      {

        double now = wrench::S4U_Simulation::getClock();

        // Create a job with 3 tasks
        wrench::WorkflowTask *t1 = this->workflow->addTask("task1", 60.0001, 2, 3, 1.0);
        wrench::WorkflowTask *t2 = this->workflow->addTask("task2", 60.0000, 1, 2, 1.0);
        wrench::WorkflowTask *t3 = this->workflow->addTask("task3", 60.0002, 2, 4, 1.0);

        std::vector<wrench::WorkflowTask *> tasks;

        tasks.push_back(t1);
        tasks.push_back(t2);
        tasks.push_back(t3);
        wrench::StandardJob *job = job_manager->createStandardJob(tasks, {}, {}, {}, {});

        job_manager->submitJob(job, this->test->cs_fcfs_aggressive_maximum_maximum_flops_best_fit);

        // Wait for a workflow execution event
        std::unique_ptr<wrench::WorkflowExecutionEvent> event;
        try {
          event = workflow->waitForNextExecutionEvent();
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

        // Check completion states and times
        if ((t1->getState() != wrench::WorkflowTask::COMPLETED) ||
            (t2->getState() != wrench::WorkflowTask::COMPLETED) ||
            (t3->getState() != wrench::WorkflowTask::COMPLETED)) {
          throw std::runtime_error("Unexpected task states");
        }

        double task1_makespan = t1->getEndDate() - now;
        double task2_makespan = t2->getEndDate() - now;
        double task3_makespan = t3->getEndDate() - now;

//        WRENCH_INFO("t1:%lf t2:%lf t3:%lf", task1_makespan, task2_makespan, task3_makespan);

        if (!MultihostMulticoreComputeServiceTestScheduling::isJustABitGreater(20, task1_makespan) ||
            !MultihostMulticoreComputeServiceTestScheduling::isJustABitGreater(60, task2_makespan) ||
            !MultihostMulticoreComputeServiceTestScheduling::isJustABitGreater(15, task3_makespan)) {
          throw std::runtime_error("DEFAULT PROPERTIES / CASE 2: Unexpected task execution times "
                                           "t1: " + std::to_string(task1_makespan) +
                                   " t2: " + std::to_string(task2_makespan) +
                                   " t3: " + std::to_string(task3_makespan));
        }
        workflow->removeTask(t1);
        workflow->removeTask(t2);
        workflow->removeTask(t3);
      }



      /*******************************************************************/
      /** DEFAULT PROPERTIES BUT MIN CORE ALLOCATIONS / CASE 1          **/
      /*******************************************************************/
      {

        double now = wrench::S4U_Simulation::getClock();

        // Create a job with 3 tasks
        wrench::WorkflowTask *t1 = this->workflow->addTask("task1", 60.0000, 2, 3, 1.0);
        wrench::WorkflowTask *t2 = this->workflow->addTask("task2", 60.0001, 1, 2, 1.0);
        wrench::WorkflowTask *t3 = this->workflow->addTask("task3", 60.0002, 2, 4, 1.0);

        std::vector<wrench::WorkflowTask *> tasks;

        tasks.push_back(t1);
        tasks.push_back(t2);
        tasks.push_back(t3);
        wrench::StandardJob *job = job_manager->createStandardJob(tasks, {}, {}, {}, {});

        job_manager->submitJob(job, this->test->cs_fcfs_aggressive_minimum_maximum_flops_best_fit);

        // Wait for a workflow execution event
        std::unique_ptr<wrench::WorkflowExecutionEvent> event;
        try {
          event = workflow->waitForNextExecutionEvent();
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

        // Check completion states and times
        if ((t1->getState() != wrench::WorkflowTask::COMPLETED) ||
            (t2->getState() != wrench::WorkflowTask::COMPLETED) ||
            (t3->getState() != wrench::WorkflowTask::COMPLETED)) {
          throw std::runtime_error("Unexpected task states");
        }

        double task1_makespan = t1->getEndDate() - now;
        double task2_makespan = t2->getEndDate() - now;
        double task3_makespan = t3->getEndDate() - now;

//        WRENCH_INFO("t1:%lf t2:%lf t3:%lf", task1_makespan, task2_makespan, task3_makespan);

        if (!MultihostMulticoreComputeServiceTestScheduling::isJustABitGreater(30, task1_makespan) ||
            !MultihostMulticoreComputeServiceTestScheduling::isJustABitGreater(60, task2_makespan) ||
            !MultihostMulticoreComputeServiceTestScheduling::isJustABitGreater(30, task3_makespan)) {
          throw std::runtime_error(
                  "DEFAULT PROPERTIES BUT MINIMUM CORE ALLOCATION / CASE 1: Unexpected task execution times "
                          "t1: " + std::to_string(task1_makespan) +
                  " t2: " + std::to_string(task2_makespan) +
                  " t3: " + std::to_string(task3_makespan));
        }

        workflow->removeTask(t1);
        workflow->removeTask(t2);
        workflow->removeTask(t3);
      }

      /*******************************************************************/
      /** DEFAULT PROPERTIES BUT MAX_MIN_CORES TASK SELECTION / CASE 1  **/
      /*******************************************************************/
      {

        double now = wrench::S4U_Simulation::getClock();

        // Create a job with 3 tasks
        wrench::WorkflowTask *t1 = this->workflow->addTask("task1", 60.0000, 3, 4, 1.0);
        wrench::WorkflowTask *t2 = this->workflow->addTask("task2", 60.0001, 1, 2, 1.0);
        wrench::WorkflowTask *t3 = this->workflow->addTask("task3", 60.0002, 2, 4, 1.0);

        std::vector<wrench::WorkflowTask *> tasks;

        tasks.push_back(t1);
        tasks.push_back(t2);
        tasks.push_back(t3);
        wrench::StandardJob *job = job_manager->createStandardJob(tasks, {}, {}, {}, {});

        job_manager->submitJob(job, this->test->cs_fcfs_aggressive_maximum_maximum_minimum_cores_best_fit);

        // Wait for a workflow execution event
        std::unique_ptr<wrench::WorkflowExecutionEvent> event;
        try {
          event = workflow->waitForNextExecutionEvent();
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

        // Check completion states and times
        if ((t1->getState() != wrench::WorkflowTask::COMPLETED) ||
            (t2->getState() != wrench::WorkflowTask::COMPLETED) ||
            (t3->getState() != wrench::WorkflowTask::COMPLETED)) {
          throw std::runtime_error("Unexpected task states");
        }

        double task1_makespan = t1->getEndDate() - now;
        double task2_makespan = t2->getEndDate() - now;
        double task3_makespan = t3->getEndDate() - now;

//        WRENCH_INFO("t1:%lf t2:%lf t3:%lf", task1_makespan, task2_makespan, task3_makespan);

        if (!MultihostMulticoreComputeServiceTestScheduling::isJustABitGreater(15, task1_makespan) ||
            !MultihostMulticoreComputeServiceTestScheduling::isJustABitGreater(45, task2_makespan) ||
            !MultihostMulticoreComputeServiceTestScheduling::isJustABitGreater(15, task3_makespan)) {
          throw std::runtime_error(
                  "DEFAULT PROPERTIES BUT MAX_MIN_CORES TASK SELECTION / CASE 1 : Unexpected task execution times "
                          "t1: " + std::to_string(task1_makespan) +
                  " t2: " + std::to_string(task2_makespan) +
                  " t3: " + std::to_string(task3_makespan));
        }

        workflow->removeTask(t1);
        workflow->removeTask(t2);
        workflow->removeTask(t3);
      }


      // Terminate
      this->shutdownAllServices();
      return 0;
    }
};

TEST_F(MultihostMulticoreComputeServiceTestScheduling, OneJob) {
  DO_TEST_WITH_FORK(do_OneJob_test);
}

void MultihostMulticoreComputeServiceTestScheduling::do_OneJob_test() {

  // Create and initialize a simulation
  auto *simulation = new wrench::Simulation();
  int argc = 1;
  auto **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_task_test");

  simulation->init(&argc, argv);

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  std::set<wrench::ComputeService *> compute_services;
  // Create a Compute Service
  EXPECT_NO_THROW(cs_fcfs_aggressive_maximum_maximum_flops_best_fit = simulation->add(
          std::unique_ptr<wrench::MultihostMulticoreComputeService>(
                  new wrench::MultihostMulticoreComputeService(
                          "Host1", true, true,
                          (std::set<std::string>){"Host1", "Host2"},
                          nullptr,
                          {{wrench::MultihostMulticoreComputeServiceProperty::JOB_SELECTION_POLICY,                      "FCFS"},
                           {wrench::MultihostMulticoreComputeServiceProperty::RESOURCE_ALLOCATION_POLICY,                "aggressive"},
                           {wrench::MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_CORE_ALLOCATION_ALGORITHM, "maximum"},
                           {wrench::MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_TASK_SELECTION_ALGORITHM,  "maximum_flops"},
                           {wrench::MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_HOST_SELECTION_ALGORITHM,  "best_fit"}
                          }))));
  compute_services.insert(cs_fcfs_aggressive_maximum_maximum_flops_best_fit);

  // Create a Compute Service
  EXPECT_NO_THROW(cs_fcfs_aggressive_minimum_maximum_flops_best_fit = simulation->add(
          std::unique_ptr<wrench::MultihostMulticoreComputeService>(
                  new wrench::MultihostMulticoreComputeService(
                          "Host1", true, true,
                          (std::set<std::string>){"Host1","Host2"},
                          nullptr,
                          {{wrench::MultihostMulticoreComputeServiceProperty::JOB_SELECTION_POLICY,                      "FCFS"},
                           {wrench::MultihostMulticoreComputeServiceProperty::RESOURCE_ALLOCATION_POLICY,                "aggressive"},
                           {wrench::MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_CORE_ALLOCATION_ALGORITHM, "minimum"},
                           {wrench::MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_TASK_SELECTION_ALGORITHM,  "maximum_flops"},
                           {wrench::MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_HOST_SELECTION_ALGORITHM,  "best_fit"}
                          }))));
  compute_services.insert(cs_fcfs_aggressive_minimum_maximum_flops_best_fit);

  // Create a Compute Service
  EXPECT_NO_THROW(cs_fcfs_aggressive_maximum_maximum_minimum_cores_best_fit = simulation->add(
          std::unique_ptr<wrench::MultihostMulticoreComputeService>(
                  new wrench::MultihostMulticoreComputeService(
                          "Host1", true, true,
                          (std::set<std::string>){"Host1", "Host2"},
                          nullptr,
                          {{wrench::MultihostMulticoreComputeServiceProperty::JOB_SELECTION_POLICY,                      "FCFS"},
                           {wrench::MultihostMulticoreComputeServiceProperty::RESOURCE_ALLOCATION_POLICY,                "aggressive"},
                           {wrench::MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_CORE_ALLOCATION_ALGORITHM, "maximum"},
                           {wrench::MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_TASK_SELECTION_ALGORITHM,  "maximum_minimum_cores"},
                           {wrench::MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_HOST_SELECTION_ALGORITHM,  "best_fit"}
                          }))));
  compute_services.insert(cs_fcfs_aggressive_maximum_maximum_minimum_cores_best_fit);

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          std::unique_ptr<wrench::WMS>(new OneJobTestWMS(
                  this, std::unique_ptr<wrench::Scheduler>(
                          new NoopScheduler()), compute_services, {}, "Host1"))));

  EXPECT_NO_THROW(wms->addWorkflow(workflow));

  EXPECT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}


/**********************************************************************/
/**  MULTIJOB SIMULATION TEST                                         **/
/**********************************************************************/

class MultiJobTestWMS : public wrench::WMS {

public:
    MultiJobTestWMS(MultihostMulticoreComputeServiceTestScheduling *test,
                    std::unique_ptr<wrench::Scheduler> scheduler,
                    const std::set<wrench::ComputeService *> &compute_services,
                    const std::set<wrench::StorageService *> &storage_services,
                    std::string hostname) :
            wrench::WMS(std::move(scheduler), compute_services, storage_services, hostname, "test") {
      this->test = test;
    }


private:

    MultihostMulticoreComputeServiceTestScheduling *test;

    int main() {

      // Create a job manager
      std::unique_ptr<wrench::JobManager> job_manager =
              std::unique_ptr<wrench::JobManager>(new wrench::JobManager(this->workflow));


      /**********************************************/
      /** DEFAULT_PROPERTIES / CASE 1              **/
      /**********************************************/
      {

        double now = wrench::S4U_Simulation::getClock();

        // Submit a PilotJob that lasts 3600 seconds and takes 2 cores and 0 bytes per host
        wrench::PilotJob *pilot_job = job_manager->createPilotJob(this->workflow, 2, 2, 0, 3600);

        job_manager->submitJob(pilot_job, this->test->cs_fcfs_aggressive_maximum_maximum_flops_best_fit);



        // Create and submit a job with 2 2-core tasks
        wrench::WorkflowTask *t1_1 = this->workflow->addTask("task1_1", 8000, 2, 2, 1.0);
        wrench::WorkflowTask *t1_2 = this->workflow->addTask("task1_2", 8000, 2, 2, 1.0);

        std::vector<wrench::WorkflowTask *> job1_tasks;
        job1_tasks.push_back(t1_1);
        job1_tasks.push_back(t1_2);
        wrench::StandardJob *job1 = job_manager->createStandardJob(job1_tasks, {}, {}, {}, {});

        job_manager->submitJob(job1, this->test->cs_fcfs_aggressive_maximum_maximum_flops_best_fit);

        // Create and submit a job with a 2-core task and a 1-core task
        wrench::WorkflowTask *t2_1 = this->workflow->addTask("task2_1", 60, 2, 2, 1.0);
        wrench::WorkflowTask *t2_2 = this->workflow->addTask("task2_2", 60, 1, 1, 1.0);

        std::vector<wrench::WorkflowTask *> job2_tasks;
        job2_tasks.push_back(t2_1);
        job2_tasks.push_back(t2_2);
        wrench::StandardJob *job2 = job_manager->createStandardJob(job2_tasks, {}, {}, {}, {});

        job_manager->submitJob(job2, this->test->cs_fcfs_aggressive_maximum_maximum_flops_best_fit);


        {
          // Wait for a PILOT JOB STARTED execution event
          std::unique_ptr<wrench::WorkflowExecutionEvent> event;
          try {
            event = workflow->waitForNextExecutionEvent();
          } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
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

          if (!MultihostMulticoreComputeServiceTestScheduling::isJustABitGreater(0,
                                                                                 wrench::S4U_Simulation::getClock())) {
            throw std::runtime_error("Pilot job should start_daemon at time 0");
          }
        }


        {
          // Wait for a PILOT JOB EXPIRED execution event
          std::unique_ptr<wrench::WorkflowExecutionEvent> event;
          try {
            event = workflow->waitForNextExecutionEvent();
          } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
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

          if (!MultihostMulticoreComputeServiceTestScheduling::isJustABitGreater(3600,
                                                                                 wrench::S4U_Simulation::getClock())) {
            throw std::runtime_error("Pilot job should expire at time 3600");
          }
        }


        {
          // Wait for a STANDARD JOB COMPLETED execution event
          std::unique_ptr<wrench::WorkflowExecutionEvent> event;
          try {
            event = workflow->waitForNextExecutionEvent();
          } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
          }
          switch (event->type) {
            case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
              if (event->job != job2) {
                throw std::runtime_error("Unexpected job completion (job2 should complete first!)");
              }
              // success, do nothing for now
              break;
            }
            default: {
              throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
            }
          }
          if (!MultihostMulticoreComputeServiceTestScheduling::isJustABitGreater(3600 + 60,
                                                                                 wrench::S4U_Simulation::getClock())) {
            throw std::runtime_error("Standard job #2 should complete at time 3600 + 60");
          }
        }

        {
          // Wait for a STANDARD JOB COMPLETION execution event
          std::unique_ptr<wrench::WorkflowExecutionEvent> event;
          try {
            event = workflow->waitForNextExecutionEvent();
          } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
          }
          switch (event->type) {
            case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
              if (event->job != job1) {
                throw std::runtime_error("Unexpected job completion (job1 should complete last!)");
              }
              // success, do nothing for now
              break;
            }
            default: {
              throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
            }
          }

          if (!MultihostMulticoreComputeServiceTestScheduling::isJustABitGreater(4000,
                                                                                 wrench::S4U_Simulation::getClock())) {
            throw std::runtime_error("Standard job #1 should complete at time 4000");
          }
        }

        workflow->removeTask(t1_1);
        workflow->removeTask(t1_2);
        workflow->removeTask(t2_1);
        workflow->removeTask(t2_2);
      }

      // Terminate
      this->shutdownAllServices();
      return 0;
    }
};

TEST_F(MultihostMulticoreComputeServiceTestScheduling, MultiJob) {
  DO_TEST_WITH_FORK(do_MultiJob_test);
}

void MultihostMulticoreComputeServiceTestScheduling::do_MultiJob_test() {

  // Create and initialize a simulation
  auto *simulation = new wrench::Simulation();
  int argc = 1;
  auto **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_task_test");

  simulation->init(&argc, argv);

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Create a Compute Service
  EXPECT_NO_THROW(cs_fcfs_aggressive_maximum_maximum_flops_best_fit = simulation->add(
          std::unique_ptr<wrench::MultihostMulticoreComputeService>(
                  new wrench::MultihostMulticoreComputeService("Host1", true, true,
                                                               (std::set<std::string>){"Host1", "Host2"},
                                                               nullptr,
                                                               {{wrench::MultihostMulticoreComputeServiceProperty::JOB_SELECTION_POLICY,                      "FCFS"},
                                                                {wrench::MultihostMulticoreComputeServiceProperty::RESOURCE_ALLOCATION_POLICY,                "aggressive"},
                                                                {wrench::MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_CORE_ALLOCATION_ALGORITHM, "maximum"},
                                                                {wrench::MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_TASK_SELECTION_ALGORITHM,  "maximum_flops"},
                                                                {wrench::MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_HOST_SELECTION_ALGORITHM,  "best_fit"}
                                                               }))));
  std::set<wrench::ComputeService *> compute_services;
  compute_services.insert(cs_fcfs_aggressive_maximum_maximum_flops_best_fit);

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          std::unique_ptr<wrench::WMS>(new MultiJobTestWMS(
                  this, std::unique_ptr<wrench::Scheduler>(
                          new NoopScheduler()), compute_services, {}, "Host1"))));

  EXPECT_NO_THROW(wms->addWorkflow(workflow));

  EXPECT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}
