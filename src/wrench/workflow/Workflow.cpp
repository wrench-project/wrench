/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <pugixml.hpp>
#include <nlohmann/json.hpp>
#include <wrench/util/UnitParser.h>
#include <wrench/workflow/WorkflowTask.h>

#include "wrench/logging/TerminalOutput.h"
#include "wrench/simulation/SimulationMessage.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/workflow/Workflow.h"
#include "wrench/workflow/DagOfTasks.h"

WRENCH_LOG_CATEGORY(wrench_core_workflow, "Log category for Workflow");

namespace wrench {


    /**
     * @brief Create and add a new computational task to the workflow
     *
     * @param id: a unique string id
     * @param flops: number of flops
     * @param min_num_cores: the minimum number of cores required to run the task
     * @param max_num_cores: the maximum number of cores that can be used by the task (use INT_MAX for infinity)
     * @param parallel_efficiency: the multi-core parallel efficiency (number between 0.0 and 1.0)
     * @param memory_requirement: memory requirement (in bytes)
     *
     * @return the WorkflowTask instance
     *
     * @throw std::invalid_argument
     */
    WorkflowTask *Workflow::addTask(const std::string id,
                                    double flops,
                                    unsigned long min_num_cores,
                                    unsigned long max_num_cores,
                                    double parallel_efficiency,
                                    double memory_requirement) {


        if ((flops < 0.0) || (min_num_cores < 1) || (min_num_cores > max_num_cores) ||
            (parallel_efficiency <= 0.0) || (parallel_efficiency > 1.0) || (memory_requirement < 0)) {
            throw std::invalid_argument("WorkflowTask::addTask(): Invalid argument");
        }

//        if ((min_num_cores == 0) and (max_num_cores != 0)) {
//            throw std::invalid_argument("WorkflowTask::addTask(): A task with a minimum number of cores set to 0 must also have a maximum number of cores set to 0");
//        }
//
//        if ((min_num_cores == 0) and (flops > 0)) {
//            throw std::invalid_argument("WorkflowTask::addTask(): A task with a minimum number of cores set to 0 must have 0 flops");
//        }

        // Check that the task doesn't really exist
        if (tasks.find(id) != tasks.end()) {
            throw std::invalid_argument("Workflow::addTask(): Task ID '" + id + "' already exists");
        }

        // Create the WorkflowTask object
        auto task = new WorkflowTask(id, flops, min_num_cores, max_num_cores, parallel_efficiency,
                                              memory_requirement);
        // Associate the workflow to the task
        task->workflow = this;

        task->toplevel = 0; // upon creation, a task is an exit task

        // Create a DAG node for it
        this->dag.addVertex(task);

        tasks[task->id] = std::unique_ptr<WorkflowTask>(task); // owner

        return task;
    }

    /**
     * @brief Remove a file from the workflow. WARNING: this method de-allocated
     *        memory for the file, making any pointer to the file invalid
     * @param file: a file
     *
     * @throw std::invalid_argument
     */
    void Workflow::removeFile(WorkflowFile *file) {

        if (file->getOutputOf() != nullptr) {
            throw std::invalid_argument("Workflow::removeFile(): File " +
                                        file->getID() + " cannot be removed because it is output of task " +
                                        file->getOutputOf()->getID());
        }

        if (file->getInputOf().size() > 0) {
            throw std::invalid_argument("Workflow::removeFile(): File " +
                                        file->getID() + " cannot be removed because it is input to " +
                                        std::to_string(file->getInputOf().size()) + " tasks");
        }

        this->files.erase(file->getID());
    }

    /**
     * @brief Remove a task from the workflow. WARNING: this method de-allocated
     *        memory for the task, making any pointer to the task invalid
     *
     * @param task: a task
     *
     * @throw std::invalid_argument
     */
    void Workflow::removeTask(WorkflowTask *task) {

        if (task == nullptr) {
            throw std::invalid_argument("Workflow::removeTask(): Invalid arguments");
        }

        // check that task exists (this should never happen)
        if (tasks.find(task->id) == tasks.end()) {
            throw std::invalid_argument("Workflow::removeTask(): Task '" + task->id + "' does not exist");
        }

        // Fix all files
        for (auto &f : task->getInputFiles()) {
            f->getInputOf().erase(task->getID());
        }
        for (auto &f : task->getOutputFiles()) {
            f->setOutputOf(nullptr);
        }

        // Get the task children
        auto children = this->dag.getChildren(task);

        // Remove the task from the DAG
        this->dag.removeVertex(task);

        // Remove the task from the master list
        tasks.erase(tasks.find(task->id));

        // Brute-force update of the top-level of all the children of the removed task
        for (auto const &child : children) {
            child->updateTopLevel();
        }

    }

    /**
     * @brief Find a WorkflowTask based on its ID
     *
     * @param id: a string id
     *
     * @return a workflow task (or throws a std::invalid_argument if not found)
     *
     * @throw std::invalid_argument
     */
    WorkflowTask *Workflow::getTaskByID(const std::string &id) {
        if (tasks.find(id) == tasks.end()) {
            throw std::invalid_argument("Workflow::getTaskByID(): Unknown WorkflowTask ID " + id);
        }
        return tasks[id].get();
    }

    /**
     * @brief Create a control dependency between two workflow tasks. Will not
     *        do anything if there is already a path between the two tasks.
     *
     * @param src: the parent task
     * @param dst: the child task
     * @param redundant_dependencies: whether DAG redundant dependencies should be kept in the graph
     *
     * @throw std::invalid_argument
     */
    void Workflow::addControlDependency(WorkflowTask *src, WorkflowTask *dst, bool redundant_dependencies) {

        if ((src == nullptr) || (dst == nullptr)) {
            throw std::invalid_argument("Workflow::addControlDependency(): Invalid arguments");
        }

        if (redundant_dependencies || not this->dag.doesPathExist(src, dst)) {

            WRENCH_DEBUG("Adding control dependency %s-->%s", src->getID().c_str(), dst->getID().c_str());
            this->dag.addEdge(src, dst);

            dst->updateTopLevel();

            if (src->getState() != WorkflowTask::State::COMPLETED) {
                dst->setInternalState(WorkflowTask::InternalState::TASK_NOT_READY);
                dst->setState(WorkflowTask::State::NOT_READY);
            }
        }
    }

    /**
     * @brief Add a new file to the workflow
     *
     * @param id: a unique string id
     * @param size: a file size in bytes
     *
     * @return the WorkflowFile instance
     *
     * @throw std::invalid_argument
     */
    WorkflowFile *Workflow::addFile(const std::string id, double size) {

        if (size < 0) {
            throw std::invalid_argument("Workflow::addFile(): Invalid arguments");
        }

        // Create the WorkflowFile object
        if (files.find(id) != files.end()) {
            throw std::invalid_argument("Workflow::addFile(): WorkflowFile with id '" +
                                        id + "' already exists");
        }

        WorkflowFile *file = new WorkflowFile(id, size);
        file->workflow = this;
        // Add if to the set of workflow files
        files[file->id] = std::unique_ptr<WorkflowFile>(file);

        return file;
    }

    /**
     * @brief Find a WorkflowFile based on its ID
     *
     * @param id: a string id
     *
     * @return the WorkflowFile instance (or throws a std::invalid_argument if not found)
     *
     * @throw std::invalid_argument
     */
    WorkflowFile *Workflow::getFileByID(const std::string &id) {
        if (files.find(id) == files.end()) {
            throw std::invalid_argument("Workflow::getFileByID(): Unknown WorkflowFile ID " + id);
        } else {
            return files[id].get();
        }
    }

    /**
     * @brief Output the workflow's dependency graph to EPS
     *
     * @param eps_filename: a filename to which the EPS content is saved
     *
     */
    void Workflow::exportToEPS(std::string eps_filename) {
        throw std::runtime_error("Export to EPS broken / not implemented at the moment");
    }

    /**
     * @brief Get the number of tasks in the workflow
     *
     * @return the number of tasks
     */
    unsigned long Workflow::getNumberOfTasks() {
        return this->tasks.size();
    }

    /**
     * @brief Determine whether one source is an ancestor of a destination task
     *
     * @param src: the source workflow task
     * @param dst: the destination task
     *
     * @return true if there is a path from src to dst, false otherwise
     */
    bool Workflow::pathExists(WorkflowTask *src, WorkflowTask *dst) {
       return  this->dag.doesPathExist(src, dst);
    }

    /**
     * @brief  Constructor
     */
    Workflow::Workflow() {
        this->callback_mailbox = S4U_Mailbox::generateUniqueMailboxName("workflow_mailbox");
        this->simulation = nullptr;
    }

    /**
     * @brief Get a vector of ready tasks
     *
     * @return a vector of tasks
     */
    std::vector<WorkflowTask *> Workflow::getReadyTasks() {

        std::vector<WorkflowTask *> tasks_list;

        for (auto &it : this->tasks) {
            WorkflowTask *task = it.second.get();

            if (task->getState() == WorkflowTask::State::READY) {
                tasks_list.push_back(task);
            }
        }
        return tasks_list;
    }

    /**
     * @brief Get a map of clusters composed of ready tasks
     *
     * @return map of workflow cluster tasks
     */
    // TODO: Implement this more efficiently
    std::map<std::string, std::vector<WorkflowTask *>> Workflow::getReadyClusters() {

        std::map<std::string, std::vector<WorkflowTask *>> task_map;

        for (auto &it : this->tasks) {
            WorkflowTask *task = it.second.get();

            if (task->getState() == WorkflowTask::State::READY) {

                if (task->getClusterID().empty()) {
                    task_map[task->getID()] = {task};

                } else {
                    if (task_map.find(task->getClusterID()) == task_map.end()) {
                        task_map[task->getClusterID()] = {task};
                    } else {
                        // add to clustered task
                        task_map[task->getClusterID()].push_back(task);
                    }
                }
            } else {
                if (task_map.find(task->getClusterID()) != task_map.end()) {
                    task_map[task->getClusterID()].push_back(task);
                }
            }
        }
        return task_map;
    }

    /**
     * @brief Returns whether all tasks are complete
     *
     * @return true or false
     */
    bool Workflow::isDone() {
        for (auto &it : this->tasks) {
            WorkflowTask *task = it.second.get();
            if (task->getState() != WorkflowTask::State::COMPLETED) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Get the list of all tasks in the workflow
     *
     * @return a map of tasks, indexed by ID
     */
    std::map<std::string, WorkflowTask *> Workflow::getTaskMap() {
        std::map<std::string, WorkflowTask *> all_tasks;
        for (auto const &t : this->tasks) {
            all_tasks[t.first] = (t.second.get());
        }
        return all_tasks;
    };

    /**
     * @brief Get the list of all tasks in the workflow
     *
     * @return a vector of tasks
     */
    std::vector<WorkflowTask *> Workflow::getTasks() {
        std::vector<WorkflowTask *> all_tasks;
        for (auto const &t : this->tasks) {
            all_tasks.push_back(t.second.get());
        }
        return all_tasks;
    };

    /**
     * @brief Get the list of all files in the workflow
     *
     * @return a map of files, indexed by file ID
     */
    std::map<std::string, WorkflowFile *> Workflow::getFileMap() const {
        std::map<std::string, WorkflowFile *> all_files;
        for (auto &it : this->files) {
            all_files[it.first] =  (it.second.get());
        }
        return all_files;
    };

    /**
     * @brief Get the list of all files in the workflow
     *
     * @return a vector of files
     */
    std::vector<WorkflowFile *> Workflow::getFiles() const {
        std::vector<WorkflowFile *> all_files;
        for (auto const &f : this->files) {
            all_files.push_back(f.second.get());
        }
        return all_files;
    };

    /**
     * @brief Get the list of children for a task
     *
     * @param task: a workflow task
     *
     * @return a vector of tasks
     */
    std::vector<WorkflowTask *> Workflow::getTaskChildren(const WorkflowTask *task) {
        if (task == nullptr) {
            throw std::invalid_argument("Workflow::getTaskChildren(): Invalid arguments");
        }
        return this->dag.getChildren(task);
    }

    /**
     * @brief Get the number of children for a task
     *
     * @param task: a workflow task
     *
     * @return a number of children
     */
    long Workflow::getTaskNumberOfChildren(const WorkflowTask *task) {
        if (task == nullptr) {
            throw std::invalid_argument("Workflow::getTaskNumberOfChildren(): Invalid arguments");
        }
        return this->dag.getNumberOfChildren(task);
    }

    /**
     * @brief Get the list of parents for a task
     *
     * @param task: a workflow task
     *
     * @return a vector of tasks
     */
    std::vector<WorkflowTask *> Workflow::getTaskParents(const WorkflowTask *task) {
        if (task == nullptr) {
            throw std::invalid_argument("Workflow::getTaskParents(): Invalid arguments");
        }
        return this->dag.getParents(task);
    }

    /**
     * @brief Get the number of parents for a task
     *
     * @param task: a workflow task
     *
     * @return a number of parents
     */
    long Workflow::getTaskNumberOfParents(const WorkflowTask *task) {
        if (task == nullptr) {
            throw std::invalid_argument("Workflow::getTaskNumberOfParents(): Invalid arguments");
        }
        return this->dag.getNumberOfParents(task);
    }

    /**
     * @brief Wait for the next workflow execution event
     *
     * @return a workflow execution event
     */
    std::shared_ptr<WorkflowExecutionEvent> Workflow::waitForNextExecutionEvent() {
        return WorkflowExecutionEvent::waitForNextExecutionEvent(this->callback_mailbox);
    }

    /**
    * @brief Wait for the next worklow execution event
    * @param timeout: a timeout value in seconds
    *
    * @return a workflow execution event, or nullptr if a timeout occurred
    */
    std::shared_ptr<WorkflowExecutionEvent> Workflow::waitForNextExecutionEvent(double timeout) {
        return WorkflowExecutionEvent::waitForNextExecutionEvent(this->callback_mailbox, timeout);
    }

    /**
     * @brief Get the mailbox name associated to this workflow
     *
     * @return the mailbox name
     */
    std::string Workflow::getCallbackMailbox()  {
        return this->callback_mailbox;
    }

    /**
     * @brief Retrieve the list of the input files of the workflow (i.e., those files
     *        that are input to some tasks but output from none)
     *
     * @return a map of files indexed by file ID
     */
    std::map<std::string, WorkflowFile *> Workflow::getInputFileMap() const {
        std::map<std::string, WorkflowFile *> input_files;
        for (auto const &x : this->files) {
            if ((x.second->output_of == nullptr) && (not x.second->input_of.empty())) {
                input_files.insert({x.first, x.second.get()});
            }
        }
        return input_files;
    }

    /**
     * @brief Retrieve the list of the input files of the workflow (i.e., those files
     *        that are input to some tasks but output from none)
     *
     * @return a vector of files
     */
    std::vector<WorkflowFile *> Workflow::getInputFiles() const {
        std::vector<WorkflowFile *> input_files;
        for (auto const &x : this->files) {
            if ((x.second->output_of == nullptr) && (not x.second->input_of.empty())) {
                input_files.push_back(x.second.get());
            }
        }
        return input_files;
    }

    /**
    * @brief Retrieve a list of the output files of the workflow (i.e., those files
    *        that are output from some tasks but input to none)
    *
    * @return a map of files indexed by ID
    */
    std::map<std::string, WorkflowFile *> Workflow::getOutputFileMap() const {
        std::map<std::string, WorkflowFile *> output_files;
        for (auto const &x : this->files) {
            if ((x.second->output_of != nullptr) && (x.second->input_of.empty())) {
                output_files.insert({x.first, x.second.get()});
            }
        }
        return output_files;
    }

    /**
   * @brief Retrieve a list of the output files of the workflow (i.e., those files
   *        that are output from some tasks but input to none)
   *
   * @return a vector of files
   */
    std::vector<WorkflowFile *> Workflow::getOutputFiles() const {
        std::vector<WorkflowFile *> output_files;
        for (auto const &x : this->files) {
            if ((x.second->output_of != nullptr) && (x.second->input_of.empty())) {
                output_files.push_back(x.second.get());
            }
        }
        return output_files;
    }

    /**
     * @brief Get the total number of flops for a list of tasks
     *
     * @param tasks: a vector of tasks
     *
     * @return the total number of flops
     */
    double Workflow::getSumFlops(const std::vector<WorkflowTask *> tasks) {
        double total_flops = 0;
        for (auto const &task : tasks) {
            total_flops += task->getFlops();
        }
        return total_flops;
    }

    /**
     * @brief Returns all tasks with top-levels in a range
     * @param min: the low end of the range (inclusive)
     * @param max: the high end of the range (inclusive)
     * @return a vector of tasks
     */
    std::vector<WorkflowTask *> Workflow::getTasksInTopLevelRange(unsigned long min, unsigned long max) {
        std::vector<WorkflowTask *> to_return;
        for (auto const &task : this->getTasks()) {
            if ((task->getTopLevel() >= min) and (task->getTopLevel() <= max)) {
                to_return.push_back(task);
            }
        }
        return to_return;
    }

    /**
     * @brief Get the list of exit tasks of the workflow, i.e., those tasks
     *        that don't have parents
     * @return A map of tasks indexed by their IDs
     */
    std::map<std::string, WorkflowTask *> Workflow::getEntryTaskMap() const {
        // TODO: This could be done more efficiently at the DAG level
        std::map<std::string, WorkflowTask *> entry_tasks;
        for (auto const &t : this->tasks) {
            auto task = t.second.get();
            if (task->getNumberOfParents() == 0) {
                entry_tasks[task->getID()] = task;
            }
        }
        return entry_tasks;
    }

    /**
     * @brief Get the list of exit tasks of the workflow, i.e., those tasks
     *        that don't have parents
     * @return A vector of tasks
     */
    std::vector<WorkflowTask *> Workflow::getEntryTasks() const {
        // TODO: This could be done more efficiently at the DAG level
        std::vector<WorkflowTask *> entry_tasks;
        for (auto const &t : this->tasks) {
            auto task = t.second.get();
            if (task->getNumberOfParents() == 0) {
                entry_tasks.push_back(task);
            }
        }
        return entry_tasks;
    }

       /**
        * @brief Get the exit tasks of the workflow, i.e., those tasks
        *        that don't have children
        * @return A map of tasks indexed by their IDs
        */
    std::map<std::string, WorkflowTask *> Workflow::getExitTaskMap() const {
        // TODO: This could be done more efficiently at the DAG level
        std::map<std::string, WorkflowTask *> exit_tasks;
        for (auto const &t : this->tasks) {
            auto task = t.second.get();
            if (task->getNumberOfChildren() == 0) {
                exit_tasks[task->getID()] = task;
            }
        }
        return exit_tasks;
    }

    /**
    * @brief Get the exit tasks of the workflow, i.e., those tasks
    *        that don't have children
    * @return A vector of tasks
    */
    std::vector<WorkflowTask *> Workflow::getExitTasks() const {
        // TODO: This could be done more efficiently at the DAG level
        std::vector<WorkflowTask *> exit_tasks;
        for (auto const &t : this->tasks) {
            auto task = t.second.get();
            if (task->getNumberOfChildren() == 0) {
                exit_tasks.push_back(task);
            }
        }
        return exit_tasks;
    }


    /**
     * @brief Returns the number of levels in the workflow
     * @return the number of levels
     */
    unsigned long Workflow::getNumLevels() {
        unsigned long max_top_level = 0;
        for (auto const &t : this->tasks) {
            auto task = t.second.get();
            if (task->getNumberOfChildren() == 0) {
                if (1 + task->getTopLevel() > max_top_level) {
                    max_top_level = 1 + task->getTopLevel();
                }
            }
        }
        return max_top_level;
    }

    /**
     * @brief Returns the workflow's completion date
     * @return a date in seconds (or a negative value
     *        If the workflow has not completed)
     */
    double Workflow::getCompletionDate() {
        double makespan = -1.0;
        // Get te last level
        std::vector<WorkflowTask *> last_tasks = this->getTasksInTopLevelRange(this->getNumLevels() - 1,
                                                                               this->getNumLevels() - 1);
        for (auto task : last_tasks) {
            if (task->getState() != WorkflowTask::State::COMPLETED) {
                makespan = -1.0;
                break;
            } else {
                makespan = std::max<double>(makespan, task->getEndDate());
            }
        }
        return makespan;
    }

}
