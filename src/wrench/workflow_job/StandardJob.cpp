/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <set>
#include "workflow/Workflow.h"
#include "StandardJob.h"

namespace wrench {


    /**
     * @brief Constructor
     * @param tasks: the tasks in the job, which should all be independent and in the READY state
     * @param file_locations: a map that specifies on which storage service input/output files should be read/written
     *         (default storage is used otherwise, provided that the job is submitted to a compute service
     *          for which that default was specified)
     *
     * @param pre_file_copies: a set of tuples that specify which file copy operations should be completed
     *                         before task executions begin
     *
     * @param post_file_copies: a set of tuples that specify which file copy operations should be completed
     *                         after task executions end
     *
     * @param cleanup_file_deletions: a set of tuples that specify which file copies should be removed from which
     *                         storage service. This will happen regardless of whether the job succeeds or fails
     *
     * @throw std::invalid_argument
     */
    StandardJob::StandardJob(std::vector<WorkflowTask *> tasks,
                             std::map<WorkflowFile *, StorageService *> file_locations,
                             std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> pre_file_copies,
                             std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> post_file_copies,
                             std::set<std::tuple<WorkflowFile *, StorageService *>> cleanup_file_deletions) {

      this->type = WorkflowJob::STANDARD;
      this->num_cores = 1;
      this->duration = 0.0;

      for (auto t : tasks) {
        if (t->getState() != WorkflowTask::READY) {
          throw std::invalid_argument("All tasks used to create a StandardJob must be READY");
        }
      }
      for (auto t : tasks) {
        this->tasks.push_back(t);
        t->setJob(this);
        this->duration += t->getFlops();
      }
      this->num_completed_tasks = 0;
      this->workflow = this->tasks[0]->getWorkflow();

      this->state = StandardJob::State::NOT_SUBMITTED;
      this->name = "standard_job_" + std::to_string(WorkflowJob::getNewUniqueNumber());

      this->file_locations = file_locations;
      this->pre_file_copies = pre_file_copies;
      this->post_file_copies = post_file_copies;
      this->cleanup_file_deletions = cleanup_file_deletions;

    };

    /**
     * @brief Get the number of tasks in the job
     * @return the number of tasks
     */
    unsigned long StandardJob::getNumTasks() {
      return this->tasks.size();
    }

    /**
     * @brief Increment "the number of completed tasks" counter
     */
    void StandardJob::incrementNumCompletedTasks() {
      this->num_completed_tasks++;
    }

    /**
     * @brief Get the number of completed tasks in the job
     * @return the number of completed tasks
     */
    unsigned long StandardJob::getNumCompletedTasks() {
      return this->num_completed_tasks;
    }


    /**
     * @brief Get the workflow tasks in the job
     * @return a vector of pointers to WorkflowTasks objects
     */
    std::vector<WorkflowTask *> StandardJob::getTasks() {
      return this->tasks;
    }


    /**
     * @brief Get the file location map for the job
     *
     * @return a map
     */
    std::map<WorkflowFile*, StorageService*> StandardJob::getFileLocations() {
      return this->file_locations;
    }

};