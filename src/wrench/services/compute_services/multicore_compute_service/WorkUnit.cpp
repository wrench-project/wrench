/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <workflow_job/StandardJob.h>
#include "WorkUnit.h"

namespace wrench {

    /**
    * @brief Constructor
    * @param job: the job on behalf of which this work's performed
    * @param children: the work units that depend on this one
    * @param num_pending_parents: the number of non-completed work units this work unit depends on
    * @param pre_file_copies: a set of file copy actions to perform first
    * @param tasks: a set of tasks to execute in sequence
    * @param file_locations: locations where tasks should read/write files
    * @param post_file_copies: a set of file copy actions to perform last
    */
    WorkUnit::WorkUnit(
            StandardJob *job,
            std::vector<WorkUnit *> children,
            long num_pending_parents,
            std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> pre_file_copies,
            std::vector<WorkflowTask *> tasks,
            std::map<WorkflowFile *, StorageService *> file_locations,
            std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> post_file_copies) {

      this->job = job;
      this->children = children;
      this->num_pending_parents = num_pending_parents;

      this->pre_file_copies = pre_file_copies;
      this->tasks = tasks;
      this->file_locations = file_locations;
      this->post_file_copies = post_file_copies;

    }

};