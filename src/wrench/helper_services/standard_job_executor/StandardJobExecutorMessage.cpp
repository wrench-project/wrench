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
     * @param workunit_executor: the work unit executor  on which the work unit has completed
     * @param workunit: the work unit that has completed
     * @param payload: the message size in bytes
     */
    WorkunitExecutorDoneMessage::WorkunitExecutorDoneMessage(
            std::shared_ptr<WorkunitExecutor> workunit_executor,
            std::shared_ptr<Workunit> workunit,
            double payload) :
            StandardJobExecutorMessage("WORK_UNIT_EXECUTOR_DONE", payload) {
        this->workunit_executor = workunit_executor;
        this->workunit = workunit;
    }

    /**
     * @brief Constructor
     * @param workunit_executor: the work unit executor on which the work has failed
     * @param workunit: the work unit that has failed
     * @param cause: the cause of the failure
     * @param payload: the message size in bytes
     */
    WorkunitExecutorFailedMessage::WorkunitExecutorFailedMessage(
            std::shared_ptr<WorkunitExecutor> workunit_executor,
            std::shared_ptr<Workunit> workunit,
            std::shared_ptr<FailureCause> cause,
            double payload) :
            StandardJobExecutorMessage("WORK_UNIT_EXECUTOR_FAILED", payload) {
        this->workunit_executor = workunit_executor;
        this->workunit = workunit;
        this->cause = cause;
    }

    /**
     * @brief Constructor
     * @param job: the job that has completed
     * @param executor: The executor that has completed it
     * @param payload: the message size in bytes
     */
    StandardJobExecutorDoneMessage::StandardJobExecutorDoneMessage(
            StandardJob *job,
            std::shared_ptr<StandardJobExecutor> executor,
            double payload) :
            StandardJobExecutorMessage("STANDARD_JOB_COMPLETED", payload) {
        this->job = job;
        this->executor = executor;
    }

    /**
     * @brief Constructor
     * @param job: the job that has failed
     * @param executor: the executor on which the job has failed
     * @param cause: the cause of the failure
     * @param payload: the message size in bytes
     */
    StandardJobExecutorFailedMessage::StandardJobExecutorFailedMessage(
            StandardJob *job,
            std::shared_ptr<StandardJobExecutor> executor,
            std::shared_ptr<FailureCause> cause,
            double payload) :
            StandardJobExecutorMessage("STANDARD_JOB_FAILED", payload) {
        this->job = job;
        this->executor = executor;
        this->cause = cause;

    }


    /**
     * @brief Constructor
     */
    ComputeThreadDoneMessage::ComputeThreadDoneMessage() :
            StandardJobExecutorMessage("COMPUTE_THREAD_DONE", 0) {
    }


};
