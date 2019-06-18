/**
 * Copyright (c) 2017-2019. The WRENCH Team.
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
#include "wrench/services/compute/cloud/CloudComputeService.h"
#include "wrench/services/compute/virtualized_cluster/VirtualizedClusterComputeService.h"
#include "wrench/simulation/Simulation.h"
#include "wrench/simulation/SimulationTimestampTypes.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(workflow_task, "Log category for WorkflowTask");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param id: the task id
     * @param flops: the task's number of flops
     * @param min_cores: the minimum number of cores required for running the task
     * @param max_cores: the maximum number of cores that the task can use (infinity: ULONG_MAX)
     * @param parallel_efficiency: the multi-core parallel efficiency
     * @param memory_requirement: memory requirement in bytes
     * @param type: the type of the task (WorkflowTask::TaskType)
     */
    WorkflowTask::WorkflowTask(const std::string id, const double flops, const unsigned long min_num_cores,
                               const unsigned long max_num_cores, const double parallel_efficiency,
                               const double memory_requirement, const TaskType type) :
            id(id), task_type(type), flops(flops),
            min_num_cores(min_num_cores),
            max_num_cores(max_num_cores),
            parallel_efficiency(parallel_efficiency),
            memory_requirement(memory_requirement),
            execution_host(""),
            visible_state(WorkflowTask::State::READY),
            upcoming_visible_state(WorkflowTask::State::UNKNOWN),
            internal_state(WorkflowTask::InternalState::TASK_READY),
            job(nullptr) {
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
                     file->getID().c_str(), this->getID().c_str());

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
                     file->getID().c_str(), this->getID().c_str());

//        if (file->getOutputOf() != nullptr) {
//            std::cerr << "Trying to set file '" + file->getID() + "' as output of task '" + this->getID() +
//                                        "', but this file is already the output of task '" + file->getOutputOf()->getID()+ "'\n";
//            throw std::invalid_argument("Trying to set file '" + file->getID() + "' as output of task '" + this->getID() +
//            "', but this file is already the output of task '" + file->getOutputOf()->getID()+ "'");
//
//        }

        addFileToMap(output_files, input_files, file);
        file->setOutputOf(this);

        for (auto const &x : file->getInputOf()) {
            workflow->addControlDependency(this, x.second);
        }

    }

    /**
     * @brief Get the id of the task
     *
     * @return an id as a string
     */
    std::string WorkflowTask::getID() const {
        return this->id;
    }

    /**
     * @brief Get the number of flops of the task
     *
     * @return a number of flops
     */
    double WorkflowTask::getFlops() const {
        return this->flops;
    }

    /**
     * @brief Get the minimum number of cores required for running the task
     *
     * @return a number of cores
     */
    unsigned long WorkflowTask::getMinNumCores() const {
        return this->min_num_cores;
    }

    /**
     * @brief Get the maximum number of cores that the task can use
     *
     * @return a number of cores
     */
    unsigned long WorkflowTask::getMaxNumCores() const {
        return this->max_num_cores;
    }

    /**
     * @brief Get the parallel efficiency of the task
     *
     * @return a parallel efficiency (number between 0.0 and 1.0)
     */
    double WorkflowTask::getParallelEfficiency() const {
        return this->parallel_efficiency;
    }

    /**
     * @brief Get the memory requirement of the task
     *
     * @return a memory requirement (in bytes)
     */
    double WorkflowTask::getMemoryRequirement() const {
        return this->memory_requirement;
    }


    /**
     * @brief Get the number of children of a task
     *
     * @return a number of children
     */
    unsigned long WorkflowTask::getNumberOfChildren() const {
        unsigned long count = 0;
        for (lemon::ListDigraph::OutArcIt a(*DAG, DAG_node); a != lemon::INVALID; ++a) {
            ++count;
        }
        return count;
    }

    /**
     * @brief Get the children of a task
     *
     * @return a list of workflow tasks
     */
    std::vector<WorkflowTask *> WorkflowTask::getChildren() const {
        return this->getWorkflow()->getTaskChildren(this);
    }

    /**
     * @brief Get the number of parents of a task
     *
     * @return a number of parents
     */
    unsigned long WorkflowTask::getNumberOfParents() const {
        unsigned long count = 0;
        for (lemon::ListDigraph::InArcIt a(*DAG, DAG_node); a != lemon::INVALID; ++a) {
            ++count;
        }
        return count;
    }

    /**
     * @brief Get the parents of a task
     *
     * @return a list of workflow tasks
     */
    std::vector<WorkflowTask *> WorkflowTask::getParents() const {
        return this->getWorkflow()->getTaskParents(this);
    }

    /**
     * @brief Get the state of the task
     *
     * @return a task state
     */
    WorkflowTask::State WorkflowTask::getState() const {
        return this->visible_state;
    }

    /**
    * @brief Get the state of the task
    *
    * @return a task state
    */
    WorkflowTask::State WorkflowTask::getUpcomingState() const {
        return this->upcoming_visible_state;
    }


    /**
     * @brief Get the state of the task (as known to the "internal" layer)
     *
     * @return a task state
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
            case UNKNOWN:
                return "UNKNOWN";
            default:
                return "INVALID";
        }
    }

    /**
     * @brief Get a task internal state as a string
     *
     * @param state: the internal state
     *
     * @return an internal state as a string
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
     * @param state: the task's internal state
     */
    void WorkflowTask::setInternalState(WorkflowTask::InternalState state) {
//      WRENCH_INFO("SETTING %s's INTERNAL STATE TO %s", this->getID().c_str(), WorkflowTask::stateToString(state).c_str());
        this->internal_state = state;
    }

    /**
     * @brief Set the visible state of the task
     *
     * @param state: the task state
     */
    void WorkflowTask::setState(WorkflowTask::State state) {

//      WRENCH_INFO("WorkflowTask::setState(): SETTING %s's VISIBLE STATE TO %s", this->getID().c_str(), WorkflowTask::stateToString(state).c_str());

        // Sanity check
        bool sane = true;
        switch (state) {
            case NOT_READY:
                if ((this->internal_state != WorkflowTask::InternalState::TASK_NOT_READY) and
                    (this->internal_state != WorkflowTask::InternalState::TASK_FAILED)) {
                    sane = false;
                }
                break;
            case READY:
                if ((this->internal_state != WorkflowTask::InternalState::TASK_READY) and
                    (this->internal_state != WorkflowTask::InternalState::TASK_FAILED)) {
                    sane = false;
                }
                break;
            case PENDING:
                if ((this->internal_state != WorkflowTask::InternalState::TASK_READY) and
                    (this->internal_state != WorkflowTask::InternalState::TASK_NOT_READY) and
                    (this->internal_state != WorkflowTask::InternalState::TASK_RUNNING)) {
                    sane = false;
                }
                break;
            case COMPLETED:
                if (this->internal_state != WorkflowTask::InternalState::TASK_COMPLETED) {
                    sane = false;
                }
                break;
            case UNKNOWN:
                sane = false;
                break;
        }

        if (not sane) {
            throw std::runtime_error("WorkflowTask::setState(): Cannot set " +
                                     this->getID() + "'s visible state to " +
                                     stateToString(state) + " when its internal " +
                                     "state is " + stateToString(this->internal_state));
        }
        this->visible_state = state;
    }

    /**
     * @brief Set the upcoming visible state of the task
     *
     * @param state: the task state
     */
    void WorkflowTask::setUpcomingState(WorkflowTask::State state) {
        this->upcoming_visible_state = state;
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
     * @return job: the job
     */
    WorkflowJob *WorkflowTask::getJob() const {
        return this->job;
    }

    /**
     * @brief Get the cluster Id for the task
     * @return a cluster id, or an empty string
     */
    std::string WorkflowTask::getClusterID() const {
        return this->cluster_id;
    }

    /**
     * @brief Set the cluster id for the task
     *
     * @param id: cluster id the task belongs to
     */
    void WorkflowTask::setClusterID(std::string id) {
        this->cluster_id = id;
    }

    /**
     * @brief Get the workflow task type
     * @return a workflow task type
     */
    WorkflowTask::TaskType WorkflowTask::getTaskType() const {
        return this->task_type;
    }

    /**
     * @brief Set the task type
     * @param task_type: task type
     */
    void WorkflowTask::setTaskType(wrench::WorkflowTask::TaskType task_type) {
        this->task_type = task_type;
    }

    /**
     * @brief Get the task priority. By default, priority is 0.
     * @return the task priority
     */
    long WorkflowTask::getPriority() const {
        return this->priority;
    }

    /**
     * @brief Set the task priority
     * @param priority: task priority
     */
    void WorkflowTask::setPriority(long priority) {
        this->priority = priority;
    }

    /**
     * @brief Get the task average CPU usage
     * @return the task average CPU usage
     */
    double WorkflowTask::getAverageCPU() const {
        return this->average_cpu;
    }

    /**
     * @brief Set the task average CPU usage
     * @param average_cpu: task average CPU usage
     */
    void WorkflowTask::setAverageCPU(double average_cpu) {
        this->average_cpu = average_cpu;
    }

    /**
     * @brief Get the number of bytes read by the task
     * @return number of bytes read by the task in KB
     */
    unsigned long WorkflowTask::getBytesRead() const {
        return this->bytes_read;
    }

    /**
     * @brief Set the number of bytes read by the task
     * @param bytes_read: number of bytes read by the task in KB
     */
    void WorkflowTask::setBytesRead(unsigned long bytes_read) {
        this->bytes_read = bytes_read;
    }

    /**
     * @brief Get the number of bytes written by the task
     * @return number of bytes written by the task in KB
     */
    unsigned long WorkflowTask::getBytesWritten() const {
        return this->bytes_written;
    }

    /**
     * @brief Set the number of bytes written by the task
     * @param bytes_written: number of bytes written by the task in KB
     */
    void WorkflowTask::setBytesWritten(unsigned long bytes_written) {
        this->bytes_written = bytes_written;
    }

    /**
     * @brief Set the task's start date.
     *
     * @param date: the start date
     */
    void WorkflowTask::setStartDate(double date) {
        this->execution_history.push(WorkflowTask::WorkflowTaskExecution(date));
    }

    /**
     * @brief Set the task's end date
     *
     * @param date: the end date
     * @throws std::runtime_error
     */
    void WorkflowTask::setEndDate(double date) {
        if (not this->execution_history.empty()) {
            this->execution_history.top().task_end = date;
        } else {
            throw std::runtime_error("WorkflowTask::setEndDate() cannot be called before WorkflowTask::setStartDate()");
        }
    }

    /**
     * @brief Set the date when the computation portion of a WorkflowTask has begun
     *
     * @param date: the date when the computation portion of the WorkflowTask has begun
     * @throws std::runtime_error
     */
    void WorkflowTask::setComputationStartDate(double date) {
        if (not this->execution_history.empty()) {
            this->execution_history.top().computation_start = date;
        } else {
            throw std::runtime_error(
                    "WorkflowTask::setComputationStartDate() cannot be called before WorkflowTask::setStartDate()");
        }
    }

    /**
     * @brief Set the date when the computation portion of a WorkflowTask has ended
     *
     * @param date: the date when the computation portion of the WorkflowTask has ended
     * @throws std::runtime_error
     */
    void WorkflowTask::setComputationEndDate(double date) {
        if (not this->execution_history.empty()) {
            this->execution_history.top().computation_end = date;
        } else {
            throw std::runtime_error(
                    "WorkflowTask::setComputationEndDate() cannot be called before WorkflowTask::setStartDate()");
        }
    }

    /**
     * @brief Set the date when the read input portion of a WorkflowTask has begun
     *
     * @param date: the date when the read input portion of a WorkflowTask has begun
     * @throws std::runtime_error
     */
    void WorkflowTask::setReadInputStartDate(double date) {
        if (not this->execution_history.empty()) {
            this->execution_history.top().read_input_start = date;
        } else {
            throw std::runtime_error(
                    "WorkflowTask::setReadInputStartDate() cannot be called before WorkflowTask::setStartDate()");
        }
    }

    /**
     * @brief Set the date when the read input portion of a WorkflowTask has completed
     *
     * @param date: the date when the read input portion of a WorkflowTask has completed
     * @throws std::runtime_error
     */
    void WorkflowTask::setReadInputEndDate(double date) {
        if (not this->execution_history.empty()) {
            this->execution_history.top().read_input_end = date;
        } else {
            throw std::runtime_error(
                    "WorkflowTask::setReadInputEndDate() cannot be called before WorkflowTask::setStartDate()");
        }
    }

    /**
     * @brief Set the date when the write output portion of a WorkflowTask has begun
     *
     * @param date: the date when the write output portion of a task has begun
     * @throws std::runtime_error
     */
    void WorkflowTask::setWriteOutputStartDate(double date) {
        if (not this->execution_history.empty()) {
            this->execution_history.top().write_output_start = date;
        } else {
            throw std::runtime_error(
                    "WorkflowTask::setWriteOutputStartDate() cannot be called before WorkflowTask::setStartDate()");
        }
    }

    /**
     * @brief Set the date when the write output portion of a WorkflowTask has completed
     *
     * @param date: the date when the write output portion of a task has completed
     * @throws std::runtime_error
     */
    void WorkflowTask::setWriteOutputEndDate(double date) {
        if (not this->execution_history.empty()) {
            this->execution_history.top().write_output_end = date;
        } else {
            throw std::runtime_error(
                    "WorkflowTask::setWriteOutputEndDate() cannot be called before WorkflowTask::setStartDate()");
        }
    }

    /**
     * @brief Set the date when the task has failed
     *
     * @param date: the date when the task has failed
     */
    void WorkflowTask::setFailureDate(double date) {
        if (not this->execution_history.empty()) {
            this->execution_history.top().task_failed = date;
        } else {
            throw std::runtime_error(
                    "WorkflowTask::setFailureDate() cannot be called before WorkflowTask::setStartDate()");
        }
    }

    /**
     * @brief Set the date when the task was terminated
     *
     * @param date: the date when the task was terminated
     */
    void WorkflowTask::setTerminationDate(double date) {
        if (not this->execution_history.empty()) {
            this->execution_history.top().task_terminated = date;
        } else {
            throw std::runtime_error(
                    "WorkflowTask::setTerminationDate() cannot be called before WorkflowTask::setStartDate()");
        }
    }

    /**
     * @brief Get the execution history of this task
     *
     * @return a stack of WorkflowTaskExecution objects, one for each attempted execution of the task
     */
    std::stack<WorkflowTask::WorkflowTaskExecution> WorkflowTask::getExecutionHistory() {
        return this->execution_history;
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
            throw std::invalid_argument(
                    "WorkflowTask::addFileToMap(): File ID '" + f->id + "' is already used as input or output file");
        }

        if (map_to_insert.find(f->id) != map_to_insert.end()) {
            throw std::invalid_argument("WorkflowTask::addFileToMap(): File ID '" + f->id + "' already exists");
        }
        map_to_insert[f->id] = f;
    }

    /**
     * @brief Get the number of times a task has failed
     * @return a failure count
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
     * @brief Get the set of input WorkflowFile objects for the task
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
     * @brief Get the set of output WorkflowFile objects for the task
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
     * @brief Get the task's most recent start date
     * @return a start date (-1 if task has not started yet)
     */
    double WorkflowTask::getStartDate() {
        return (not this->execution_history.empty()) ? this->execution_history.top().task_start : -1.0;
    }

    /**
     * @brief Get the task's most recent end date
     * @return a end date (-1 if task has not completed yet or if no execution history exists for this task yet)
     */
    double WorkflowTask::getEndDate() {
        return (not this->execution_history.empty()) ? this->execution_history.top().task_end : -1.0;
    }

    /**
     * @brief Get the tasks's most recent computation start date
     * @return the date when the computation portion of a task started (-1 if computation has not started yet or if no execution history exists for this task yet)
     */
    double WorkflowTask::getComputationStartDate() {
        return (not this->execution_history.empty()) ? this->execution_history.top().computation_start : -1.0;
    }

    /**
     * @brief Get the task's most recent computation end date
     * @return the date when the computation portion of a task ended (-1 if computation has not ended yet or if no execution history exists for this task yet)
     */
    double WorkflowTask::getComputationEndDate() {
        return (not this->execution_history.empty()) ? this->execution_history.top().computation_end : -1.0;
    }

    /**
     * @brief Get the task's most recent read input start date
     * @return the date when the read input portion of the task has begun (-1 if it has not yet begun or if no execution history exists for this task yet)
     */
    double WorkflowTask::getReadInputStartDate() {
        return (not this->execution_history.empty()) ? this->execution_history.top().read_input_start : -1.0;
    }

    /**
     * @brief Get the task's most recent read input end date
     * @return the date when the read input portion of the task has completed (-1 if it has not begun or if no execution history exists for this task yet)
     */
    double WorkflowTask::getReadInputEndDate() {
        return (not this->execution_history.empty()) ? this->execution_history.top().read_input_end : -1.0;
    }

    /**
     * @brief Get the task's most recent write output start date
     * @return the date when the write output portion of a task has begun (-1 if it has not yet started or if no execution history exists for this task yet)
     */
    double WorkflowTask::getWriteOutputStartDate() {
        return (not this->execution_history.empty()) ? this->execution_history.top().write_output_start : -1.0;
    }

    /**
     * @brief Get the task's most recent write output end date
     * @return the date when the write output portion of a task has completed (-1 if it has not completed yet or if no execution history exists for this task yet)
     */
    double WorkflowTask::getWriteOutputEndDate() {
        return (not this->execution_history.empty()) ? this->execution_history.top().write_output_end : -1.0;
    }

    /**
     * @brief Get the task's most recent failure date
     * @return the date when the task failed (-1 if it didn't fail or if no execution history exists for this task yet)
     */
    double WorkflowTask::getFailureDate() {
        return (not this->execution_history.empty()) ? this->execution_history.top().task_failed : -1.0;
    }

    /**
     * @brief Get the tasks's most recent termination date (when it was explicitly requested to be terminated by the WMS)
     * @return the date when the task was terminated (-1 if it wasn't terminated or if not execution history exists for this task yet)
     */
    double WorkflowTask::getTerminationDate() {
        return (not this->execution_history.empty()) ? this->execution_history.top().task_terminated : -1.0;
    }

    /**
     * @brief Update the task's top level (looking only at the parents, and updating children)
     * @return the task's updated top level
     */
    unsigned long WorkflowTask::updateTopLevel() {
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
        std::vector<WorkflowTask *> children = this->workflow->getTaskChildren(this);
        for (auto child : children) {
            child->updateTopLevel();
        }
        return this->toplevel;
    }

    /**
     * @brief Returns the task's top level (max number of hops on a reverse path up to an entry task. Entry
     *        tasks have a top-level of 0)
     * @return
     */
    unsigned long WorkflowTask::getTopLevel() {
        return this->toplevel;
    }

    /**
     * @brief Returns the name of the host on which the task has most recently been executed, or "" if
     *        the task has never been executed yet
     * @return hostname
     */
    std::string WorkflowTask::getExecutionHost() {
        return (not this->execution_history.empty()) ? this->execution_history.top().execution_host : "";
    }

    /**
     * @brief Returns the number of cores allocated for this task's most recent execution or 0 if an execution attempt was never made
     * @return number of cores
     */
    unsigned long WorkflowTask::getNumCoresAllocated() {
        return (not this->execution_history.empty()) ? this->execution_history.top().num_cores_allocated : 0;
    }

    /**
     * @brief Sets the host on which this task is running
     *
     * @param hostname: the host name
     */
    void WorkflowTask::setExecutionHost(std::string hostname) {
        if (not this->execution_history.empty()) {
            this->execution_history.top().execution_host = hostname;
        } else {
            throw std::runtime_error(
                    "WorkflowTask::setExecutionHost() cannot be called before WorkflowTask::setStartDate()");
        }
    }

    /**
     * @brief Sets the number of cores allocated for this task
     * @param num_cores: the number of cores allocated to this task
     */
    void WorkflowTask::setNumCoresAllocated(unsigned long num_cores) {
        if (not this->execution_history.empty()) {
            this->execution_history.top().num_cores_allocated = num_cores;
        } else {
            throw std::runtime_error(
                    "WorkflowTask::setNumCoresAllocated() cannot be called before WorkflowTask::setStartDate()");
        }
    }

    /**
     * @brief Get a map of src and dst hosts for file transfers
     *        (only available for WorkflowTask::TaskType::TRANSFER_IN or WorkflowTask::TaskType::TRANSFER_OUT tasks)
     * @return transfer src and dst pair
     */
    std::map<WorkflowFile *, std::pair<std::string, std::string>> WorkflowTask::getFileTransfers() const {
        return this->fileTransfers;
    }

    /**
     * @brief Set a pair of src and dest hosts for transfers (it is only meaningful for
     *        WorkflowTask::TaskType::TRANSFER tasks)
     *
     * @param workflow_file: a pointer to a file to be transferred
     * @param src: source hostname
     * @param dst: destination hostname
     */
    void WorkflowTask::addSrcDest(WorkflowFile *workflow_file, const std::string &src, const std::string &dst) {
        if (this->fileTransfers.find(workflow_file) == this->fileTransfers.end()) {
            this->fileTransfers.insert(std::make_pair(workflow_file, std::make_pair(src, dst)));
        }
    }

};
