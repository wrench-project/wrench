/**
 * Copyright (c) 2017-2019. The WRENCH Team.
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

WRENCH_LOG_CATEGORY(workflow_task_test, "Log category for Workflow Task Test");


class WorkflowTaskTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service = nullptr, backup_storage_service = nullptr;
    std::shared_ptr<wrench::FileRegistryService> file_registry_service = nullptr;

    std::unique_ptr<wrench::Workflow> workflow;
    wrench::WorkflowTask *t1, *t2, *t4, *t5, *t6;
    wrench::WorkflowFile *large_input_file, *small_input_file, *t4_output_file;

    void do_WorkflowTaskExecutionHistory_test();

protected:
    WorkflowTaskTest() {
        workflow = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());

        t1 = workflow->addTask("task-01", 100000, 1, 1, 0);
        t2 = workflow->addTask("task-02", 100, 2, 4, 0);
        t2->setParallelModel(wrench::ParallelModel::CONSTANTEFFICIENCY(0.5));
        t2->setAverageCPU(90.2);

        workflow->addControlDependency(t1, t2);

        // t3 is created in InputOutputFile test..
        t4 = workflow->addTask("task-04", 10, 1, 3, 0);


        large_input_file = workflow->addFile("large_input_file", 1000000);
        small_input_file = workflow->addFile("zz_small_input_file", 10);
        t4_output_file = workflow->addFile("t4_output_file", 1000);

        t4->addInputFile(small_input_file);
        t4->addInputFile(large_input_file);
        t4->addOutputFile(t4_output_file);
        t4->setBytesRead(1000010);
        t4->setBytesWritten(1000);
        t4->setAverageCPU(50.5);

        t5 = workflow->addTask("task-05", 100, 1, 2, 0);
        t6 = workflow->addTask("task-06", 100, 1, 3, 0);

        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"WMSHost\" speed=\"1f\" core=\"1\">"
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"other_large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/backup\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"ExecutionHost\" speed=\"1f\" core=\"3\"/> "
                          "       <link id=\"1\" bandwidth=\"1Gbps\" latency=\"1000000us\"/>"
                          "       <route src=\"ExecutionHost\" dst=\"WMSHost\"> <link_ctn id=\"1\"/> </route>"
                          "   </zone> "
                          "</platform>";

        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);

    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};

TEST_F(WorkflowTaskTest, TaskStructure) {
    // WorkflowTask structure sanity check
    ASSERT_EQ(t1->getWorkflow(), workflow.get());

    ASSERT_NE(t1->getID(), t2->getID());
    ASSERT_EQ(t1->getID(), "task-01");
    ASSERT_NE(t2->getID(), "task-01");

    ASSERT_GT(t1->getFlops(), t2->getFlops());

    ASSERT_EQ(t1->getMinNumCores(), 1);
    ASSERT_EQ(t1->getMaxNumCores(), 1);
    ASSERT_EQ(t2->getMinNumCores(), 2);
    ASSERT_EQ(t2->getMaxNumCores(), 4);

    ASSERT_EQ(t1->getState(), wrench::WorkflowTask::State::READY);
    ASSERT_EQ(t2->getState(), wrench::WorkflowTask::State::NOT_READY); // due to control dependency

    ASSERT_EQ(t1->getJob(), nullptr);
    ASSERT_EQ(t2->getJob(), nullptr);

    ASSERT_EQ(t1->getNumberOfParents(), 0);
    ASSERT_EQ(t1->getParents().size(), 0);
    ASSERT_EQ(t2->getNumberOfParents(), 1);
    ASSERT_EQ(t2->getParents().size(), 1);
    ASSERT_EQ(t1->getNumberOfChildren(), 1);
    ASSERT_EQ(t1->getChildren().size(), 1);
    ASSERT_EQ(t2->getNumberOfChildren(), 0);
    ASSERT_EQ(t2->getChildren().size(), 0);

    ASSERT_EQ(t1->getClusterID(), "");
}

TEST_F(WorkflowTaskTest, GetSet) {
    t1->setInternalState(wrench::WorkflowTask::InternalState::TASK_NOT_READY);
    ASSERT_EQ(t1->getInternalState(), wrench::WorkflowTask::InternalState::TASK_NOT_READY);

    t1->setClusterID("my-cluster-id");
    ASSERT_EQ(t1->getClusterID(), "my-cluster-id");

    t1->setInternalState(wrench::WorkflowTask::InternalState::TASK_READY);
    ASSERT_EQ(t1->getInternalState(), wrench::WorkflowTask::InternalState::TASK_READY);
    ASSERT_EQ(t2->getInternalState(), wrench::WorkflowTask::InternalState::TASK_NOT_READY);


    t1->setInternalState(wrench::WorkflowTask::InternalState::TASK_RUNNING);
    ASSERT_EQ(t1->getInternalState(), wrench::WorkflowTask::InternalState::TASK_RUNNING);

    t1->setInternalState(wrench::WorkflowTask::InternalState::TASK_COMPLETED);
    ASSERT_EQ(t1->getInternalState(), wrench::WorkflowTask::InternalState::TASK_COMPLETED);
    t2->setInternalState(wrench::WorkflowTask::InternalState::TASK_READY);
    ASSERT_EQ(t2->getInternalState(), wrench::WorkflowTask::InternalState::TASK_READY);

    // without setting a start date first, the following 10 'setXXXXXX' should fail
    ASSERT_THROW(t1->setReadInputStartDate(1.0), std::runtime_error);
    ASSERT_THROW(t1->setReadInputEndDate(1.0), std::runtime_error);
    ASSERT_THROW(t1->setComputationStartDate(1.0), std::runtime_error);
    ASSERT_THROW(t1->setComputationEndDate(1.0), std::runtime_error);
    ASSERT_THROW(t1->setWriteOutputStartDate(1.0), std::runtime_error);
    ASSERT_THROW(t1->setWriteOutputEndDate(1.0), std::runtime_error);
    ASSERT_THROW(t1->setEndDate(1.0), std::runtime_error);
    ASSERT_THROW(t1->setFailureDate(1.0), std::runtime_error);
    ASSERT_THROW(t1->setNumCoresAllocated(10), std::runtime_error);
    ASSERT_THROW(t1->setExecutionHost("host"), std::runtime_error);

    // getting the execution_host before a task has run at least once should return an empty string
    ASSERT_TRUE(t1->getExecutionHost().empty());
    ASSERT_TRUE(t1->getPhysicalExecutionHost().empty());

    ASSERT_NO_THROW(t1->setStartDate(1.0)); // need to set start date first before anything else
    ASSERT_DOUBLE_EQ(t1->getStartDate(), 1.0);

    ASSERT_NO_THROW(t1->setEndDate(1.0));
    ASSERT_DOUBLE_EQ(t1->getEndDate(), 1.0);

    ASSERT_NO_THROW(t1->setTerminationDate(1.0));
    ASSERT_DOUBLE_EQ(t1->getTerminationDate(), 1.0);

    ASSERT_NO_THROW(t1->setReadInputStartDate(1.0));
    ASSERT_DOUBLE_EQ(t1->getReadInputStartDate(), 1.0);

    ASSERT_NO_THROW(t1->setReadInputEndDate(1.0));
    ASSERT_DOUBLE_EQ(t1->getReadInputEndDate(), 1.0);

    ASSERT_NO_THROW(t1->setComputationStartDate(1.0));
    ASSERT_DOUBLE_EQ(t1->getComputationStartDate(), 1.0);

    ASSERT_NO_THROW(t1->setComputationEndDate(1.0));
    ASSERT_DOUBLE_EQ(t1->getComputationEndDate(), 1.0);

    ASSERT_NO_THROW(t1->setWriteOutputStartDate(1.0));
    ASSERT_DOUBLE_EQ(t1->getWriteOutputStartDate(), 1.0);

    ASSERT_NO_THROW(t1->setWriteOutputEndDate(1.0));
    ASSERT_DOUBLE_EQ(t1->getWriteOutputEndDate(), 1.0);

    ASSERT_NO_THROW(t1->setExecutionHost("hostname"));
    ASSERT_STREQ(t1->getExecutionHost().c_str(), "hostname");
    ASSERT_STREQ(t1->getPhysicalExecutionHost().c_str(), "hostname");

    ASSERT_NO_THROW(t1->setNumCoresAllocated(10));
    ASSERT_EQ(t1->getNumCoresAllocated(), 10);

    ASSERT_EQ(t1->getFailureCount(), 0);
    t1->incrementFailureCount();
    ASSERT_EQ(t1->getFailureCount(), 1);
    t1->getFailureDate();

//  ASSERT_EQ(t1->getTaskType(), wrench::WorkflowTask::TaskType::COMPUTE);

    ASSERT_EQ(t1->getBytesWritten(), -1);
    ASSERT_EQ(t4->getBytesRead(), 1000010);
    ASSERT_EQ(t4->getBytesWritten(), 1000);

    ASSERT_EQ(t1->getAverageCPU(), -1.0);
    ASSERT_EQ(t2->getAverageCPU(), 90.2);
    ASSERT_EQ(t4->getAverageCPU(), 50.5);
}

TEST_F(WorkflowTaskTest, InputOutputFile) {
    wrench::WorkflowFile *f1 = workflow->addFile("file-01", 10);
    wrench::WorkflowFile *f2 = workflow->addFile("file-02", 100);

    t1->addInputFile(f1);
    t1->addOutputFile(f2);

    ASSERT_THROW(t1->addInputFile(f1), std::invalid_argument);
    ASSERT_THROW(t1->addOutputFile(f2), std::invalid_argument);

    ASSERT_THROW(t1->addInputFile(f2), std::invalid_argument);
    ASSERT_THROW(t1->addOutputFile(f1), std::invalid_argument);

    ASSERT_THROW(workflow->removeFile(f1), std::invalid_argument);
    ASSERT_THROW(workflow->removeFile(f2), std::invalid_argument);

    wrench::WorkflowTask *t3 = workflow->addTask("task-03", 50, 2, 4, 0);
    t3->addInputFile(f2);

    ASSERT_EQ(t3->getNumberOfParents(), 1);
}

TEST_F(WorkflowTaskTest, StateToString) {

    ASSERT_EQ(wrench::WorkflowTask::stateToString(wrench::WorkflowTask::State::NOT_READY), "NOT READY");
    ASSERT_EQ(wrench::WorkflowTask::stateToString(wrench::WorkflowTask::State::READY), "READY");
    ASSERT_EQ(wrench::WorkflowTask::stateToString(wrench::WorkflowTask::State::PENDING), "PENDING");
    ASSERT_EQ(wrench::WorkflowTask::stateToString(wrench::WorkflowTask::State::COMPLETED), "COMPLETED");
    ASSERT_EQ(wrench::WorkflowTask::stateToString(wrench::WorkflowTask::State::UNKNOWN), "UNKNOWN");
    ASSERT_EQ(wrench::WorkflowTask::stateToString((wrench::WorkflowTask::State) 100), "INVALID");

    ASSERT_EQ(wrench::WorkflowTask::stateToString(wrench::WorkflowTask::InternalState::TASK_NOT_READY), "NOT READY");
    ASSERT_EQ(wrench::WorkflowTask::stateToString(wrench::WorkflowTask::InternalState::TASK_READY), "READY");
    ASSERT_EQ(wrench::WorkflowTask::stateToString(wrench::WorkflowTask::InternalState::TASK_RUNNING), "RUNNING");
    ASSERT_EQ(wrench::WorkflowTask::stateToString(wrench::WorkflowTask::InternalState::TASK_COMPLETED), "COMPLETED");
    ASSERT_EQ(wrench::WorkflowTask::stateToString(wrench::WorkflowTask::InternalState::TASK_FAILED), "FAILED");
    ASSERT_EQ(wrench::WorkflowTask::stateToString((wrench::WorkflowTask::InternalState) 100), "UNKNOWN STATE");
}

/**********************************************************************/
/**            WorkflowTaskExecutionHistoryTest                      **/
/**********************************************************************/

class WorkflowTaskExecutionHistoryTestWMS : public wrench::WMS {
public:
    WorkflowTaskExecutionHistoryTestWMS(WorkflowTaskTest *test,
                                        const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                        const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                        std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    WorkflowTaskTest *test;

    int main() {
        auto job_manager = this->createJobManager();

        auto job_that_will_fail = job_manager->createStandardJob(this->test->t4,
                                                                                 {{this->test->small_input_file,
                                                                                          wrench::FileLocation::LOCATION(this->test->storage_service)},
                                                                                  {this->test->large_input_file,
                                                                                          wrench::FileLocation::LOCATION(this->test->storage_service)},
                                                                                  {this->test->t4_output_file,
                                                                                          wrench::FileLocation::LOCATION(this->test->storage_service)}});
        job_manager->submitJob(job_that_will_fail, this->test->compute_service);

        // while large_input_file is being read, we delete small_input_file so that the one task job will fail
        wrench::StorageService::deleteFile(this->getWorkflow()->getFileByID("zz_small_input_file"),
                                           wrench::FileLocation::LOCATION(this->test->storage_service),
                                           this->test->file_registry_service);

        std::shared_ptr<wrench::WorkflowExecutionEvent> event;
        try {
            event = this->getWorkflow()->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }

        if (not std::dynamic_pointer_cast<wrench::StandardJobFailedEvent>(event)) {
            throw std::runtime_error("Job should have failed!");
        }

        auto job_that_will_complete = job_manager->createStandardJob(this->test->t4,
                                                                                     {{this->test->small_input_file,
                                                                                              wrench::FileLocation::LOCATION(this->test->backup_storage_service)},
                                                                                      {this->test->large_input_file,
                                                                                              wrench::FileLocation::LOCATION(this->test->storage_service)},
                                                                                      {this->test->t4_output_file,
                                                                                              wrench::FileLocation::LOCATION(this->test->storage_service)}});
        job_manager->submitJob(job_that_will_complete, this->test->compute_service);
        this->waitForAndProcessNextEvent();


        auto job_that_will_be_terminated = job_manager->createStandardJob(this->test->t5);
        job_manager->submitJob(job_that_will_be_terminated, this->test->compute_service);
        wrench::S4U_Simulation::sleep(10.0);
        job_manager->terminateJob(job_that_will_be_terminated);

        auto job_that_will_fail_2 = job_manager->createStandardJob(this->test->t6);
        job_manager->submitJob(job_that_will_fail_2, this->test->compute_service);
        wrench::S4U_Simulation::sleep(10.0);
        this->test->compute_service->stop();

        return 0;
    }
};

TEST_F(WorkflowTaskTest, WorkflowTaskExecutionHistoryTest) {
    DO_TEST_WITH_FORK(do_WorkflowTaskExecutionHistory_test);
}

void WorkflowTaskTest::do_WorkflowTaskExecutionHistory_test() {
    auto simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
//    argv[1] = strdup("--wrench-full-logs");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    std::string wms_host = "WMSHost";
    std::string execution_host = "ExecutionHost";

    ASSERT_NO_THROW(compute_service = simulation->add(new wrench::BareMetalComputeService(
            wms_host,
            {std::make_pair(
                    execution_host,
                    std::make_tuple(
                            wrench::ComputeService::ALL_CORES,
                            wrench::ComputeService::ALL_RAM))}, "",
            {})));

    ASSERT_NO_THROW(storage_service = simulation->add(new wrench::SimpleStorageService(wms_host, {"/"})));

    ASSERT_NO_THROW(
            backup_storage_service = simulation->add(new wrench::SimpleStorageService(wms_host, {"/backup"})));

    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(new WorkflowTaskExecutionHistoryTestWMS(
            this, {compute_service}, {storage_service, backup_storage_service}, wms_host
    )));

    ASSERT_NO_THROW(wms->addWorkflow(workflow.get()));

    file_registry_service = simulation->add(new wrench::FileRegistryService(wms_host));

    ASSERT_NO_THROW(simulation->stageFile(large_input_file, storage_service));
    ASSERT_NO_THROW(simulation->stageFile(small_input_file, storage_service));
    ASSERT_NO_THROW(simulation->stageFile(small_input_file, backup_storage_service));

    ASSERT_NO_THROW(simulation->launch());


    auto t4_history = t4->getExecutionHistory();

// t4 was executed twice, so its execution history should be of size 2
    ASSERT_EQ(t4_history.size(), 2);

// t4's second execution was successfull, so all values in its history for the second execution should be set
    wrench::WorkflowTask::WorkflowTaskExecution t4_successful_execution = t4_history.top();
    std::vector<double> t4_successful_execution_history_values = {
            t4_successful_execution.task_start,
            t4_successful_execution.read_input_start,
            t4_successful_execution.read_input_end,
            t4_successful_execution.computation_start,
            t4_successful_execution.computation_end,
            t4_successful_execution.write_output_start,
            t4_successful_execution.write_output_end,
            t4_successful_execution.task_end
    };

// none of the values should be -1 except task_failure
    for (auto &value : t4_successful_execution_history_values) {
        ASSERT_NE(value, -1.0);
    }

    ASSERT_DOUBLE_EQ(t4_successful_execution.task_failed, -1.0);

// the values should be ordered
    for (size_t i = 1; i < t4_successful_execution_history_values.size(); ++i) {
        ASSERT_GE(t4_successful_execution_history_values.at(i), t4_successful_execution_history_values.at(i - 1));
    }

// execution_host and num_cores_allocated should be set
    ASSERT_EQ(t4_successful_execution.num_cores_allocated, 3);
    ASSERT_STREQ(t4_successful_execution.execution_host.c_str(), "ExecutionHost");

    t4_history.pop();

// t4's first execution was unsuccessful, only task_start, read_input_start, execution_host, and num_cores_allocated should be set, everything else should be -1
    wrench::WorkflowTask::WorkflowTaskExecution t4_unsuccessful_execution = t4_history.top();
    ASSERT_NE(t4_unsuccessful_execution.task_start, -1.0);
    ASSERT_NE(t4_unsuccessful_execution.read_input_start, -1.0);
    ASSERT_NE(t4_unsuccessful_execution.task_failed, -1.0);
    ASSERT_EQ(t4_unsuccessful_execution.num_cores_allocated, 3);
    ASSERT_STREQ(t4_unsuccessful_execution.execution_host.c_str(), "ExecutionHost");

// the rest of the values should be set to -1 since the task failed while reading input
    ASSERT_DOUBLE_EQ(t4_unsuccessful_execution.read_input_end, -1.0);
    ASSERT_DOUBLE_EQ(t4_unsuccessful_execution.computation_start, -1.0);
    ASSERT_DOUBLE_EQ(t4_unsuccessful_execution.computation_end, -1.0);
    ASSERT_DOUBLE_EQ(t4_unsuccessful_execution.write_output_start, -1.0);
    ASSERT_DOUBLE_EQ(t4_unsuccessful_execution.write_output_end, -1.0);
    ASSERT_DOUBLE_EQ(t4_unsuccessful_execution.task_end, -1.0);

// t5 should have ran then been cleanly_terminated right after computation started
    auto t5_history = t5->getExecutionHistory();
    ASSERT_EQ(t5_history.size(), 1);

    auto t5_terminated_execution = t5_history.top();
    ASSERT_NE(t5_terminated_execution.task_start, -1.0);
    ASSERT_NE(t5_terminated_execution.read_input_start, -1.0);
    ASSERT_NE(t5_terminated_execution.read_input_end, -1.0);
    ASSERT_NE(t5_terminated_execution.computation_start, -1.0);
    ASSERT_NE(t5_terminated_execution.task_terminated, -1.0);

    ASSERT_EQ(t5_terminated_execution.computation_end, -1.0);
    ASSERT_EQ(t5_terminated_execution.write_output_start, -1.0);
    ASSERT_EQ(t5_terminated_execution.write_output_end, -1.0);
    ASSERT_EQ(t5_terminated_execution.task_failed, -1.0);
    ASSERT_EQ(t5_terminated_execution.task_end, -1.0);
    ASSERT_EQ(t5_terminated_execution.num_cores_allocated, 2);
    ASSERT_STREQ(t5_terminated_execution.execution_host.c_str(), "ExecutionHost");

// t6 should have ran then failed right after computation started because the compute service was stopped
    auto t6_history = t6->getExecutionHistory();
    ASSERT_EQ(t6_history.size(), 1);

    auto t6_failed_execution = t6_history.top();
    ASSERT_NE(t6_failed_execution.task_start, -1.0);
    ASSERT_NE(t6_failed_execution.read_input_start, -1.0);
    ASSERT_NE(t6_failed_execution.read_input_end, -1.0);
    ASSERT_NE(t6_failed_execution.computation_start, -1.0);
    ASSERT_NE(t6_failed_execution.task_failed, -1.0);

    ASSERT_EQ(t6_failed_execution.computation_end, -1.0);
    ASSERT_EQ(t6_failed_execution.write_output_start, -1.0);
    ASSERT_EQ(t6_failed_execution.write_output_end, -1.0);
    ASSERT_EQ(t6_failed_execution.task_terminated, -1.0);
    ASSERT_EQ(t6_failed_execution.task_end, -1.0);
    ASSERT_EQ(t6_failed_execution.num_cores_allocated, 3);
    ASSERT_STREQ(t6_failed_execution.execution_host.c_str(), "ExecutionHost");

    delete simulation;
    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
