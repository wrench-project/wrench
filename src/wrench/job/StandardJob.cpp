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
#include <wrench/workflow/Workflow.h>
#include <wrench/job/StandardJob.h>

WRENCH_LOG_CATEGORY(wrench_core_standard_job, "Log category for StandardJob");


namespace wrench {

    /**
     * @brief Constructor
     *
     * @param workflow: the workflow for which this job is
     *
     * @param tasks: the tasks in the job (which must be either READY, or children of COMPLETED tasks or
     *                                   of tasks also included in the standard job)
     * @param file_locations: a map that specifies locations where input/output files should be read/written
     * @param pre_file_copies: a vector of tuples that specify which file copy operations should be completed
     *                         before task executions begin
     * @param post_file_copies: a vector of tuples that specify which file copy operations should be completed
     *                         after task executions end
     * @param cleanup_file_deletions: a vector of tuples that specify which file copies should be removed from which
     *                         locations. This will happen regardless of whether the job succeeds or fails
     *
     * @throw std::invalid_argument
     */
    StandardJob::StandardJob(Workflow *workflow, std::vector<WorkflowTask *> tasks,
                             std::map<WorkflowFile *, std::vector<std::shared_ptr<FileLocation>>> &file_locations,
                             std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>  >> &pre_file_copies,
                             std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>  >> &post_file_copies,
                             std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>  >> &cleanup_file_deletions)
            :
            Job("", nullptr),
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
        this->name = "standard_job_" + std::to_string(Job::getNewUniqueNumber());
    }

    /**
     * @brief Returns the minimum number of cores required to run the job (i.e., at least
     *        one task in the job cannot run if fewer cores than this minimum are available)
     * @return the number of cores
     */
    unsigned long StandardJob::getMinimumRequiredNumCores() {
        unsigned long max_min_num_cores = 1;
        for (auto t : tasks) {
            max_min_num_cores = std::max<unsigned long>(max_min_num_cores, t->getMinNumCores());
        }
        return max_min_num_cores;
    }

    /**
     * @brief Returns the minimum RAM capacity required to run the job (i.e., at least
     *        one task in the job cannot run if less ram than this minimum is available)
     * @return the number of cores
     */
    unsigned long StandardJob::getMinimumRequiredMemory() {
        unsigned long max_ram = 0;
        for (auto t : tasks) {
            max_ram = std::max<unsigned long>(max_ram, t->getMemoryRequirement());
        }
        return max_ram;
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
    std::map<WorkflowFile *, std::vector<std::shared_ptr<FileLocation>>> StandardJob::getFileLocations() {
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

    /**
     * @brief get the job's pre-overhead
     * @return a number o seconds
     */
    double StandardJob::getPreJobOverheadInSeconds() {
        return this->pre_overhead;
    }

    /**
    * @brief get the job's post-overhead
    * @return a number o seconds
    */
    double StandardJob::getPostJobOverheadInSeconds() {
        return this->post_overhead;
    }

    /**
    * @brief sets the job's pre-overhead
    * @param overhead: the overhead in seconds
    */
    void StandardJob::setPreJobOverheadInSeconds(double overhead) {
        this->pre_overhead = overhead;
    }

    /**
    * @brief sets the job's post-overhead
    * @param overhead: the overhead in seconds
    */
    void StandardJob::setPostJobOverheadInSeconds(double overhead) {
        this->post_overhead = overhead;
    }

}
