/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <map>
#include <xbt.h>

#include "WorkflowFile.h"
#include "WorkflowTask.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(workflowFile, "Log category for WorkflowFile");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param string: the file name/id
     * @param s: the file size
     */
    WorkflowFile::WorkflowFile(const std::string name, double s) :
            id(name), size(s), output_of(nullptr) {
      this->input_of = {};
    };

    /**
     * @brief Get the file size
     * @return  the file size in bytes
     */
    double WorkflowFile::getSize() {
      return this->size;
    }

    /**
     * @brief Get the file id
     * @return
     */
    std::string WorkflowFile::getId() {
      return this->id;
    }

    /**
     * @brief Define the task that outputs this file
     *
     * @param task: a pointer to a WorkflowTask object
     */
    void WorkflowFile::setOutputOf(WorkflowTask *task) {
      this->output_of = task;
    }

    /**
     * @brief Get the task the outputs this file
     *
     * @return a pointer to a WorkflowTask
     */
    WorkflowTask *WorkflowFile::getOutputOf() {
      return this->output_of;
    }

    /**
     * @brief Add a task that uses this file as input
     *
     * @param task: a pointer to a WorkflowTask object
     */
    void WorkflowFile::setInputOf(WorkflowTask *task) {
      this->input_of[task->getId()] = task;
    }

    /**
     * @brief Get the set of tasks that use this file as input
     *
     * @return a map of pointers to WorkflowTask objects
     */
    std::map<std::string, WorkflowTask *> WorkflowFile::getInputOf() {
      return this->input_of;
    }

    /**
     * @brief Retrieve the file's workflow
     * @return the woriflow
     */
    Workflow *WorkflowFile::getWorkflow() {
      return this->workflow;
    }


    /**
     * @brief Returns true if the file is the output of some task, false otherwise
     * @return true or false
     */
    bool WorkflowFile::isOuput() {
      return (this->output_of != nullptr);
    };

};
