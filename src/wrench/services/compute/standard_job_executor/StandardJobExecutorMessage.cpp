/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include "StandardJobExecutorMessage.h"

namespace wrench {


    /**
    * @brief Constructor
    *
    * @param name: the message name
    * @param payload: the message size in bytes
    */
    StandardJobExecutorMessage::StandardJobExecutorMessage(std::string name, double payload) :
            SimulationMessage("StandardJobExecutorMessage::" + name, payload) {
    }


    /**
     * @brief Constructor
     * @param workunit_executor: the work unit executor  on which the work unit was performed
     * @param workunit: the work unit that was performed
     * @param files_in_scratch: the set of files stored in scratch space of the compute service while executing this workunit
     * @param payload: the message size in bytes
     */
    WorkunitExecutorDoneMessage::WorkunitExecutorDoneMessage(
            WorkunitMulticoreExecutor *workunit_executor,
            Workunit *workunit,
            std::set<WorkflowFile*> files_in_scratch,
            double payload) :
            StandardJobExecutorMessage("WORK_UNIT_EXECUTOR_DONE", payload) {
      this->workunit_executor = workunit_executor;
      this->workunit = workunit;
      this->files_in_scratch = files_in_scratch;

    }

    /**
     * @brief Constructor
     * @param workunit_executor: the work unit executor on which the work was performed
     * @param workunit: the work unit that was performed (and failed)
     * @param files_in_scratch: the set of files stored in scratch space of the compute service while executing this workunit
     * @param cause: the cause of the failure
     * @param payload: the message size in bytes
     */
    WorkunitExecutorFailedMessage::WorkunitExecutorFailedMessage(
            WorkunitMulticoreExecutor *workunit_executor,
            Workunit *workunit,
            std::set<WorkflowFile*> files_in_scratch,
            std::shared_ptr<FailureCause> cause,
            double payload):
            StandardJobExecutorMessage("WORK_UNIT_EXECUTOR_FAILED", payload) {
      this->workunit_executor = workunit_executor;
      this->workunit = workunit;
      this->files_in_scratch = files_in_scratch;
      this->cause = cause;
    }

    /**
     * @brief Constructor
     * @param job: the job that completed
     * @param executor: The executor that completed it
     * @param payload: the message size in bytes
     */
    StandardJobExecutorDoneMessage::StandardJobExecutorDoneMessage(
            StandardJob *job,
            StandardJobExecutor *executor,
            double payload) :
            StandardJobExecutorMessage("STANDARD_JOB_COMPLETED", payload) {
      this->job = job;
      this->executor = executor;
    }

    /**
     * @brief Constructor
     * @param job: the job that failed
     * @param executor: the executor on which the job failed
     * @param cause: the cause of the failure
     * @param payload: the message size in bytes
     */
    StandardJobExecutorFailedMessage::StandardJobExecutorFailedMessage(
            StandardJob *job,
            StandardJobExecutor *executor,
            std::shared_ptr<FailureCause> cause,
            double payload) :
            StandardJobExecutorMessage("STANDARD_JOB_FAILED", payload) {
      this->job = job;
      this->executor = executor;
      this->cause = cause;

    }

    /**
     * @brief Constructor
     * @param scratch_files: the files stored in scratch space because of the execution of a standardjob
     * @param executor: the executor on which the job failed
     * @param cause: the cause of the failure
     * @param payload: the message size in bytes
     */
    FilesInScratchMessageByStandardJobExecutor::FilesInScratchMessageByStandardJobExecutor(
            std::set<WorkflowFile*> scratch_files,
            double payload) :
            StandardJobExecutorMessage("FILES_IN_SCRATCH", payload) {
      this->scratch_files = scratch_files;

    }

    /**
     * @brief Constructor
     */
    ComputeThreadDoneMessage::ComputeThreadDoneMessage() :
            StandardJobExecutorMessage("COMPUTE_THREAD_DONE", 0) {
    }




};
