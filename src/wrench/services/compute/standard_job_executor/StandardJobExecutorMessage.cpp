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
     * @param worker_thread: the worker thread on which the work was performed
     * @param work: the work unit that was performed
     * @param payload: the message size in bytes
     */
    WorkunitExecutorDoneMessage::WorkunitExecutorDoneMessage(
            WorkunitMulticoreExecutor *workunit_executor,
            std::shared_ptr<Workunit> workunit,
            double payload) :
            StandardJobExecutorMessage("WORK_UNIT_EXECUTOR_DONE", payload) {
      this->workunit_executor = workunit_executor;
      this->workunit = workunit;

    }

    /**
     * @brief Constructor
     * @param worker_thread: the worker thread on which the work was performed
     * @param work: the work unit that was performed (and failed)
     * @param cause: the cause of the failure
     * @param payload: the message size in bytes
     */
    WorkunitExecutorFailedMessage::WorkunitExecutorFailedMessage(
            WorkunitMulticoreExecutor *workunit_executor,
            std::shared_ptr<Workunit> workunit,
            std::shared_ptr<FailureCause> cause,
            double payload):
            StandardJobExecutorMessage("WORK_UNIT_EXECUTOR_FAILED", payload) {
      this->workunit_executor = workunit_executor;
      this->workunit = workunit;
      this->cause = cause;
    }

    /**
     * @brief Constructor
     * @param job: The job that completed
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
};
