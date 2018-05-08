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

#include "wrench/logging/TerminalOutput.h"
#include "wrench/workflow/WorkflowTask.h"
#include "wrench/workflow/Workflow.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(workflowTask, "Log category for WorkflowTask");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param id: the task id
     * @param flops: the task's number of flops
     * @param min_cores: the minimum number of cores required for running the task
     * @param max_cores: the maximum number of cores that the task can use (infinity: ComputeService::ALL_CORES)
     * @param parallel_efficiency: the multi-core parallel efficiency
     * @param memory_requirement: memory requirement in bytes
     */
    WorkflowTask::WorkflowTask(const std::string id, const double flops, const unsigned long min_num_cores,
                               const unsigned long max_num_cores, const double parallel_efficiency,
                               const double memory_requirement) :
            id(id), flops(flops), min_num_cores(min_num_cores), max_num_cores(max_num_cores),
            parallel_efficiency(parallel_efficiency), memory_requirement(memory_requirement),
            visible_state(WorkflowTask::State::READY),
            internal_state(WorkflowTask::InternalState::TASK_READY), job(nullptr) {
    }

    /**
     * @brief Add an input file to the task
     *
     * @param file: the file
     */
    void WorkflowTask::addInputFile(WorkflowFile *file) {
      addFileToMap(input_files, output_files, file);

      file->setInputOf(this);

      WRENCH_DEBUG("Adding file '%s' as input to task %s",
                   file->getId().c_str(), this->getId().c_str());

      if (file->getOutputOf()) {
        workflow->addControlDependency(file->getOutputOf(), this);
      }
    }

    /**
     * @brief Add an output file to the task
     *
     * @param file: the file
     */
    void WorkflowTask::addOutputFile(WorkflowFile *file) {
      WRENCH_DEBUG("Adding file '%s' as output t task %s",
                   file->getId().c_str(), this->getId().c_str());

      addFileToMap(output_files, input_files, file);
      file->setOutputOf(this);

      for (auto const &x : file->getInputOf()) {
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

    /**
     * @brief Get the minimum number of cores required for running the task
     *
     * @return the number of cores
     */
    unsigned long WorkflowTask::getMinNumCores() const {
      return this->min_num_cores;
    }

    /**
     * @brief Get the maximum number of cores for the task can use
     *
     * @return the number of cores
     */
    unsigned long WorkflowTask::getMaxNumCores() const {
      return this->max_num_cores;
    }

    /**
     * @brief Get the parallel efficiency of the task
     *
     * @return the parallel efficiency (number between 0.0 and 1.0)
     */
    double WorkflowTask::getParallelEfficiency() const {
      return this->parallel_efficiency;
    }

    /**
     * @brief Get the memory requirement of the task
     *
     * @return the memory requirement (in bytes)
     */
    double WorkflowTask::getMemoryRequirement() const {
      return this->memory_requirement;
    }


    /**
     * @brief Get the number of children of a task
     *
     * @return the number of children
     */
    int WorkflowTask::getNumberOfChildren() const {
      int count = 0;
      for (lemon::ListDigraph::OutArcIt a(*DAG, DAG_node); a != lemon::INVALID; ++a) {
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
      for (lemon::ListDigraph::InArcIt a(*DAG, DAG_node); a != lemon::INVALID; ++a) {
        ++count;
      }
      return count;
    }

    /**
     * @brief Get the state of the task
     *
     * @return the task state
     */
    WorkflowTask::State WorkflowTask::getState() const {
      return this->visible_state;
    }


    /**
     * @brief Get the state of the task (as known to the "internal" layer)
     *
     * @return the task state
     */
    WorkflowTask::InternalState WorkflowTask::getInternalState() const {
      return this->internal_state;
    }

    /**
     * @brief Convert task state to a string (useful for output, debugging, logging, etc.)
     * @param state: task state
     * @return a string
     */
    std::string WorkflowTask::stateToString(WorkflowTask::State state) {
      switch (state) {
        case NOT_READY:
          return "NOT READY";
        case READY:
          return "READY";
        case PENDING:
          return "PENDING";
        case COMPLETED:
          return "COMPLETED";
        default:
          return "UNKNOWN STATE";
      }
    }

    /**
     * @brief Get a task state as a string
     *
     * @param state: the state
     *
     * @return the state as a string
     */
    std::string WorkflowTask::stateToString(WorkflowTask::InternalState state) {
      switch (state) {
        case TASK_NOT_READY:
          return "NOT READY";
        case TASK_READY:
          return "READY";
        case TASK_RUNNING:
          return "RUNNING";
        case TASK_COMPLETED:
          return "COMPLETED";
        case TASK_FAILED:
          return "FAILED";
        default:
          return "UNKNOWN STATE";
      }
    }

    /**
     * @brief Get the workflow that contains the task
     * @return a workflow
     */
    Workflow *WorkflowTask::getWorkflow() const {
      return this->workflow;
    }

    /**
     * @brief Set the internal state of the task
     *
     * @param state: the task state
     */
    void WorkflowTask::setInternalState(WorkflowTask::InternalState state) {
      this->internal_state = state;
    }

    /**
    * @brief Set the visible state of the task
    *
    * @param state: the task state
    */
    void WorkflowTask::setState(WorkflowTask::State state) {

      // Sanity check
      bool sane = true;
      switch(state) {
        case NOT_READY:
          if (this->internal_state != WorkflowTask::InternalState::TASK_NOT_READY) {
            sane = false;
          }
          break;
        case READY:
          if (this->internal_state != WorkflowTask::InternalState::TASK_READY) {
            sane = false;
          }
          break;
        case PENDING:
          if ((this->internal_state != WorkflowTask::InternalState::TASK_READY) and
              (this->internal_state != WorkflowTask::InternalState::TASK_NOT_READY)) {
            sane = false;
          }
          break;
        case COMPLETED:
          if (this->internal_state != WorkflowTask::InternalState::TASK_COMPLETED) {
            sane = false;
          }
          break;
      }

      if (not sane) {
        throw std::runtime_error("WorkflowTask::setState(): Cannot set " +
                                 this->getId() +"' visible state to " +
                                 std::to_string(state) + " when its internal " +
                                 "state is " + std::to_string(this->internal_state));
      }
      this->visible_state = state;
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
 * @brief Get the task's containing job
 *
 * @return job: the job
 */
    WorkflowJob *WorkflowTask::getJob() const {
      return this->job;
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
 * @brief Set the task's start date
 *
 * @param date: the end date
 */
    void WorkflowTask::setStartDate(double date) {
      this->start_date = date;
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
 * @brief Helper method to add a file to a map if necessary
 *
 * @param map_to_insert: the map of workflow files to insert
 * @param map_to_check: the map of workflow files to check
 * @param f: a workflow file
 *
 * @throw std::invalid_argument
 */
    void WorkflowTask::addFileToMap(std::map<std::string, WorkflowFile *> &map_to_insert,
                                    std::map<std::string, WorkflowFile *> &map_to_check,
                                    WorkflowFile *f) {

      if (map_to_check.find(f->id) != map_to_check.end()) {
        throw std::invalid_argument("WorkflowTask::addFileToMap(): File ID '" + f->id + "' is already used as input or output file");
      }

      if (map_to_insert.find(f->id) != map_to_insert.end()) {
        throw std::invalid_argument("WorkflowTask::addFileToMap(): File ID '" + f->id + "' already exists");
      }
      map_to_insert[f->id] = f;
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

/**
 * @brief Retrieves the set of input WorkflowFile objects for the task
 * @return a set workflow files
 */
    std::set<WorkflowFile *> WorkflowTask::getInputFiles() {
      std::set<WorkflowFile *> input;

      for (auto f: this->input_files) {
        input.insert(f.second);
      }
      return input;
    }

/**
 * @brief Retrieves the set of output WorkflowFile objects for the task
 * @return a set of workflow files
 */
    std::set<WorkflowFile *> WorkflowTask::getOutputFiles() {
      std::set<WorkflowFile *> output;

      for (auto f: this->output_files) {
        output.insert(f.second);
      }
      return output;
    }

/**
 * @brief Get the task's start date
 * @return the start date
 */
    double WorkflowTask::getStartDate() {
      return this->start_date;
    }

/**
 * @brief Get the task's end date
 * @return the start date
 */
    double WorkflowTask::getEndDate() {
      return this->end_date;
    }

/**
 * @brief Update the task's top level (looking only at the parents)
 */
    void WorkflowTask::updateTopLevel() {
      std::vector<WorkflowTask *> parents = this->workflow->getTaskParents(this);
      if (parents.empty()) {
        this->toplevel = 0;
      } else {
        unsigned long max_toplevel = 0;
        for (auto parent : parents) {
          max_toplevel = (max_toplevel < parent->toplevel ? parent->toplevel : max_toplevel);
        }
        this->toplevel = 1 + max_toplevel;
      }
      if (this->workflow->getNumLevels() < 1 + this->toplevel) {
        this->workflow->setNumLevels(1 + this->toplevel);
      }
      std::vector<WorkflowTask *> children = this->workflow->getTaskChildren(this);
      for (auto child : children) {
        child->updateTopLevel();
      }
    }

/**
 * @brief Returns the task's top level (max number of hops on a reverse path up to an entry task. Entry
 *        tasks have a top-leve of 0)
 * @return
 */
    unsigned long WorkflowTask::getTopLevel() {
      return this->toplevel;
    }
};
