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

#include "wrench/services/compute/workunit_executor/WorkunitExecutor.h"
#include "wrench/services/compute/ComputeServiceMessage.h"

namespace wrench {

    class WorkflowTask;

    class StandardJobExecutor;

    class WorkunitMultiCoreExecutor;

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Top-level class for messages received/sent by a StandardJobExecutor
     */
    class StandardJobExecutorMessage : public SimulationMessage {
    protected:
        StandardJobExecutorMessage(std::string name, double payload);
    };

    /**
     * @brief A message sent by a WorkunitExecutor to notify that it has completed a WorkUnit
     */
    class WorkunitExecutorDoneMessage : public StandardJobExecutorMessage {
    public:
        WorkunitExecutorDoneMessage(
                std::shared_ptr<WorkunitExecutor> workunit_executor,
                std::shared_ptr<Workunit> workunit,
                double payload);

        /** @brief The work unit executor that has completed the work unit */
        std::shared_ptr<WorkunitExecutor> workunit_executor;
        /** @brief The work unit that has completed */
        std::shared_ptr<Workunit> workunit;
    };

    /**
     * @brief A message sent by a WorkunitExecutor to notify that its WorkUnit as failed
     */
    class WorkunitExecutorFailedMessage : public StandardJobExecutorMessage {
    public:
        WorkunitExecutorFailedMessage(
                std::shared_ptr<WorkunitExecutor> workunit_executor,
                std::shared_ptr<Workunit> workunit,
                std::shared_ptr<FailureCause> cause,
                double payload);

        /** @brief The worker unit executor that has failed to perform the work unit */
        std::shared_ptr<WorkunitExecutor> workunit_executor;
        /** @brief The work unit that has failed */
        std::shared_ptr<Workunit> workunit;
        /** @brief The cause of the failure */
        std::shared_ptr<FailureCause> cause;
    };


    /**
     * @brief A message sent by a StandardJobExecutor to notify that it has completed a StandardJob
     */
    class StandardJobExecutorDoneMessage : public StandardJobExecutorMessage {
    public:
        StandardJobExecutorDoneMessage(
                StandardJob *job,
                std::shared_ptr<StandardJobExecutor> executor,
                double payload);

        /** @brief The standard job executor that has completed the standard job */
        std::shared_ptr<StandardJobExecutor> executor;
        /** @brief The standard job that has completed */
        StandardJob *job;

    };

    /**
     * @brief A message sent by a StandardJobExecutor to notify that its StandardJob has failed
     */
    class StandardJobExecutorFailedMessage : public StandardJobExecutorMessage {
    public:
        StandardJobExecutorFailedMessage(
                StandardJob *job,
                std::shared_ptr<StandardJobExecutor> executor,
                std::shared_ptr<FailureCause> cause,
                double payload);

        /** @brief The standard job executor that has ailed to complete the standard job */
        std::shared_ptr<StandardJobExecutor> executor;
        /** @brief The standard job that has failed */
        StandardJob *job;
        /** @brief The cause of the failure */
        std::shared_ptr<FailureCause> cause;

    };


    /**
     * @brief A message sent by a ComputeThread once it's done performing its computation
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
