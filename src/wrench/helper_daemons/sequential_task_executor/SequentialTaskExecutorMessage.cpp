/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include "SequentialTaskExecutorMessage.h"

namespace wrench {

    SequentialTaskExecutorMessage::SequentialTaskExecutorMessage(std::string name, double payload) :
            SimulationMessage("SequentialTaskExecutor::" + name, payload) {

    }

    /**
     * @brief Constructor
     * @param task: pointer to a WorkflowTask
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    SequentialTaskExecutorRunTaskMessage::SequentialTaskExecutorRunTaskMessage(WorkflowTask *task,
                                   std::map<WorkflowFile *, StorageService *> file_locations,
                                   double payload) : SequentialTaskExecutorMessage("RUN_TASK", payload) {
      if (task == nullptr) {
        throw std::invalid_argument("Invalid constructor arguments");
      }
      this->task = task;
      this->file_locations = file_locations;
    }

    /**
     * @brief Constructor
     * @param task: pointer to a WorkflowTask
     * @param executor: pointer to a SequentialTaskExecutor
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    SequentialTaskExecutorTaskDoneMessage::SequentialTaskExecutorTaskDoneMessage(WorkflowTask *task, SequentialTaskExecutor *executor, double payload)
            : SequentialTaskExecutorMessage("TASK_DONE", payload) {
      if ((task == nullptr) || (executor == nullptr)) {
        throw std::invalid_argument("Invalid constructor arguments");
      }
      this->task = task;
      this->task_executor = executor;
    }

    /**
     * @brief Constructor
     * @param task: pointer to a WorkflowTask
     * @param executor: pointer to a SequentialTaskExecutor
     * @param cause: cause of the failure
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    SequentialTaskExecutorTaskFailedMessage::SequentialTaskExecutorTaskFailedMessage(WorkflowTask *task,
                                         SequentialTaskExecutor *executor,
                                         WorkflowExecutionFailureCause *cause,
                                         double payload)
            : SequentialTaskExecutorMessage("TASK_FAILED", payload) {
      if ((task == nullptr) || (executor == nullptr) || (cause == nullptr)) {
        throw std::invalid_argument("Invalid constructor arguments");
      }
      this->task = task;
      this->cause = cause;
      this->task_executor = executor;
    }


};