/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <map>
#include "wrench/logging/TerminalOutput.h"

#include "wrench/workflow/WorkflowFile.h"
#include "wrench/workflow/WorkflowTask.h"

WRENCH_LOG_CATEGORY(wrench_core_workflowFile, "Log category for WorkflowFile");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param string: the file id
     * @param s: the file size
     */
    WorkflowFile::WorkflowFile(const std::string name, double s) :
            id(name), size(s), output_of(nullptr) {
        this->input_of = {};
        this->output_of = nullptr;
    }

    /**
     * @brief Get the file size
     * @return a size in bytes
     */
    double WorkflowFile::getSize() {
        return this->size;
    }

    /**
     * @brief Get the file id
     * @return the id
     */
    std::string WorkflowFile::getID() {
        return this->id;
    }

    /**
     * @brief Define the task that outputs this file
     *
     * @param task: a task (or nullptr to state that the file is output of no task)
     */
    void WorkflowFile::setOutputOf(WorkflowTask *const task) {
        this->output_of = task;
    }

    /**
     * @brief Get the task that outputs this file
     *
     * @return a task
     */
    WorkflowTask *WorkflowFile::getOutputOf() {
        return this->output_of;
    }

    /**
     * @brief Add a task that uses this file as input
     *
     * @param task: a task
     */
    void WorkflowFile::setInputOf(WorkflowTask *task) {
        this->input_of[task->getID()] = task;
    }

    /**
     * @brief Get the list of tasks that use this file as input
     *
     * @return a map of tasks index by task ids
     */
    std::map<std::string, WorkflowTask *> WorkflowFile::getInputOf() {
        return this->input_of;
    }

    /**
     * @brief Get the workflow that this file is a part of
     * @return a workflow
     */
    Workflow *WorkflowFile::getWorkflow() {
        return this->workflow;
    }

    /**
     * @brief Returns true if the file is the output of some task, false otherwise
     * @return true or false
     */
    bool WorkflowFile::isOutput() {
        return (this->output_of != nullptr);
    }

}
