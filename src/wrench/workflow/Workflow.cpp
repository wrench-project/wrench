/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <lemon/list_graph.h>
#include <lemon/graph_to_eps.h>
#include <lemon/bfs.h>
#include <pugixml.hpp>
#include <json.hpp>

#include "wrench/logging/TerminalOutput.h"
#include "wrench/simulation/SimulationMessage.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/workflow/Workflow.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(workflow, "Log category for Workflow");

namespace wrench {

    /**
     * @brief Create and add a new computational task to the workflow
     *
     * @param id: a unique string id
     * @param flops: number of flops
     * @param min_num_cores: the minimum number of cores required to run the task
     * @param max_num_cores: the maximum number of cores that can be used by the task (0 means infinity)
     * @param parallel_efficiency: the multi-core parallel efficiency (number between 0.0 and 1.0)
     * @param memory_requirement: memory requirement (in bytes)
     *
     * @return the WorkflowTask instance
     *
     * @throw std::invalid_argument
     */
    WorkflowTask *Workflow::addTask(const std::string id,
                                    double flops,
                                    int min_num_cores,
                                    int max_num_cores,
                                    double parallel_efficiency,
                                    double memory_requirement) {

      if ((flops < 0.0) || (min_num_cores < 1) || (max_num_cores < 0) ||
          ((max_num_cores > 0) && (min_num_cores > max_num_cores))) {
        throw std::invalid_argument("WorkflowTask::addTask(): Invalid argument");
      }

      // Check that the task doesn't really exist
      if (tasks.find(id) != tasks.end()) {
        throw std::invalid_argument("Workflow::addTask(): Task ID '" + id + "' already exists");
      }

      // Create the WorkflowTask object
      WorkflowTask *task = new WorkflowTask(id, flops, min_num_cores, max_num_cores, parallel_efficiency, memory_requirement);
      // Create a DAG node for it
      task->workflow = this;
      task->DAG = this->DAG.get();
      task->DAG_node = DAG->addNode();
      // Add it to the DAG node's metadata
      (*DAG_node_map)[task->DAG_node] = task;
      // Add it to the set of workflow tasks
      tasks[task->id] = std::unique_ptr<WorkflowTask>(task); // owner

      return task;
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

      // check that task exists
      if (tasks.find(task->id) == tasks.end()) {
        throw std::invalid_argument("Workflow::removeTask(): Task '" + task->id + "' does not exist");
      }

      DAG.get()->erase(task->DAG_node);
      tasks.erase(tasks.find(task->id));
    }

    /**
     * @brief Find a WorkflowTask object based on its ID
     *
     * @param id: a string id
     *
     * @return a workflow task
     *
     * @throw std::invalid_argument
     */
    WorkflowTask *Workflow::getWorkflowTaskByID(const std::string id) {
      if (not tasks[id]) {
        throw std::invalid_argument("Workflow::getWorkflowTaskByID(): Unknown WorkflowTask ID " + id);
      }
      return tasks[id].get();
    }

    /**
     * @brief Create a control dependency between two workflow tasks. Will not
     *        do anything if there is already a path between the two tasks.
     *
     * @param src: the parent task
     * @param dst: the child task
     *
     * @throw std::invalid_argument
     */
    void Workflow::addControlDependency(WorkflowTask *src, WorkflowTask *dst) {

      if ((src == nullptr) || (dst == nullptr)) {
        throw std::invalid_argument("Workflow::addControlDependency(): Invalid arguments");
      }

      if (not pathExists(src, dst)) {

        WRENCH_DEBUG("Adding control dependency %s-->%s",
                     src->getId().c_str(), dst->getId().c_str());
        DAG->addArc(src->DAG_node, dst->DAG_node);

        if (src->getState() != WorkflowTask::COMPLETED) {
          updateTaskState(dst, WorkflowTask::NOT_READY);
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
     * @brief Find a WorkflowFile object based on its ID
     *
     * @param id: a string id
     *
     * @return the WorkflowFile instance, or nullptr if not found
     *
     * @throw std::invalid_argument
     */
    WorkflowFile *Workflow::getWorkflowFileByID(const std::string id) {
      if (files.find(id) == files.end()) {
        throw std::invalid_argument("Workflow::getWorkflowFileByID(): Unknown WorkflowFile ID " + id);
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
      graphToEps(*DAG, eps_filename).run();
      WRENCH_INFO("Export to EPS broken / not implemented at the moment");
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
     * @brief Create a workflow based on a DAX file
     *
     * @param filename: the path to the DAX file
     *
     * @throw std::invalid_argument
     */
    void Workflow::loadFromDAX(const std::string &filename) {

      pugi::xml_document dax_tree;

      if (not dax_tree.load_file(filename.c_str())) {
        throw std::invalid_argument("Workflow::loadFromDAX(): Invalid DAX file");
      }

      // Get the root node
      pugi::xml_node dag = dax_tree.child("adag");

      // Iterate through the "job" nodes
      for (pugi::xml_node job = dag.child("job"); job; job = job.next_sibling("job")) {
        WorkflowTask *task;
        // Get the job attributes
        std::string id = job.attribute("id").value();
        std::string name = job.attribute("name").value();
        double flops = std::strtod(job.attribute("runtime").value(), NULL);
        int num_procs = 1;
        bool found_one = false;
        for (std::string tag : {"numprocs", "num_procs", "numcores", "num_cores"}) {
          if (job.attribute(tag.c_str())) {
            if (found_one) {
              std::cerr << "THROWING\n";
              throw std::invalid_argument(
                      "Workflow::loadFromDAX(): multiple \"number of cores/procs\" specification for task " + id);
            } else {
              found_one = true;
              num_procs = std::stoi(job.attribute("num_procs").value());
            }
          }
        }


        // Create the task
        // If the DAX says num_procs = x, then we set min_cores=1, min_cores=x, efficiency=1.0
        task = this->addTask(id, flops, 1, num_procs, 1.0);

        // Go through the children "uses" nodes
        for (pugi::xml_node uses = job.child("uses"); uses; uses = uses.next_sibling("uses")) {
          // getMessage the "uses" attributes
          // TODO: There are several attributes that we're ignoring for now...
          std::string id = uses.attribute("file").value();

          double size = std::strtod(uses.attribute("size").value(), NULL);
          std::string link = uses.attribute("link").value();
          // Check whether the file already exists
          WorkflowFile *file = nullptr;

          try {
            file = this->getWorkflowFileByID(id);
          } catch (std::invalid_argument &e) {
            file = this->addFile(id, size);
          }
          if (link == "input") {
            task->addInputFile(file);
          }
          if (link == "output") {
            task->addOutputFile(file);
          }
          // TODO: Are there other types of "link" values?
        }
      }

      // Iterate through the "child" nodes to handle control dependencies
      for (pugi::xml_node child = dag.child("child"); child; child = child.next_sibling("child")) {

        WorkflowTask *child_task = this->getWorkflowTaskByID(child.attribute("ref").value());

        // Go through the children "parent" nodes
        for (pugi::xml_node parent = child.child("parent"); parent; parent = parent.next_sibling("parent")) {
          std::string parent_id = parent.attribute("ref").value();

          WorkflowTask *parent_task = this->getWorkflowTaskByID(parent_id);
          this->addControlDependency(parent_task, child_task);
        }
      }
    }

    /**
     * @brief Create a workflow based on a JSON file
     *
     * @param filename: the path to the JSON file
     *
     * @throw std::invalid_argument
     */
    void Workflow::loadFromJSON(const std::string &filename) {
      ///make workflow task
      wrench::WorkflowTask *task;

      /// read a JSON file
      std::ifstream file;
      nlohmann::json j;

      //handle the exceptions of opening the json file
      file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
      try {
        file.open(filename);
        file >> j;
      } catch (const std::ifstream::failure &e) {
        throw std::invalid_argument("Workflow::loadFromJson(): Invalid Json file");
      }

      nlohmann::json workflowJobs;
      try {
        workflowJobs = j.at("workflow");
      } catch (std::out_of_range &e) {
        std::cerr << "out of range" << '\n';
      }

      for (nlohmann::json::iterator it = workflowJobs.begin(); it != workflowJobs.end(); ++it) {
        if (it.key() == "jobs") {
          std::vector<nlohmann::json> jobs = it.value();

          for (auto &job : jobs) {
            std::string name = job.at("name");
            double flops = job.at("runtime");
            int num_procs = 1;
            task = this->addTask(name, flops, num_procs);
            std::vector<nlohmann::json> files = job.at("files");

            for (auto &f : files) {
              double size = f.at("size");
              std::string link = f.at("link");
              std::string id = f.at("name");
              wrench::WorkflowFile *file = nullptr;
              try {
                file = this->getWorkflowFileByID(id);
              } catch (const std::invalid_argument &ia) {
                // making a new file
                file = this->addFile(id, size);
              }
              if (link == "input") {
                task->addInputFile(file);
              }
              if (link == "output") {
                task->addOutputFile(file);
              }
            }
          }
        }
      }
      file.close();
    }

    /**
     * @brief Determine whether one source is an ancestor of a destination task
     *
     * @param src: the soure workflow task
     * @param dst: the destination task
     *
     * @return true if there is a path from src to dst, false otherwise
     */
    bool Workflow::pathExists(WorkflowTask *src, WorkflowTask *dst) {
      lemon::Bfs<lemon::ListDigraph> bfs(*DAG);

      bool reached = bfs.run(src->DAG_node, dst->DAG_node);
      return reached;
    }

    /**
     * @brief  Constructor
     */
    Workflow::Workflow() {
      DAG = std::unique_ptr<lemon::ListDigraph>(new lemon::ListDigraph());
      DAG_node_map = std::unique_ptr<lemon::ListDigraph::NodeMap<WorkflowTask *>>(
              new lemon::ListDigraph::NodeMap<WorkflowTask *>(*DAG));
      this->callback_mailbox = S4U_Mailbox::generateUniqueMailboxName("workflow_mailbox");
    };

    /**
     * @brief Get a vector of the ready tasks
     *
     * @return vector of workflow tasks
     */
    // TODO: Implement this more efficiently
    std::map<std::string, std::vector<WorkflowTask *>> Workflow::getReadyTasks() {

      std::map<std::string, std::vector<WorkflowTask *>> task_map;

      for (auto &it : this->tasks) {
        WorkflowTask *task = it.second.get();

        if (task->getState() == WorkflowTask::READY) {

          if (task->getClusterId().empty()) {
            task_map[task->getId()] = {task};

          } else {
            if (task_map.find(task->getClusterId()) == task_map.end()) {
              task_map[task->getClusterId()] = {task};
            } else {
              // add to clustered task
              task_map[task->getClusterId()].push_back(task);
            }
          }
        } else {
          if (task_map.find(task->getClusterId()) != task_map.end()) {
            if (task->getState() == WorkflowTask::NOT_READY) {
              task->setState(WorkflowTask::READY);
            }
            task_map[task->getClusterId()].push_back(task);
          }
        }
      }
      return task_map;
    }

    /**
     * @brief Check whether all tasks are complete
     *
     * @return true or false
     */
    bool Workflow::isDone() {
      for (auto &it : this->tasks) {
        WorkflowTask *task = it.second.get();
        if (task->getState() != WorkflowTask::COMPLETED) {
          return false;
        }
      }
      return true;
    }

    /**
     * @brief Get a list of all tasks in the workflow
     *
     * @return a vector of workflow tasks
     */
    std::vector<WorkflowTask *> Workflow::getTasks() {
      std::vector<WorkflowTask *> all_tasks;
      for (auto &it : this->tasks) {
        all_tasks.push_back(it.second.get());
      }
      return all_tasks;
    };

    /**
     * @brief Get a list of all files in the workflow
     *
     * @return a vector of workflow files
     */
    std::vector<WorkflowFile *> Workflow::getFiles() {
      std::vector<WorkflowFile *> all_files;
      for (auto &it : this->files) {
        all_files.push_back(it.second.get());
      }
      return all_files;
    };

    /**
     * @brief Get list of children for a task
     *
     * @param task: a workflow task
     *
     * @return a vector of workflow tasks
     */
    std::vector<WorkflowTask *> Workflow::getTaskChildren(const WorkflowTask *task) {
      if (task == nullptr) {
        throw std::invalid_argument("Workflow::getTaskChildren(): Invalid arguments");
      }
      std::vector<WorkflowTask *> children;
      for (lemon::ListDigraph::OutArcIt a(*DAG, task->DAG_node); a != lemon::INVALID; ++a) {
        children.push_back((*DAG_node_map)[(*DAG).target(a)]);
      }
      return children;
    }

    /**
     * @brief Get list of parents for a task
     *
     * @param task: a workflow task
     *
     * @return a vector of workflow tasks
     */
    std::vector<WorkflowTask *> Workflow::getTaskParents(const WorkflowTask *task) {
      if (task == nullptr) {
        throw std::invalid_argument("Workflow::getTaskParents(): Invalid arguments");
      }
      std::vector<WorkflowTask *> parents;
      for (lemon::ListDigraph::InArcIt a(*DAG, task->DAG_node); a != lemon::INVALID; ++a) {
        parents.push_back((*DAG_node_map)[(*DAG).source(a)]);
      }
      return parents;
    }

    /**
     * @brief Wait for the next worklow execution event
     *
     * @return a workflow execution event
     */
    std::unique_ptr<WorkflowExecutionEvent> Workflow::waitForNextExecutionEvent() {
      return WorkflowExecutionEvent::waitForNextExecutionEvent(this->callback_mailbox);
    }

    /**
     * @brief Get the mailbox name associated to this workflow
     *
     * @return the mailbox name
     */
    std::string Workflow::getCallbackMailbox() {
      return this->callback_mailbox;
    }

    /**
     * @brief Update the state of a task, and propagate the change
     *        to other tasks if necessary. WARNING: This method
     *        doesn't do ANY CHECK about whether the state change makes sense
     *
     * @param task: a workflow task
     * @param state: the new task state
     *
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    void Workflow::updateTaskState(WorkflowTask *task, WorkflowTask::State state) {
      if (task == nullptr) {
        throw std::invalid_argument("Workflow::updateTaskState(): Invalid arguments");
      }

      if (task->getState() == state) {
        return;
      }

//      WRENCH_INFO("Changing state of task %s from '%s' to '%s'",
//                  task->getId().c_str(),
//                  WorkflowTask::stateToString(task->state).c_str(),
//                  WorkflowTask::stateToString(state).c_str());

      switch (state) {
        // Make a task completed, which may failure_cause its children to become ready
        case WorkflowTask::COMPLETED: {
          task->setState(WorkflowTask::COMPLETED);

          // Go through the children and make them ready if possible
          for (lemon::ListDigraph::OutArcIt a(*DAG, task->DAG_node); a != lemon::INVALID; ++a) {
            WorkflowTask *child = (*DAG_node_map)[(*DAG).target(a)];
            updateTaskState(child, WorkflowTask::READY);
          }
          break;
        }
        case WorkflowTask::READY: {
          // Go through the parent and check whether they are all completed
          for (lemon::ListDigraph::InArcIt a(*DAG, task->DAG_node); a != lemon::INVALID; ++a) {
            WorkflowTask *parent = (*DAG_node_map)[(*DAG).source(a)];
            if (parent->getState() != WorkflowTask::COMPLETED) {
              // At least one parent is not in the COMPLETED state
              return;
            }
          }
          task->setState(WorkflowTask::READY);

          break;
        }
        case WorkflowTask::RUNNING: {
          task->setState(WorkflowTask::RUNNING);
          break;
        }
        case WorkflowTask::NOT_READY: {
          task->setState(WorkflowTask::NOT_READY);
          break;
        }
        case WorkflowTask::FAILED: {
          task->setState(WorkflowTask::FAILED);
          break;
        }
        default: {
          throw std::invalid_argument("Workflow::updateTaskState(): Unknown task state '" +
                                      std::to_string(state) + "'");
        }
      }
    }

    /**
     * @brief Retrieve a map (indexed by file id) of input files for a workflow (i.e., those files
     *        that are input to some tasks but output from none)
     *
     * @return a std::map of files
     */
    std::map<std::string, WorkflowFile *> Workflow::getInputFiles() {
      std::map<std::string, WorkflowFile *> input_files;
      for (auto const &x : this->files) {
        if ((x.second->output_of == nullptr) && (x.second->input_of.size() > 0)) {
          input_files.insert({x.first, x.second.get()});
        }
      }
      return input_files;
    }

    /**
     * @brief Retrieve a file by its id
     * @param id: the file id
     * @return the file, or nullptr if not found
     */
    WorkflowFile *Workflow::getFileById(const std::string id) {
      if (this->files.find(id) != this->files.end()) {
        return this->files[id].get();
      } else {
        return nullptr;
      }
    }

    /**
     * @brief Get the total number of flops for a list of tasks
     *
     * @param tasks: list of tasks
     *
     * @return the total number of flops
     */
    double Workflow::getSumFlops(std::vector<WorkflowTask *> tasks) {
      double total_flops = 0;
      for (auto it : tasks) {
        total_flops += (*it).getFlops();
      }
      return total_flops;
    }

};
