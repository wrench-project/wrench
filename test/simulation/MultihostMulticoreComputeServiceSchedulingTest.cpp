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
#include "NoopScheduler.h"
#include "TestWithFork.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Log category for test");

#define EPSILON 0.05


class MultihostMulticoreComputeServiceTestScheduling : public ::testing::Test {


public:
    wrench::ComputeService *compute_service = nullptr;

    void do_OneJob_test();


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
              "   <AS id=\"AS0\" routing=\"Full\"> "
              "       <host id=\"Host1\" speed=\"1f\" core=\"4\"/> "
              "       <host id=\"Host2\" speed=\"1f\" core=\"4\"/> "
              "        <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
              "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\"/> </route>"
              "   </AS> "
              "</platform>";
      FILE *platform_file = fopen(platform_file_path.c_str(), "w");
      fprintf(platform_file, "%s", xml.c_str());
      fclose(platform_file);

    }

    std::string platform_file_path = "/tmp/platform.xml";
    wrench::Workflow *workflow;
};


/**********************************************************************/
/**  NOOP SIMULATION TEST                                            **/
/**********************************************************************/

class OneJobTestWMS : public wrench::WMS {

public:
    OneJobTestWMS(MultihostMulticoreComputeServiceTestScheduling *test,
                  wrench::Workflow *workflow,
                  std::unique_ptr<wrench::Scheduler> scheduler,
                  std::string hostname) :
            wrench::WMS(workflow, std::move(scheduler), hostname, "test") {
      this->test = test;
    }


private:

    MultihostMulticoreComputeServiceTestScheduling *test;

    int main() {

      // Create a job manager
      std::unique_ptr<wrench::JobManager> job_manager =
              std::unique_ptr<wrench::JobManager>(new wrench::JobManager(this->workflow));

      /**********************************************/
      /** CASE 1                                   **/
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

        job_manager->submitJob(job, this->test->compute_service);

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

        if (!MultihostMulticoreComputeServiceTestScheduling::isJustABitGreater(now + 30, task1_makespan) ||
            !MultihostMulticoreComputeServiceTestScheduling::isJustABitGreater(now + 30, task2_makespan) ||
            !MultihostMulticoreComputeServiceTestScheduling::isJustABitGreater(now + 15, task3_makespan)) {
          throw std::runtime_error("CASE 1: Unexpected task execution times "
                                           "t1: " + std::to_string(task1_makespan) +
                                   " t2: " + std::to_string(task2_makespan) +
                                   " t3: " + std::to_string(task3_makespan));
        }

        workflow->removeTask(t1);
        workflow->removeTask(t2);
        workflow->removeTask(t3);
      }

      /**********************************************/
      /** CASE 2                                   **/
      /**********************************************/
      {

        double now = wrench::S4U_Simulation::getClock();
        WRENCH_INFO("NOW = %lf", now);

        // Create a job with 3 tasks
        wrench::WorkflowTask *t1 = this->workflow->addTask("task1", 60.0001, 2, 3, 1.0);
        wrench::WorkflowTask *t2 = this->workflow->addTask("task2", 60.0000, 1, 2, 1.0);
        wrench::WorkflowTask *t3 = this->workflow->addTask("task3", 60.0002, 2, 4, 1.0);

        std::vector<wrench::WorkflowTask *> tasks;

        tasks.push_back(t1);
        tasks.push_back(t2);
        tasks.push_back(t3);
        wrench::StandardJob *job = job_manager->createStandardJob(tasks, {}, {}, {}, {});

        job_manager->submitJob(job, this->test->compute_service);

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
          throw std::runtime_error("CASE 2: Unexpected task execution times "
                                           "t1: " + std::to_string(task1_makespan) +
                                   " t2: " + std::to_string(task2_makespan) +
                                   " t3: " + std::to_string(task3_makespan));
        }
      }



      // Terminate
      this->simulation->shutdownAllComputeServices();
      this->simulation->shutdownAllStorageServices();
      return 0;
    }
};

TEST_F(MultihostMulticoreComputeServiceTestScheduling, Noop) {
  DO_TEST_WITH_FORK(do_OneJob_test);
}

void MultihostMulticoreComputeServiceTestScheduling::do_OneJob_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_task_test");

  simulation->init(&argc, argv);

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Create a WMS
  EXPECT_NO_THROW(wrench::WMS *wms = simulation->setWMS(
          std::unique_ptr<wrench::WMS>(new OneJobTestWMS(this, workflow,
                                                         std::unique_ptr<wrench::Scheduler>(
                                                                 new NoopScheduler()),
                          "Host1"))));

  // Create a Compute Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MultihostMulticoreComputeService>(
                  new wrench::MultihostMulticoreComputeService("Host1", true, true,
                                                               {std::make_pair("Host1", 0),
                                                                std::make_pair("Host2", 0)},
                                                               nullptr,
                                                               {{wrench::MultihostMulticoreComputeServiceProperty::JOB_SELECTION_POLICY, "FCFS"},
                                                                {wrench::MultihostMulticoreComputeServiceProperty::RESOURCE_ALLOCATION_POLICY, "aggressive"},
                                                               }))));

  EXPECT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}

