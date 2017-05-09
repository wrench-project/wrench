/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <lemon/list_graph.h>
#include <xbt.h>
#include <logging/TerminalOutput.h>
#include "WorkflowTask.h"
#include "Workflow.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(workflowTask, "Log category for WorkflowTask");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param id: the task id
     * @param flops: the task's number of flops
     * @param n: the number of processors for running the task
     */
    WorkflowTask::WorkflowTask(const std::string id, const double flops, const int n) :
            id(id), flops(flops), number_of_processors(n), state(WorkflowTask::READY), job(nullptr) {
    }

    /**
     * @brief Add an input file to the task
     *
     * @param f: a pointer to the file
     */
    void WorkflowTask::addInputFile(WorkflowFile *f) {
      addFileToMap(input_files, f);

      f->setInputOf(this);

      WRENCH_DEBUG("Adding file '%s' as input to task %s",
                   f->getId().c_str(), this->getId().c_str());
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
      WRENCH_DEBUG("Adding file '%s' as output t task %s",
                   f->getId().c_str(), this->getId().c_str());

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
    int WorkflowTask::getNumberOfChildren() const {
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
    int WorkflowTask::getNumberOfParents() const {
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
      return this->state;
    }

    /**
     * @brief Get the state of the task as a string
     *
     * @return the task state as a string
     */
    std::string WorkflowTask::stateToString(WorkflowTask::State state) {
      switch (state) {
        case NOT_READY:
          return "NOT READY";
        case READY:
          return "READY";
        case PENDING:
          return "PENDING";
        case RUNNING:
          return "RUNNING";
        case COMPLETED:
          return "COMPLETED";
        case FAILED:
          return "FAILED";
        default:
          return "UNKNOWN STATE";
      }
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
     * @brief Get the cluster Id for the task
     *
     * @return the cluster id, or an empty string
     */
    std::string WorkflowTask::getClusterId() const {
      return this->cluster_id;
    }

    /**
     * Set the cluster id for the task
     *
     * @param id: cluster id the task belongs to
     */
    void WorkflowTask::setClusterId(std::string id) {
      this->cluster_id = id;
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

    /**
     * @brief Retrieve the number of times a task has failed
     * @return the failure count
     */
    unsigned int WorkflowTask::getFailureCount() {
      return this->failure_count;
    }

    /**
     * @brief Increment the failure count of a task
     */
    void WorkflowTask::incrementFailureCount() {
      this->failure_count++;
    }
};


