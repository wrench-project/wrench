/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <set>
#include <wrench-dev.h>
#include "wrench/workflow/Workflow.h"
#include "wrench/workflow/job/StandardJob.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(standard_job, "Log category for StandardJob");


namespace wrench {

    /**
     * @brief Constructor
     *
     * @param workflow: the workflow for which this job is
     *
     * @param tasks: the tasks in the job (which must be either READY, or children of COMPLETED tasks or
     *                                   of tasks also included in the standard job)
     * @param file_locations: a map that specifies on which storage service input/output files should be read/written
     *         (default storage is used otherwise, provided that the job is submitted to a compute service
     *          for which that default was specified)
     * @param pre_file_copies: a set of tuples that specify which file copy operations should be completed
     *                         before task executions begin
     * @param post_file_copies: a set of tuples that specify which file copy operations should be completed
     *                         after task executions end
     * @param cleanup_file_deletions: a set of tuples that specify which file copies should be removed from which
     *                         storage service. This will happen regardless of whether the job succeeds or fails
     *
     * @throw std::invalid_argument
     */
    StandardJob::StandardJob(Workflow *workflow, std::vector<WorkflowTask *> tasks,
                             std::map<WorkflowFile *, std::shared_ptr<StorageService> > &file_locations,
                             std::set<std::tuple<WorkflowFile *, std::shared_ptr<StorageService>, std::shared_ptr<StorageService>  >> &pre_file_copies,
                             std::set<std::tuple<WorkflowFile *, std::shared_ptr<StorageService>, std::shared_ptr<StorageService>  >> &post_file_copies,
                             std::set<std::tuple<WorkflowFile *, std::shared_ptr<StorageService>  >> &cleanup_file_deletions)
            :
            WorkflowJob(WorkflowJob::STANDARD),
            num_completed_tasks(0),
            file_locations(file_locations),
            pre_file_copies(pre_file_copies),
            post_file_copies(post_file_copies),
            cleanup_file_deletions(cleanup_file_deletions),
            state(StandardJob::State::NOT_SUBMITTED) {

      // Check that this is a ready sub-graph
      for (auto t : tasks) {
        if (t->getState() != WorkflowTask::State::READY) {
          std::vector<WorkflowTask *> parents = t->getWorkflow()->getTaskParents(t);
          for (auto parent : parents) {
            if (parent->getState() != WorkflowTask::State::COMPLETED) {
              if (std::find(tasks.begin(), tasks.end(), parent) == tasks.end()) {
                throw std::invalid_argument("StandardJob::StandardJob(): Task '" + t->getID() +
                                            "' has non-completed parents not included in the job");
              }
            }
          }
        }
      }

      for (auto t : tasks) {
        this->tasks.push_back(t);
        t->setJob(this);
        this->total_flops += t->getFlops();
      }
      this->workflow = workflow;
      this->name = "standard_job_" + std::to_string(WorkflowJob::getNewUniqueNumber());
    };

    /**
     * @brief Returns the minimum number of cores required, over all tasks in the job (i.e., at least
     *        one task in the job cannot run if fewer cores than this minimum are available)
     * @return the number of cores
     */
    unsigned long StandardJob::getMinimumRequiredNumCores() {
      unsigned long min_num_cores = 1;
      for (auto t : tasks) {
        if (min_num_cores < t->getMinNumCores()) {
          min_num_cores = t->getMinNumCores();
        }
      }
      return min_num_cores;
    }

    /**
     * @brief Get the number of tasks in the job
     *
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
     *
     * @return the number of completed tasks
     */
    unsigned long StandardJob::getNumCompletedTasks() {
      return this->num_completed_tasks;
    }

    /**
     * @brief Get the workflow tasks in the job
     *
     * @return a vector of workflow tasks
     */
    std::vector<WorkflowTask *> StandardJob::getTasks() {
      return this->tasks;
    }

    /**
     * @brief Get the file location map for the job
     *
     * @return a map of files to storage services
     */
    std::map<WorkflowFile *, std::shared_ptr<StorageService> > StandardJob::getFileLocations() {
      return this->file_locations;
    }

    /**
     * @brief Get the workflow priority value (the maximum priority from all tasks)
     *
     * @return the job priority value
     */
    unsigned long StandardJob::getPriority() {
      unsigned long max_priority = 0;
      for (auto task : tasks) {
        if (task->getPriority() > max_priority) {
          max_priority = task->getPriority();
        }
      }
      return max_priority;
    }

    /**
     * @brief Get the state of the standard job
     * @return the state
     */
    StandardJob::State StandardJob::getState() {
      return this->state;
    }

};
