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

XBT_LOG_NEW_DEFAULT_CATEGORY(test_bare_metalk_compute_service_scheduling, "Log category for BareMetalComputeServiceTestScheduling");

#define EPSILON 0.005


class BareMetalComputeServiceTestScheduling : public ::testing::Test {


public:
    // Default
    std::shared_ptr<wrench::ComputeService> cs = nullptr;

    // Old Default
    std::shared_ptr<wrench::ComputeService> cs_fcfs_aggressive_maximum_maximum_flops_best_fit = nullptr;
    // "minimum" core allocation
    std::shared_ptr<wrench::ComputeService> cs_fcfs_aggressive_minimum_maximum_flops_best_fit = nullptr;
    // "maximum_minimum_cores" task selection
    std::shared_ptr<wrench::ComputeService> cs_fcfs_aggressive_maximum_maximum_minimum_cores_best_fit = nullptr;

    void do_RAMPressure_test();
    void do_LoadBalancing1_test();
    void do_LoadBalancing2_test();


    static bool isJustABitGreater(double base, double variable) {
        return ((variable > base) && (variable < base + EPSILON));
    }

    static bool isAboutTheSame(double base, double variable) {
        return ((std::abs<double>(variable - base) < EPSILON));
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
                          "       <host id=\"Host3\" speed=\"3f\" core=\"4\"> "
                          "            <prop id=\"ram\" value=\"1000\"/> "
                          "       </host> "
                          "        <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                          "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host1\" dst=\"Host3\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host2\" dst=\"Host3\"> <link_ctn id=\"1\"/> </route>"
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
/**  RAM PRESSURE TEST                                               **/
/**********************************************************************/

class RAMPressureTestWMS : public wrench::WMS {

public:
    RAMPressureTestWMS(BareMetalComputeServiceTestScheduling *test,
                       const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                       const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                       std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }


private:

    BareMetalComputeServiceTestScheduling *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

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

        auto ram_availabilities = this->test->cs->getPerHostAvailableMemoryCapacity();
        if ((not BareMetalComputeServiceTestScheduling::isAboutTheSame(ram_availabilities["Host1"],  500)) ||
            (not BareMetalComputeServiceTestScheduling::isAboutTheSame(ram_availabilities["Host2"], 1000))) {
            throw std::runtime_error("Unexpected memory availabilities");
        }


        // Wait for completions
        std::map<wrench::WorkflowTask*, std::tuple<double,double>> times;
        for (int i=0; i < 4; i++) {
            std::shared_ptr<wrench::WorkflowExecutionEvent> event;
            try {
                event = this->getWorkflow()->waitForNextExecutionEvent();
            } catch (wrench::WorkflowExecutionException &e) {
                throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
            }
            if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
                throw std::runtime_error("Unexpected execution event: " + event->toString());
            }

            wrench::StandardJob *job = std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)->standard_job;
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
                                                (std::set<std::string>){"Host1", "Host2"}, {},
                                                {}, {})));
    std::set<std::shared_ptr<wrench::ComputeService>> compute_services;
    compute_services.insert(cs);

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new RAMPressureTestWMS(
                    this, compute_services, {}, "Host1")));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}




/**********************************************************************/
/**  LOAD-BALANCING  TEST  #1                                        **/
/**********************************************************************/

class LoadBalancing1TestWMS : public wrench::WMS {

public:
    LoadBalancing1TestWMS(BareMetalComputeServiceTestScheduling *test,
                          const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                          const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                          std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }


private:

    BareMetalComputeServiceTestScheduling *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a few tasks
        std::vector<wrench::WorkflowTask *> tasks;
        tasks.push_back(this->getWorkflow()->addTask("task1", 100, 4, 4, 1.0, 500));
        tasks.push_back(this->getWorkflow()->addTask("task2", 100, 4, 4, 1.0, 500));
        tasks.push_back(this->getWorkflow()->addTask("task3", 100, 4, 4, 1.0, 500));
        tasks.push_back(this->getWorkflow()->addTask("task4", 100, 4, 4, 1.0, 500));

        // Submit them in order
        for (auto const & t : tasks) {
            wrench::StandardJob *j = job_manager->createStandardJob(t, {});
            std::map<std::string, std::string> cs_specific_args;
            cs_specific_args.insert(std::make_pair(t->getID(), ""));
            job_manager->submitJob(j, this->test->cs, cs_specific_args);
        }

        // Wait for completions
        std::map<wrench::WorkflowTask*, std::tuple<double,double>> times;
        for (int i=0; i < 4; i++) {
            std::shared_ptr<wrench::WorkflowExecutionEvent> event;
            try {
                event = this->getWorkflow()->waitForNextExecutionEvent();
            } catch (wrench::WorkflowExecutionException &e) {
                throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
            }

            auto real_event = std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event);
            if (not real_event) {
                throw std::runtime_error("Unexpected execution event: " + event->toString());
            }

            wrench::StandardJob *job = real_event->standard_job;
            wrench::WorkflowTask *task = *(job->getTasks().begin());
            double start_time = task->getStartDate();
            double end_time = task->getEndDate();
            times.insert(std::make_pair(task, std::make_tuple(start_time, end_time)));
        }

        // Inspect hosts
        std::vector<std::string> hosts;
        hosts.push_back(tasks.at(0)->getExecutionHost());
        hosts.push_back(tasks.at(1)->getExecutionHost());
        hosts.push_back(tasks.at(2)->getExecutionHost());
        hosts.push_back(tasks.at(3)->getExecutionHost());

        int host1_count = 0;
        int host2_count = 0;
        for (auto const &h : hosts) {
            if (h == "Host1") {
                host1_count++;
            }
            if (h == "Host2") {
                host2_count++;
            }
        }
        if ((host1_count != 2) or (host2_count != 2)) {
            throw std::runtime_error("Unexpecting execution hosts: " + hosts.at(0) + ", " +
                                     hosts.at(1) + ", " + hosts.at(2) + ", " + hosts.at(3));
        }

        return 0;
    }


};

TEST_F(BareMetalComputeServiceTestScheduling, LoadBalancing1) {
    DO_TEST_WITH_FORK(do_LoadBalancing1_test);
}

void BareMetalComputeServiceTestScheduling::do_LoadBalancing1_test() {

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
                                                (std::set<std::string>){"Host1", "Host2"}, "",
                                                {}, {})));
    std::set<std::shared_ptr<wrench::ComputeService>> compute_services;
    compute_services.insert(cs);

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new LoadBalancing1TestWMS(
                    this, compute_services, {}, "Host1")));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}




/**********************************************************************/
/**  LOAD-BALANCING  TEST  #2 (HETEROGENEOUS: 1 1x host, 1 3x host)  **/
/**********************************************************************/

class LoadBalancing2TestWMS : public wrench::WMS {

public:
    LoadBalancing2TestWMS(BareMetalComputeServiceTestScheduling *test,
                          const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                          const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                          std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }


private:

    BareMetalComputeServiceTestScheduling *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a few tasks
        std::vector<wrench::WorkflowTask *> tasks;
        tasks.push_back(this->getWorkflow()->addTask("task1", 100, 4, 4, 1.0, 100));
        tasks.push_back(this->getWorkflow()->addTask("task2", 100, 4, 4, 1.0, 100));
        tasks.push_back(this->getWorkflow()->addTask("task3", 100, 4, 4, 1.0, 100));
        tasks.push_back(this->getWorkflow()->addTask("task4", 100, 4, 4, 1.0, 100));

        // Submit them in order
        for (auto const & t : tasks) {
            wrench::StandardJob *j = job_manager->createStandardJob(t, {});
            std::map<std::string, std::string> cs_specific_args;
            cs_specific_args.insert(std::make_pair(t->getID(), ""));
            job_manager->submitJob(j, this->test->cs, cs_specific_args);
        }

        // Wait for completions
        std::map<wrench::WorkflowTask*, std::tuple<double,double>> times;
        for (int i=0; i < 4; i++) {
            std::shared_ptr<wrench::WorkflowExecutionEvent> event;
            try {
                event = this->getWorkflow()->waitForNextExecutionEvent();
            } catch (wrench::WorkflowExecutionException &e) {
                throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
            }
            auto real_event = std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event);

            if (not real_event) {
                throw std::runtime_error("Unexpected execution event: " + event->toString());
            }

            wrench::StandardJob *job = real_event->standard_job;
            wrench::WorkflowTask *task = *(job->getTasks().begin());
            double start_time = task->getStartDate();
            double end_time = task->getEndDate();
            times.insert(std::make_pair(task, std::make_tuple(start_time, end_time)));
        }

        // Inspect hosts
        std::vector<std::string> hosts;
        hosts.push_back(tasks.at(0)->getExecutionHost());
        hosts.push_back(tasks.at(1)->getExecutionHost());
        hosts.push_back(tasks.at(2)->getExecutionHost());
        hosts.push_back(tasks.at(3)->getExecutionHost());

        int host1_count = 0;
        int host3_count = 0;
        for (auto const &h : hosts) {
            if (h == "Host1") {
                host1_count++;
            }
            if (h == "Host3") {
                host3_count++;
            }
        }
        if ((host1_count != 1) or (host3_count != 3)) {
            throw std::runtime_error("Unexpecting execution hosts: " + hosts.at(0) + ", " +
                                     hosts.at(1) + ", " + hosts.at(2) + ", " + hosts.at(3));
        }

        return 0;
    }


};

TEST_F(BareMetalComputeServiceTestScheduling, LoadBalancing2) {
    DO_TEST_WITH_FORK(do_LoadBalancing2_test);
}

void BareMetalComputeServiceTestScheduling::do_LoadBalancing2_test() {

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
                                                (std::set<std::string>){"Host1", "Host3"}, "",
                                                {}, {})));
    std::set<std::shared_ptr<wrench::ComputeService>> compute_services;
    compute_services.insert(cs);

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new LoadBalancing2TestWMS(
                    this, compute_services, {}, "Host1")));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}
