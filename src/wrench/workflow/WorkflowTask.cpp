/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/workflow/parallel_model/AmdahlParallelModel.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/workflow/WorkflowTask.h>
#include <wrench/workflow/Workflow.h>
#include <wrench/services/compute/cloud/CloudComputeService.h>

#include <utility>

WRENCH_LOG_CATEGORY(wrench_core_workflow_task, "Log category for WorkflowTask");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param id: the task id
     * @param flops: the task's number of flops
     * @param min_num_cores: the minimum number of cores required for running the task
     * @param max_num_cores: the maximum number of cores that the task can use (infinity: ULONG_MAX)
     * @param memory_requirement: memory_manager_service requirement in bytes
     */
    WorkflowTask::WorkflowTask(std::string id,
                               const double flops,
                               const unsigned long min_num_cores,
                               const unsigned long max_num_cores,
                               const double memory_requirement) : id(std::move(id)), color(""), flops(flops),
                                                                  min_num_cores(min_num_cores),
                                                                  max_num_cores(max_num_cores),
                                                                  memory_requirement(memory_requirement),
                                                                  execution_host(""),
                                                                  visible_state(WorkflowTask::State::READY),
                                                                  //            upcoming_visible_state(WorkflowTask::State::UNKNOWN),
                                                                  //            internal_state(WorkflowTask::InternalState::TASK_READY),
                                                                  job(nullptr) {
        // The default is that the task is perfectly parallelizable
        this->parallel_model = ParallelModel::CONSTANTEFFICIENCY(1.0);
        this->toplevel = 0;
    }

    /**
     * @brief Add an input file to the task
     *
     * @param file: the file
     * @throw std::invalid_argument
     */
    void WorkflowTask::addInputFile(const std::shared_ptr<DataFile> &file) {
        WRENCH_DEBUG("Adding file '%s' as input to task %s", file->getID().c_str(), this->getID().c_str());

        // If the file is alreadxy an input file of the task, complain
        if (this->input_files.find(file->getID()) != this->input_files.end()) {
            throw std::invalid_argument(" WorkflowTask::addInputFile(): File ID '" + file->getID() +
                                        "' is already an input file of task '" + this->getID() + "'");
        }

        // If file is already an output file of the task, complain
        if (this->output_files.find(file->getID()) != this->output_files.end()) {
            throw std::invalid_argument(" WorkflowTask::addInputFile(): File ID '" + file->getID() +
                                        "' is already an output file of task '" + this->getID() + "'");
        }

        // Add the file
        this->input_files[file->getID()] = file;
        this->workflow->task_input_files[file].insert(this->getSharedPtr());

        // Add control dependency
        if (this->workflow->task_output_files.find(file) != this->workflow->task_output_files.end()) {
            workflow->addControlDependency(this->workflow->task_output_files[file], this->getSharedPtr());
        }
    }

    /**
     * @brief Add an output file to the task
     *
     * @param file: the file
     */
    void WorkflowTask::addOutputFile(const std::shared_ptr<DataFile> &file) {
        WRENCH_DEBUG("Adding file '%s' as output t task %s", file->getID().c_str(), this->getID().c_str());

        // If the file is already input, complain
        if (this->input_files.find(file->getID()) != this->input_files.end()) {
            throw std::invalid_argument("WorkflowTask::addOutputFile(): File ID '" + file->getID() +
                                        "' is already an input file of task '" + this->getID() + "'");
        }

        // If the file is already output of another task, complain
        if (this->workflow->isFileOutputOfSomeTask(file)) {
            throw std::invalid_argument("WorkflowTask::addOutputFile(): File ID '" + file->getID() +
                                        "' is already an output file of another task (task '" +
                                        this->workflow->getTaskThatOutputs(file)->getID() + "')");
        }

        // Otherwise proceeed
        this->output_files[file->getID()] = file;
        this->workflow->task_output_files[file] = this->getSharedPtr();

        for (auto const &x: this->workflow->getTasksThatInput(file)) {
            workflow->addControlDependency(this->getSharedPtr(), x);
        }
    }

    /**
     * @brief Get the id of the task
     *
     * @return an id as a string
     */
    const std::string &WorkflowTask::getID() const {
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
     * @brief Set the number of f of the task (to be used only in very specific
     * cases in which it is guaranteed that changing a task's work after that task
     * has been created is a valid thing to do)
     *
     * @param f: the number of f
     */
    void WorkflowTask::setFlops(double f) {
        this->flops = f;
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
     * @brief Get the memory_manager_service requirement of the task
     *
     * @return a memory_manager_service requirement (in bytes)
     */
    double WorkflowTask::getMemoryRequirement() const {
        return this->memory_requirement;
    }

    /**
     * @brief Get the number of children of a task
     *
     * @return a number of children
     */
    unsigned long WorkflowTask::getNumberOfChildren() {
        return this->workflow->getTaskNumberOfChildren(this->getSharedPtr());
    }

    /**
     * @brief Get the children of a task
     *
     * @return a list of workflow tasks
     */
    std::vector<std::shared_ptr<WorkflowTask>> WorkflowTask::getChildren() {
        return this->getWorkflow()->getTaskChildren(this->getSharedPtr());
    }

    /**
     * @brief Get the number of parents of a task
     *
     * @return a number of parents
     */
    unsigned long WorkflowTask::getNumberOfParents() {
        return this->workflow->getTaskNumberOfParents(this->getSharedPtr());
    }

    /**
     * @brief Get the parents of a task
     *
     * @return a list of workflow tasks
     */
    std::vector<std::shared_ptr<WorkflowTask>> WorkflowTask::getParents() {
        return this->getWorkflow()->getTaskParents(this->getSharedPtr());
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
    * @brief Get the state of the task as a string
    *
    * @return a string
    */
    std::string WorkflowTask::getStateAsString() const {
        switch (this->visible_state) {
            case WorkflowTask::State::NOT_READY:
                return "NOT_READY";
            case WorkflowTask::State::READY:
                return "READY";
            case WorkflowTask::State::PENDING:
                return "PENDING";
            case WorkflowTask::State::COMPLETED:
                return "COMPLETED";
            case WorkflowTask::State::UNKNOWN:
            default:
                return "UNKNOWN";
        }
    }

    //    /**
    //    * @brief Get the state of the task
    //    *
    //    * @return a task state
    //    */
    //    WorkflowTask::State WorkflowTask::getUpcomingState() const {
    //        return this->upcoming_visible_state;
    //    }

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
     * @param internal_state: the internal state
     *
     * @return an internal state as a string
     */
    std::string WorkflowTask::stateToString(WorkflowTask::InternalState internal_state) {
        switch (internal_state) {
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
    std::shared_ptr<Workflow> WorkflowTask::getWorkflow() const {
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
        if (this->visible_state == WorkflowTask::State::READY) {
            this->workflow->ready_tasks.erase(this->getSharedPtr());
        }
        this->visible_state = state;
        if (state == WorkflowTask::State::READY) {
            this->workflow->ready_tasks.insert(this->getSharedPtr());
        }
    }

    //    /**
    //     * @brief Set the upcoming visible state of the task
    //     *
    //     * @param state: the task state
    //     */
    //    void WorkflowTask::setUpcomingState(WorkflowTask::State state) {
    //        this->upcoming_visible_state = state;
    //    }

    /**
     * @brief Set the task's containing j
     *
     * @param j: the j
     */
    void WorkflowTask::setJob(Job *j) {
        this->job = j;
    }

    /**
     * @brief Get the task's containing job
     * @return job: the job
     */
    Job *WorkflowTask::getJob() const {
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
     * @brief Set the cluster c_id for the task
     *
     * @param c_id: cluster c_id the task belongs to
     */
    void WorkflowTask::setClusterID(const std::string &c_id) {
        this->cluster_id = c_id;
    }

    /**
     * @brief Get the task priority. By default, priority is 0.
     * @return the task priority
     */
    unsigned long WorkflowTask::getPriority() const {
        return this->priority;
    }

    /**
     * @brief Set the task p
     * @param p: task p
     */
    void WorkflowTask::setPriority(long p) {
        this->priority = p;
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
     * @param a_cpu: task average CPU usage
     */
    void WorkflowTask::setAverageCPU(double a_cpu) {
        this->average_cpu = a_cpu;
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
     * @param b_read: number of bytes read by the task in KB
     */
    void WorkflowTask::setBytesRead(unsigned long b_read) {
        this->bytes_read = b_read;
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
     * @param b_written: number of bytes written by the task in KB
     */
    void WorkflowTask::setBytesWritten(unsigned long b_written) {
        this->bytes_written = b_written;
    }

    /**
     * @brief Set the task's start date (which pushing a new execution history!)
     *
     * @param date: the start date
     */
    void WorkflowTask::setStartDate(double date) {
        this->execution_history.push(WorkflowTask::WorkflowTaskExecution(date));
    }

    /**
     * @brief Update the task's start date.
     *
     * @param date: the start date
     */
    void WorkflowTask::updateStartDate(double date) {
        if (not this->execution_history.empty()) {
            this->execution_history.top().task_start = date;
        } else {
            throw std::runtime_error("WorkflowTask::updateStartDate() cannot be called before WorkflowTask::setStartDate()");
        }
        this->execution_history.top().task_start = date;
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
    std::stack<WorkflowTask::WorkflowTaskExecution> WorkflowTask::getExecutionHistory() const {
        return this->execution_history;
    }

    /**
     * @brief Get the number of times a task has failed
     *
     * @return a failure count
     */
    unsigned int WorkflowTask::getFailureCount() const {
        return this->failure_count;
    }

    /**
     * @brief Increment the failure count of a task
     */
    void WorkflowTask::incrementFailureCount() {
        this->failure_count++;
    }

    /**
     * @brief Get the list of input DataFile objects for the task
     * @return a list workflow files
     */
    std::vector<std::shared_ptr<DataFile>> WorkflowTask::getInputFiles() const {
        std::vector<std::shared_ptr<DataFile>> input;
        input.reserve(this->input_files.size());

        for (const auto &f: this->input_files) {
            input.emplace_back(f.second);
        }
        return input;
    }

    /**
     * @brief Get the list of output DataFile objects for the task
     * @return a list of workflow files
     */
    std::vector<std::shared_ptr<DataFile>> WorkflowTask::getOutputFiles() const {
        std::vector<std::shared_ptr<DataFile>> output;
        output.reserve(this->output_files.size());

        for (const auto &f: this->output_files) {
            output.emplace_back(f.second);
        }
        return output;
    }

    /**
     * @brief Get the task's most recent start date
     * @return a start date (-1 if task has not started yet)
     */
    double WorkflowTask::getStartDate() const {
        return (not this->execution_history.empty()) ? this->execution_history.top().task_start : -1.0;
    }

    /**
     * @brief Get the task's most recent end date
     * @return a end date (-1 if task has not completed yet or if no execution history exists for this task yet)
     */
    double WorkflowTask::getEndDate() const {
        return (not this->execution_history.empty()) ? this->execution_history.top().task_end : -1.0;
    }

    /**
     * @brief Get the tasks's most recent computation start date
     * @return the date when the computation portion of a task started (-1 if computation has not started yet or if no execution history exists for this task yet)
     */
    double WorkflowTask::getComputationStartDate() const {
        return (not this->execution_history.empty()) ? this->execution_history.top().computation_start : -1.0;
    }

    /**
     * @brief Get the task's most recent computation end date
     * @return the date when the computation portion of a task ended (-1 if computation has not ended yet or if no execution history exists for this task yet)
     */
    double WorkflowTask::getComputationEndDate() const {
        return (not this->execution_history.empty()) ? this->execution_history.top().computation_end : -1.0;
    }

    /**
     * @brief Get the task's most recent read input start date
     * @return the date when the read input portion of the task has begun (-1 if it has not yet begun or if no execution history exists for this task yet)
     */
    double WorkflowTask::getReadInputStartDate() const {
        return (not this->execution_history.empty()) ? this->execution_history.top().read_input_start : -1.0;
    }

    /**
     * @brief Get the task's most recent read input end date
     * @return the date when the read input portion of the task has completed (-1 if it has not begun or if no execution history exists for this task yet)
     */
    double WorkflowTask::getReadInputEndDate() const {
        return (not this->execution_history.empty()) ? this->execution_history.top().read_input_end : -1.0;
    }

    /**
     * @brief Get the task's most recent write output start date
     * @return the date when the write output portion of a task has begun (-1 if it has not yet started or if no execution history exists for this task yet)
     */
    double WorkflowTask::getWriteOutputStartDate() const {
        return (not this->execution_history.empty()) ? this->execution_history.top().write_output_start : -1.0;
    }

    /**
     * @brief Get the task's most recent write output end date
     * @return the date when the write output portion of a task has completed (-1 if it has not completed yet or if no execution history exists for this task yet)
     */
    double WorkflowTask::getWriteOutputEndDate() const {
        return (not this->execution_history.empty()) ? this->execution_history.top().write_output_end : -1.0;
    }

    /**
     * @brief Get the task's most recent failure date
     * @return the date when the task failed (-1 if it didn't fail or if no execution history exists for this task yet)
     */
    double WorkflowTask::getFailureDate() const {
        return (not this->execution_history.empty()) ? this->execution_history.top().task_failed : -1.0;
    }

    /**
     * @brief Get the tasks's most recent termination date (when it was explicitly requested to be terminated by the execution controller)
     * @return the date when the task was terminated (-1 if it wasn't terminated or if not execution history exists for this task yet)
     */
    double WorkflowTask::getTerminationDate() const {
        return (not this->execution_history.empty()) ? this->execution_history.top().task_terminated : -1.0;
    }

    /**
     * @brief Update the task's top level (looking only at the parents, and updating children)
     * @return the task's updated top level
     */
    unsigned long WorkflowTask::updateTopLevel() {
        std::vector<std::shared_ptr<WorkflowTask>> parents = this->workflow->getTaskParents(this->getSharedPtr());
        if (parents.empty()) {
            this->toplevel = 0;
        } else {
            unsigned long max_toplevel = 0;
            for (const auto &parent: parents) {
                max_toplevel = (max_toplevel < parent->toplevel ? parent->toplevel : max_toplevel);
            }
            this->toplevel = 1 + max_toplevel;
        }
        std::vector<std::shared_ptr<WorkflowTask>> children = this->workflow->getTaskChildren(this->getSharedPtr());
        for (const auto &child: children) {
            child->updateTopLevel();
        }
        return this->toplevel;
    }

    /**
     * @brief Returns the task's top level (max number of hops on a reverse path up to an entry task. Entry
     *        tasks have a top-level of 0)
     * @return
     */
    unsigned long WorkflowTask::getTopLevel() const {
        return this->toplevel;
    }

    /**
     * @brief Returns the name of the host on which the task has most recently been executed, or "" if
     *        the task has never been executed yet. Could be a virtual hostname.
     * @return hostname
     */
    std::string WorkflowTask::getExecutionHost() const {
        return (not this->execution_history.empty()) ? this->execution_history.top().execution_host : "";
    }

    /**
    * @brief Returns the name of the PHYSICAL host on which the task has most recently been executed, or "" if
    *        the task has never been executed yet.
    * @return hostname
    */
    std::string WorkflowTask::getPhysicalExecutionHost() const {
        return (not this->execution_history.empty()) ? this->execution_history.top().physical_execution_host : "";
    }

    /**
     * @brief Returns the number of cores allocated for this task's most recent execution or 0 if an execution attempt was never made
     * @return number of cores
     */
    unsigned long WorkflowTask::getNumCoresAllocated() const {
        return (not this->execution_history.empty()) ? this->execution_history.top().num_cores_allocated : 0;
    }

    /**
     * @brief Sets the host on which this task is running.If the hostname is a VM name, then
     * the corresponding physical host name will be set!
     * @param hostname: the host name
     */
    void WorkflowTask::setExecutionHost(const std::string &hostname) {
        std::string physical_hostname;
        /** The conversion below has been removed as it makes more sense to keep the virtual and the physical separate **/
        // Convert the virtual hostname to a physical hostname if needed
        if (S4U_VirtualMachine::vm_to_pm_map.find(hostname) != S4U_VirtualMachine::vm_to_pm_map.end()) {
            physical_hostname = S4U_VirtualMachine::vm_to_pm_map[hostname];
        } else {
            physical_hostname = hostname;
        }
        if (not this->execution_history.empty()) {
            this->execution_history.top().execution_host = hostname;
            this->execution_history.top().physical_execution_host = physical_hostname;
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
     * @brief Get the task's color ("" if none)
     * @return A color string in  "#rrggbb" format
     */
    std::string WorkflowTask::getColor() const {
        return this->color;
    }

    /**
     * @brief Set the task's color
     * @param c: A color string in  "#rrggbb" format
     */
    void WorkflowTask::setColor(const std::string& c) {
        this->color = c;
    }

    /**
     * @brief Set the task's parallel model
     * @param model: a parallel model
     */
    void WorkflowTask::setParallelModel(std::shared_ptr<ParallelModel> model) {
        this->parallel_model = std::move(model);
    }

    /**
     * @brief Get the task's parallel model
     * @return the parallel model
     */
    std::shared_ptr<ParallelModel> WorkflowTask::getParallelModel() const {
        return this->parallel_model;
    }

    /**
     * @brief Update task readiness
     */
    void WorkflowTask::updateReadiness() {
        if (this->getState() == WorkflowTask::State::NOT_READY) {
            for (auto const &parent: this->getParents()) {
                if (parent->getState() != WorkflowTask::State::COMPLETED) {
                    return;
                }
            }
            this->setState(WorkflowTask::State::READY);
        } else if (this->getState() == WorkflowTask::State::READY) {
            for (auto const &parent: this->getParents()) {
                if (parent->getState() != WorkflowTask::State::COMPLETED) {
                    this->setState(WorkflowTask::State::NOT_READY);
                    return;
                }
            }
        } else {
            // do nothing
        }
    }

};// namespace wrench
