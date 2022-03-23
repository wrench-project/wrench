/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <gtest/gtest.h>

#include <wrench-dev.h>

#include "../include/TestWithFork.h"
#include "../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(parallel_model_test, "Log category for Parallel Model Test");


class ParallelModelTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;
    std::shared_ptr<wrench::WorkflowTask> task = nullptr;

    void do_AdmdahlParallelModelTest_test();
    void do_ConstantEfficiencyParallelModelTest_test();
    void do_CustomParallelModelTest_test();

protected:
    ParallelModelTest() {


        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"WMSHost\" speed=\"1f\" core=\"4\">"
                          "       </host>"
                          "       <link id=\"1\" bandwidth=\"1Tbps\" latency=\"0us\"/>"
                          "       <route src=\"WMSHost\" dst=\"WMSHost\"> <link_ctn id=\"1\"/> </route>"
                          "   </zone> "
                          "</platform>";

        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);

    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};


class ParallelModelTestWMS : public wrench::ExecutionController {
public:
    ParallelModelTestWMS(ParallelModelTest *test, std::shared_ptr<wrench::Workflow> workflow, std::string &hostname) :
            wrench::ExecutionController(hostname, "test"), test(test), workflow(workflow) {
    }

private:
    ParallelModelTest *test;
    std::shared_ptr<wrench::Workflow> workflow;

    int main() {
        auto job_manager = this->createJobManager();

        auto job = job_manager->createStandardJob(this->workflow->getTaskByID("task1"));
        job_manager->submitJob(job, this->test->compute_service);

        std::shared_ptr<wrench::ExecutionEvent> event;
        try {
            event = this->waitForNextEvent();
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }

        if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
            throw std::runtime_error("Job should have completed!");
        }

        return 0;
    }
};

/**********************************************************************/
/**            AmdahlParallelModelTest                               **/
/**********************************************************************/

TEST_F(ParallelModelTest, AmdahlParallelModelTest) {
    DO_TEST_WITH_FORK(do_AdmdahlParallelModelTest_test);
}

void ParallelModelTest::do_AdmdahlParallelModelTest_test() {
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
//    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    std::string wms_host = "WMSHost";

    ASSERT_NO_THROW(compute_service = simulation->add(new wrench::BareMetalComputeService(
            wms_host,
            {std::make_pair(
                    wms_host,
                    std::make_tuple(
                            wrench::ComputeService::ALL_CORES,
                            wrench::ComputeService::ALL_RAM))}, "",
            {})));

    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    auto workflow = wrench::Workflow::createWorkflow();
    ASSERT_NO_THROW(wms = simulation->add(new ParallelModelTestWMS(
            this, workflow, wms_host)));
    double work = 100.0;
    double alpha = 0.3;
    this->task = workflow->addTask("task1", work, 1, 4, 0.0);
    task->setParallelModel(wrench::ParallelModel::AMDAHL(alpha));

    auto parallel_model = task->getParallelModel();
    auto real_parallel_model = std::dynamic_pointer_cast<wrench::AmdahlParallelModel>(parallel_model);
    ASSERT_NE(real_parallel_model, nullptr);
    ASSERT_EQ(real_parallel_model->getAlpha(), 0.3);

    ASSERT_NO_THROW(simulation->launch());

    double makespan = task->getComputationEndDate() - task->getComputationStartDate();
    double expected_makespan = (alpha * work)  / 4 + (1 - alpha) * work;

    if (std::abs(makespan - expected_makespan) > 0.001) {
        throw std::runtime_error("Unexpected task1 makespan: " + std::to_string(makespan) +
        " instead of " + std::to_string(expected_makespan));
    }

    workflow->clear();

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**            ConstantEfficiencyParallelModelTest                   **/
/**********************************************************************/


TEST_F(ParallelModelTest, ConstantEfficiencyParallelModelTest) {
    DO_TEST_WITH_FORK(do_ConstantEfficiencyParallelModelTest_test);
}

void ParallelModelTest::do_ConstantEfficiencyParallelModelTest_test() {
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
//    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    std::string wms_host = "WMSHost";

    ASSERT_NO_THROW(compute_service = simulation->add(new wrench::BareMetalComputeService(
            wms_host,
            {std::make_pair(
                    wms_host,
                    std::make_tuple(
                            wrench::ComputeService::ALL_CORES,
                            wrench::ComputeService::ALL_RAM))}, "",
            {})));

    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    auto workflow = wrench::Workflow::createWorkflow();
    ASSERT_NO_THROW(wms = simulation->add(new ParallelModelTestWMS(this, workflow, wms_host)));

    double work = 100.0;
    double efficiency = 0.3;
    this->task = workflow->addTask("task1", work, 1, 4, 0.0);
    task->setParallelModel(wrench::ParallelModel::CONSTANTEFFICIENCY(efficiency));

    auto parallel_model = task->getParallelModel();
    auto real_parallel_model = std::dynamic_pointer_cast<wrench::ConstantEfficiencyParallelModel>(parallel_model);
    ASSERT_NE(real_parallel_model, nullptr);
    ASSERT_EQ(real_parallel_model->getEfficiency(), 0.3);

    ASSERT_NO_THROW(simulation->launch());

    double makespan = task->getComputationEndDate() - task->getComputationStartDate();
    double expected_makespan = work  / (4 * efficiency);

    WRENCH_INFO("--> %lf %lf\n", makespan, expected_makespan);

    if (std::abs(makespan - expected_makespan) > 0.001) {
        throw std::runtime_error("Unexpected task1 makespan: " + std::to_string(makespan) +
                                 " instead of " + std::to_string(expected_makespan));
    }

    workflow->clear();

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**            CustomParallelModelTest                               **/
/**********************************************************************/


TEST_F(ParallelModelTest, CustomParallelModelTest) {
    DO_TEST_WITH_FORK(do_CustomParallelModelTest_test);
}

void ParallelModelTest::do_CustomParallelModelTest_test() {
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
//    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    std::string wms_host = "WMSHost";

    ASSERT_NO_THROW(compute_service = simulation->add(new wrench::BareMetalComputeService(
            wms_host,
            {std::make_pair(
                    wms_host,
                    std::make_tuple(
                            wrench::ComputeService::ALL_CORES,
                            wrench::ComputeService::ALL_RAM))}, "",
            {})));

    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    auto workflow = wrench::Workflow::createWorkflow();
    ASSERT_NO_THROW(wms = simulation->add(new ParallelModelTestWMS(
            this, workflow, wms_host)));

    double work = 100.0;
    this->task = workflow->addTask("task1", work, 1, 4, 0.0);
    task->setParallelModel(wrench::ParallelModel::CUSTOM(
            [] (double work, unsigned long num_threads) {
                return 0.1*work;
            },
            [] (double work, unsigned long num_threads) {
                return 0.9*work / num_threads;
            }
            ));

    auto parallel_model = task->getParallelModel();
    auto real_parallel_model = std::dynamic_pointer_cast<wrench::CustomParallelModel>(parallel_model);
    ASSERT_NE(real_parallel_model, nullptr);

    ASSERT_NO_THROW(simulation->launch());

    double makespan = task->getComputationEndDate() - task->getComputationStartDate();
    double expected_makespan = 0.1 * work + 0.9 * work / 4;

    if (std::abs(makespan - expected_makespan) > 0.001) {
        throw std::runtime_error("Unexpected task1 makespan: " + std::to_string(makespan) +
                                 " instead of " + std::to_string(expected_makespan));
    }

    workflow->clear();

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
