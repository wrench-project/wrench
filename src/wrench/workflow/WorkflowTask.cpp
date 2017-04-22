/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <lemon/list_graph.h>
#include "WorkflowTask.h"
#include "Workflow.h"

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param string: the task id
     * @param flops: the task's number of flops
     * @param n: the number of processors for running the task
     */
    WorkflowTask::WorkflowTask(const std::string string, const double flops, const int n) {
      this->id = string;
      this->flops = flops;
      this->number_of_processors = n;
      this->state = WorkflowTask::READY;
      this->job = nullptr;
    }

    /**
     * @brief Add an input file to the task
     *
     * @param f: a pointer to the file
     */
    void WorkflowTask::addInputFile(WorkflowFile *f) {
      addFileToMap(input_files, f);

      f->setInputOf(this);

      // Perhaps add a control dependency?
      if (f->getOutputOf()) {
        workflow->addControlDependency(f->getOutputOf(), this);
      }
    }

    /**
     * @brief Add an output file to the task
     *
     * @param f: a pointer to a WorkflowFile object
     */
    void WorkflowTask::addOutputFile(WorkflowFile *f) {
      addFileToMap(output_files, f);
      f->setOutputOf(this);
      // Perhaps add control dependencies?
      for (auto const &x : f->getInputOf()) {
        workflow->addControlDependency(this, x.second);
      }

    }

    /**
     * @brief Get the id of the task
     *
     * @return the id as a string
     */
    std::string WorkflowTask::getId() const {
      return this->id;
    }

    /**
     * @brief Get the number of flops of the task
     *
     * @return the number of flops
     */
    double WorkflowTask::getFlops() const {
      return this->flops;
    }

    int WorkflowTask::getNumProcs() const {
      return this->number_of_processors;
    }

    /**
     * @brief Get the number of children of a task
     *
     * @return the number of children
     */
    int WorkflowTask::getNumberOfChildren() {
      int count = 0;
      for (ListDigraph::OutArcIt a(*DAG, DAG_node); a != INVALID; ++a) {
        ++count;
      }
      return count;
    }

    /**
     * @brief Get the number of parents of a task
     *
     * @return the number of parents
     */
    int WorkflowTask::getNumberOfParents() {
      int count = 0;
      for (ListDigraph::InArcIt a(*DAG, DAG_node); a != INVALID; ++a) {
        ++count;
      }
      return count;
    }


    /**
     * @brief Get the state of the task
     *
     * @return the task state
     */
    WorkflowTask::State WorkflowTask::getState() {
      return state;
    }

    /**
     * @brief Get the job that contains the task
     *
     * @return the containing job, or nullptr
     */
    WorkflowJob *WorkflowTask::getJob() {
      return this->job;
    }

    Workflow *WorkflowTask::getWorkflow() {
      return this->workflow;
    }

    /**
     * @brief Set the state of the task
     *
     * @param state: the task state
     */
    void WorkflowTask::setState(WorkflowTask::State state) {
      this->state = state;
    }

    /**
     * @brief Set the task's containing job
     *
     * @param job: the job
     */
    void WorkflowTask::setJob(WorkflowJob *job) {
      this->job = job;
    }

    /**
     * @brief Set the task's end date
     *
     * @param date: the end date
     */
    void WorkflowTask::setEndDate(double date) {
      this->end_date = date;
    }

    /**
     * @brief Set the task to the ready state
     */
    void WorkflowTask::setReady() {
      this->workflow->updateTaskState(this, WorkflowTask::READY);
    }

    /**
     * @brief Set the task to the running state
     */
    void WorkflowTask::setRunning() {
      this->workflow->updateTaskState(this, WorkflowTask::RUNNING);
    }

    /**
     * @brief Set the task to the completed state
     */
    void WorkflowTask::setCompleted() {
      this->workflow->updateTaskState(this, WorkflowTask::COMPLETED);
    }


    /**
     * @brief Helper method to add a file to a map if necessary
     *
     * @param map: the map of workflow files
     * @param f: a pointer to a WorkflowFile object
     */
    void WorkflowTask::addFileToMap(std::map<std::string, WorkflowFile *> map,
                                    WorkflowFile *f) {
      map[f->id] = f;
    }


};


