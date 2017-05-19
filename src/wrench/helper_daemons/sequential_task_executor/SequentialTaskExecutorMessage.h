/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SEQUENTIALTASKEXECUTORMESSAGE_H
#define WRENCH_SEQUENTIALTASKEXECUTORMESSAGE_H


#include <simulation/SimulationMessage.h>

namespace wrench {

    class WorkflowTask;
    class SequentialTaskExecutor;

    /***********************/
    /** \cond INTERNAL     */
    /***********************/


    class SequentialTaskExecutorMessage : public SimulationMessage {
    protected:
        SequentialTaskExecutorMessage(std::string name, double payload);

    };

    /**
     * @brief "RUN_TASK" SimulationMessage class
     */
    class SequentialTaskExecutorRunTaskMessage : public SequentialTaskExecutorMessage {
    public:
        SequentialTaskExecutorRunTaskMessage(WorkflowTask *, std::map<WorkflowFile *, StorageService *>,
                                             double payload);

        WorkflowTask *task;
        std::map<WorkflowFile *, StorageService *> file_locations;
    };

    /**
     * @brief "TASK_DONE" SimulationMessage class
     */
    class SequentialTaskExecutorTaskDoneMessage : public SequentialTaskExecutorMessage {
    public:
        SequentialTaskExecutorTaskDoneMessage(WorkflowTask *, SequentialTaskExecutor *, double payload);

        WorkflowTask *task;
        SequentialTaskExecutor *task_executor;
    };

    /**
    * @brief "TASK_FAILED" SimulationMessage class
    */
    class SequentialTaskExecutorTaskFailedMessage : public SequentialTaskExecutorMessage {
    public:
        SequentialTaskExecutorTaskFailedMessage(WorkflowTask *,
                                                SequentialTaskExecutor *,
                                                WorkflowExecutionFailureCause *,
                                                double payload);

        WorkflowTask *task;
        SequentialTaskExecutor *task_executor;
        WorkflowExecutionFailureCause *cause;
    };

    /***********************/
    /** \endcond           */
    /***********************/
};


#endif //WRENCH_SEQUENTIALTASKEXECUTORMESSAGE_H
