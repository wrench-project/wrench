/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/workflow/StandardJob.h>
#include "wrench/services/compute/standard_job_executor/Workunit.h"

namespace wrench {

    /**
    * @brief Constructor
    * @param pre_file_copies: a set of file copy actions to perform first
    * @param tasks: a set of tasks to execute in sequence
    * @param file_locations: locations where tasks should read/write files
    * @param post_file_copies: a set of file copy actions to perform after all tasks
    * @param cleanup_file_deletions: a set of file deletion actions to perform last
    */
    Workunit::Workunit(
            std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> pre_file_copies,
            std::vector<WorkflowTask *> tasks,
            std::map<WorkflowFile *, StorageService *> file_locations,
            std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> post_file_copies,
            std::set<std::tuple<WorkflowFile *, StorageService *>> cleanup_file_deletions) {

      this->num_pending_parents = 0;

      this->pre_file_copies = pre_file_copies;
      this->tasks = tasks;
      this->file_locations = file_locations;
      this->post_file_copies = post_file_copies;
      this->cleanup_file_deletions = cleanup_file_deletions;

    }

    /**
     * @brief Add a dependency between two work unit (does nothing
     *        if the dependency already exists)
     *
     * @param parent: the parent work unit
     * @param child: the child work unit
     *
     * @throw std::invalid_argument
     */
    void Workunit::addDependency(std::shared_ptr<Workunit> parent, std::shared_ptr<Workunit> child) {
      if ((parent == nullptr) || (child == nullptr)) {
        throw std::invalid_argument("Workunit::addDependency(): Invalid arguments");
      }

      // If dependency already exits, do nothing
      if (parent->children.find(child) != parent->children.end()) {
        return;
      }

      parent->children.insert(child);
      child->num_pending_parents++;
      return;
    }

    Workunit::~Workunit() {
      this->tasks.clear();
    }

};
