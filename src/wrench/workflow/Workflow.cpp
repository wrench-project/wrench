/**
 * Copyright (c) 2017-2018. The WRENCH Team.
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
#include <nlohmann/json.hpp>
#include <wrench/util/UnitParser.h>
#include <wrench/workflow/WorkflowTask.h>

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
     * @param max_num_cores: the maximum number of cores that can be used by the task (use INT_MAX for infinity)
     * @param parallel_efficiency: the multi-core parallel efficiency (number between 0.0 and 1.0)
     * @param memory_requirement: memory requirement (in bytes)
     * @param type: workflow task type (WorkflowTask::TaskType)
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
                                    double memory_requirement,
                                    const WorkflowTask::TaskType type) {


      if ((flops < 0.0) || (min_num_cores < 1) || (min_num_cores > max_num_cores) ||
          (parallel_efficiency < 0.0) || (parallel_efficiency > 1.0) || (memory_requirement < 0)) {
        throw std::invalid_argument("WorkflowTask::addTask(): Invalid argument");
      }

      // Check that the task doesn't really exist
      if (tasks.find(id) != tasks.end()) {
        throw std::invalid_argument("Workflow::addTask(): Task ID '" + id + "' already exists");
      }

      // Create the WorkflowTask object
      WorkflowTask *task = new WorkflowTask(id, flops, min_num_cores, max_num_cores, parallel_efficiency,
                                            memory_requirement, type);
      // Create a DAG node for it
      task->workflow = this;
      task->DAG = this->DAG.get();
      task->DAG_node = DAG->addNode();
      task->toplevel = 0; // upon creation, a task is an entry task

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

      // check that task exists (this should never happen)
      if (tasks.find(task->id) == tasks.end()) {
        throw std::invalid_argument("Workflow::removeTask(): Task '" + task->id + "' does not exist");
      }

      std::vector<wrench::WorkflowTask *> children = this->getTaskChildren(task);

      DAG.get()->erase(task->DAG_node);
      tasks.erase(tasks.find(task->id));

      // Brute-force a top-level update all all the children of the removed task
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
    WorkflowTask *Workflow::getTaskByID(const std::string id) {
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
     *
     * @throw std::invalid_argument
     */
    void Workflow::addControlDependency(WorkflowTask *src, WorkflowTask *dst) {

      if ((src == nullptr) || (dst == nullptr)) {
        throw std::invalid_argument("Workflow::addControlDependency(): Invalid arguments");
      }

      if (not pathExists(src, dst)) {

        WRENCH_DEBUG("Adding control dependency %s-->%s", src->getID().c_str(), dst->getID().c_str());
        DAG->addArc(src->DAG_node, dst->DAG_node);

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
    WorkflowFile *Workflow::getFileByID(const std::string id) {
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
//      typedef lemon::dim2::Point<int> Point;
//      lemon::ListDigraph::NodeMap<Point> coords(*DAG);

//      graphToEps(*DAG, eps_filename).
//              coords(coords).             // Doesn't compute coordinates!
//              run();
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
      lemon::Bfs<lemon::ListDigraph> bfs(*DAG);

      bool reached = bfs.run(src->DAG_node, dst->DAG_node);
      return reached;
    }

    /**
     * @brief  Constructor
     */
    Workflow::Workflow() {
      this->DAG = std::unique_ptr<lemon::ListDigraph>(new lemon::ListDigraph());
      this->DAG_node_map = std::unique_ptr<lemon::ListDigraph::NodeMap<WorkflowTask *>>(
              new lemon::ListDigraph::NodeMap<WorkflowTask *>(*DAG));
      this->callback_mailbox = S4U_Mailbox::generateUniqueMailboxName("workflow_mailbox");
      this->simulation = nullptr;
    };

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
     * @return a vector of tasks
     */
    std::vector<WorkflowTask *> Workflow::getTasks() {
      std::vector<WorkflowTask *> all_tasks;
      for (auto &it : this->tasks) {
        all_tasks.push_back(it.second.get());
      }
      return all_tasks;
    };

    /**
     * @brief Get the list of all files in the workflow
     *
     * @return a vector of files
     */
    std::vector<WorkflowFile *> Workflow::getFiles() {
      std::vector<WorkflowFile *> all_files;
      for (auto &it : this->files) {
        all_files.push_back(it.second.get());
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
      std::vector<WorkflowTask *> children;
      for (lemon::ListDigraph::OutArcIt a(*DAG, task->DAG_node); a != lemon::INVALID; ++a) {
        children.push_back((*DAG_node_map)[(*DAG).target(a)]);
      }
      return children;
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

#if 0
    //    /**
    //     * @brief Update the state of a task, and propagate the change
    //     *        to other tasks if necessary. WARNING: This method
    //     *        doesn't do ANY CHECK about whether the state change makes sense
    //     *
    //     * @param task: a workflow task
    //     * @param state: the new task state
    //     *
    //     * @throw std::invalid_argument
    //     * @throw std::runtime_error
    //     */
    //    void Workflow::updateTaskState(WorkflowTask *task, WorkflowTask::State state) {
    //      if (task == nullptr) {
    //        throw std::invalid_argument("Workflow::updateTaskState(): Invalid arguments");
    //      }
    //
    //      if (task->getState() == state) {
    //        return;
    //      }
    //
    ////      WRENCH_INFO("Changing state of task %s from '%s' to '%s'",
    ////                  task->getID().c_str(),
    ////                  WorkflowTask::stateToString(task->state).c_str(),
    ////                  WorkflowTask::stateToString(state).c_str());
    //
    //      switch (state) {
    //        // Make a task completed, which may failure_cause its children to become ready
    //        case WorkflowTask::COMPLETED: {
    //          task->setState(WorkflowTask::COMPLETED);
    //
    //          // Go through the children and make them ready if possible
    //          for (lemon::ListDigraph::OutArcIt a(*DAG, task->DAG_node); a != lemon::INVALID; ++a) {
    //            WorkflowTask *child = (*DAG_node_map)[(*DAG).target(a)];
    //            updateTaskState(child, WorkflowTask::READY);
    //          }
    //          break;
    //        }
    //        case WorkflowTask::READY: {
    //          // Go through the parent and check whether they are all completed
    //          for (lemon::ListDigraph::InArcIt a(*DAG, task->DAG_node); a != lemon::INVALID; ++a) {
    //            WorkflowTask *parent = (*DAG_node_map)[(*DAG).source(a)];
    //            if (parent->getState() != WorkflowTask::COMPLETED) {
    //              // At least one parent is not in the COMPLETED state
    //              return;
    //            }
    //          }
    //          task->setState(WorkflowTask::READY);
    //
    //          break;
    //        }
    //        case WorkflowTask::RUNNING: {
    //          task->setState(WorkflowTask::RUNNING);
    //          break;
    //        }
    //        case WorkflowTask::NOT_READY: {
    //          task->setState(WorkflowTask::NOT_READY);
    //          break;
    //        }
    //        case WorkflowTask::FAILED: {
    //          task->setState(WorkflowTask::FAILED);
    //          break;
    //        }
    //        default: {
    //          throw std::invalid_argument("Workflow::updateTaskState(): Unknown task state '" +
    //                                      std::to_string(state) + "'");
    //        }
    //      }
    //    }
#endif

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

    /**
     * @brief Create a workflow based on a DAX file
     *
     * @param filename: the path to the DAX file
     * @param reference_flop_rate: a reference compute speed (in flops/sec), assuming a task's computation is purely flops.
     *                             This is needed because DAX files specify task execution times in seconds,
     *                             but the WRENCH simulation needs some notion of "amount of computation" to
     *                             apply reasonable scaling. (Because the XML platform description specifies host
     *                             compute speeds in flops/sec). The times in the DAX file are thus asume to be obtained
     *                             on an machine with flop rate reference_flop_rate.
     *
     * @throw std::invalid_argument
     */
    void Workflow::loadFromDAX(const std::string &filename, const std::string &reference_flop_rate) {

      pugi::xml_document dax_tree;

      double flop_rate;

      try {
        flop_rate = UnitParser::parse_compute_speed(reference_flop_rate);
      } catch (std::invalid_argument &e) {
        throw;
      }

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
        double runtime = std::strtod(job.attribute("runtime").value(), NULL);
        int num_procs = 1;
        bool found_one = false;
        for (std::string tag : {"numprocs", "num_procs", "numcores", "num_cores"}) {
          if (job.attribute(tag.c_str())) {
            if (found_one) {
              throw std::invalid_argument(
                      "Workflow::loadFromDAX(): multiple \"number of cores/procs\" specification for task " + id);
            } else {
              found_one = true;
              num_procs = std::stoi(job.attribute(tag.c_str()).value());
            }
          }
        }


        // Create the task
        // If the DAX says num_procs = x, then we set min_cores=1, max_cores=x, efficiency=1.0
        task = this->addTask(id, runtime * flop_rate, 1, num_procs, 1.0, 0.0);

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
            file = this->getFileByID(id);
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

        WorkflowTask *child_task = this->getTaskByID(child.attribute("ref").value());

        // Go through the children "parent" nodes
        for (pugi::xml_node parent = child.child("parent"); parent; parent = parent.next_sibling("parent")) {
          std::string parent_id = parent.attribute("ref").value();

          WorkflowTask *parent_task = this->getTaskByID(parent_id);
          this->addControlDependency(parent_task, child_task);
        }
      }
    }

    /**
     * @brief Create a workflow based on a JSON file
     *
     * @param filename: the path to the JSON file
     * @param reference_flop_rate: a reference compute speed (in flops/sec), assuming a task's computation is purely flops.
     *                             This is needed because JSON files specify task execution times in seconds,
     *                             but the WRENCH simulation needs some notion of "amount of computation" to
     *                             apply reasonable scaling. (Because the XML platform description specifies host
     *                             compute speeds in flops/sec). The times in the JSON file are thus asume to be obtained
     *                             on an machine with flop rate reference_flop_rate.
     *
     * @throw std::invalid_argument
     */
    void Workflow::loadFromJSON(const std::string &filename, const std::string &reference_flop_rate) {

      std::ifstream file;
      nlohmann::json j;

      double flop_rate;

      try {
        flop_rate = UnitParser::parse_compute_speed(reference_flop_rate);
      } catch (std::invalid_argument &e) {
        throw;
      }

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
        throw std::invalid_argument("Workflow::loadFromJson(): Could not find a workflow entry");
      }

      wrench::WorkflowTask *task;

      for (nlohmann::json::iterator it = workflowJobs.begin(); it != workflowJobs.end(); ++it) {
        if (it.key() == "jobs") {
          std::vector<nlohmann::json> jobs = it.value();

          for (auto &job : jobs) {
            std::string name = job.at("name");
            double runtime = job.at("runtime");
            unsigned long num_procs = 1;
            task = this->addTask(name, runtime * flop_rate, num_procs, num_procs, 1.0, 0.0);

            // task priority
            try {
              task->setPriority(job.at("priority"));
            } catch (nlohmann::json::out_of_range &e) {
              // do nothing
            }

            // task type
            std::string type = job.at("type");
            if (type == "transfer") {
              task->setTaskType(WorkflowTask::TaskType::TRANSFER);
            } else if (type == "auxiliary") {
              task->setTaskType(WorkflowTask::TaskType::AUXILIARY);
            }

            // task files
            std::vector<nlohmann::json> files = job.at("files");

            for (auto &f : files) {
              double size = f.at("size");
              std::string link = f.at("link");
              std::string id = f.at("name");
              wrench::WorkflowFile *workflow_file = nullptr;
              try {
                workflow_file = this->getFileByID(id);
              } catch (const std::invalid_argument &ia) {
                // making a new file
                workflow_file = this->addFile(id, size);
              }
              if (link == "input") {
                task->addInputFile(workflow_file);
              } else if (link == "output") {
                task->addOutputFile(workflow_file);
              }
              if (type == "transfer") {
                task->addSrcDest(workflow_file, f.at("src"), f.at("dest"));
              }
            }
          }

          // since tasks may not be ordered in the JSON file, we need to iterate over all tasks again
          for (auto &job : jobs) {
            task = this->getTaskByID(job.at("name"));
            std::vector<nlohmann::json> parents = job.at("parents");
            // task dependencies
            for (auto &parent : parents) {
              try {
                WorkflowTask *parent_task = this->getTaskByID(parent);
                this->addControlDependency(parent_task, task);
              } catch (std::invalid_argument &e) {
                // do nothing
              }
            }
          }
        }
      }
      file.close();
    }

    /**
     * @brief Returns all tasks with top-levels in a range
     * @param min: the low end of the range (inclusive)
     * @param max: the high end of the range (exclusive)
     * @return a vector of tasks
     */
    std::vector<WorkflowTask *> Workflow::getTasksInTopLevelRange(unsigned long min, unsigned long max) {
      std::vector<WorkflowTask *> to_return;
      for (auto t : this->getTasks()) {
        if ((t->getTopLevel() >= min) and (t->getTopLevel() <= max)) {
          to_return.push_back(t);
        }
      }
      return to_return;
    }

    /**
     * @brief Returns the number of levels in the workflow
     * @return the number of levels
     */
    unsigned long Workflow::getNumLevels() {
      unsigned long max_top_level = 0;
      for (auto const &t : this->tasks) {
        wrench::WorkflowTask *task = t.second.get();
        if (task->getNumberOfChildren() == 0) {
          if (1 + task->getTopLevel() > max_top_level) {
            max_top_level = 1 + task->getTopLevel();
          }
        }
      }
      return max_top_level;
    }


//    /**
//     * @brief Sets the number of levels in the workflow
//     * @param num_levels: the number of levels
//     */
//    void Workflow::setNumLevels(unsigned long num_levels) {
//      this->num_levels = num_levels;
//    }

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

};
