/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_MULTINODEMULTICORESTANDARDJOBEXECUTORMESSAGE_H
#define WRENCH_MULTINODEMULTICORESTANDARDJOBEXECUTORMESSAGE_H

#include <vector>

#include "wrench/services/compute/standard_job_executor/WorkunitMulticoreExecutor.h"
#include "services/compute/ComputeServiceMessage.h"

namespace wrench {

    class WorkflowTask;

    class StandardJobExecutor;

    class WorkunitMultiCoreExecutor;

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief StandardJobExecutorMessage top-level class
     */
    class StandardJobExecutorMessage : public SimulationMessage {
    protected:
        StandardJobExecutorMessage(std::string name, double payload);
    };

    /**
     * @brief WorkunitExecutorDoneMessage class
     */
    class WorkunitExecutorDoneMessage : public StandardJobExecutorMessage {
    public:
        WorkunitExecutorDoneMessage(
                WorkunitMulticoreExecutor *workunit_executor,
                std::shared_ptr<Workunit> workunit,
                double payload);

        /** @brief The worker thread that performed the work */
        WorkunitMulticoreExecutor *workunit_executor;
        /** @brief The work that was performed */
        std::shared_ptr<Workunit> workunit;

    };

    /**
     * @brief WorkunitExecutorFailedMessage class
     */
    class WorkunitExecutorFailedMessage : public StandardJobExecutorMessage {
    public:
        WorkunitExecutorFailedMessage(
                WorkunitMulticoreExecutor *workunit_executor,
                std::shared_ptr<Workunit> workunit,
                std::shared_ptr<FailureCause> cause,
                double payload);

        /** @brief The worker thread that failed to perform the work */
        WorkunitMulticoreExecutor *workunit_executor;
        /** @brief The work that failed */
        std::shared_ptr<Workunit> workunit;
        /** @brief The cause of the failure */
        std::shared_ptr<FailureCause> cause;
    };


    /**
     * @brief StandardJobExecutorDoneMessage class
     */
    class StandardJobExecutorDoneMessage : public StandardJobExecutorMessage {
    public:
        StandardJobExecutorDoneMessage(
                StandardJob *job,
                StandardJobExecutor *executor,
                double payload);

        /** @brief The executor that completed the work */
        StandardJobExecutor *executor;
        /** @brief The job that was completed */
        StandardJob *job;

    };

    /**
     * @brief StandardJobExecutorFailedMessage class
     */
    class StandardJobExecutorFailedMessage : public StandardJobExecutorMessage {
    public:
        StandardJobExecutorFailedMessage(
                StandardJob *job,
                StandardJobExecutor *executor,
                std::shared_ptr<FailureCause> cause,
                double payload);

        /** @brief The executor that failed to complete the work */
        StandardJobExecutor *executor;
        /** @brief The job that failed */
        StandardJob *job;
        /** @brief The cause of the failure */
        std::shared_ptr<FailureCause> cause;

    };


    /***********************/
    /** \endcond           */
    /***********************/
};

#endif //WRENCH_MULTINODEMULTICORESTANDARDJOBEXECUTORMESSAGE_H
