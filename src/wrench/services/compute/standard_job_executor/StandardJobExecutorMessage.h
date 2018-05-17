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
#include "wrench/services/compute/ComputeServiceMessage.h"

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
                Workunit *workunit,
                std::set<WorkflowFile*> files_in_scratch,
                double payload);

        /** @brief The worker thread that performed the work */
        WorkunitMulticoreExecutor *workunit_executor;
        /** @brief The work that was performed */
        Workunit *workunit;
        /** @brief The set of files stored in scratch space by this work unit executor */
        std::set<WorkflowFile*> files_in_scratch;
    };

    /**
     * @brief WorkunitExecutorFailedMessage class
     */
    class WorkunitExecutorFailedMessage : public StandardJobExecutorMessage {
    public:
        WorkunitExecutorFailedMessage(
                WorkunitMulticoreExecutor *workunit_executor,
                Workunit *workunit,
                std::set<WorkflowFile*> files_in_scratch,
                std::shared_ptr<FailureCause> cause,
                double payload);

        /** @brief The worker thread that failed to perform the work */
        WorkunitMulticoreExecutor *workunit_executor;
        /** @brief The work that failed */
        Workunit *workunit;
        /** @brief The cause of the failure */
        std::shared_ptr<FailureCause> cause;
        /** @brief The set of files stored in scratch space by this work unit executor */
        std::set<WorkflowFile*> files_in_scratch;
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

    /**
     * @brief FilesInScratchMessageByStandardJobExecutor class
     */
    class FilesInScratchMessageByStandardJobExecutor : public StandardJobExecutorMessage {
    public:
        FilesInScratchMessageByStandardJobExecutor(
                std::set<WorkflowFile*> scratch_files,
                double payload);
        /** @brief The set of files stored in scratch because of the execution of a standardjob */
        std::set<WorkflowFile*> scratch_files;
    };

    /**
     * @brief ComputeThreadDoneMessage class
     */
    class ComputeThreadDoneMessage : public StandardJobExecutorMessage {
    public:
        ComputeThreadDoneMessage();

    };


    /***********************/
    /** \endcond           */
    /***********************/
};

#endif //WRENCH_MULTINODEMULTICORESTANDARDJOBEXECUTORMESSAGE_H
