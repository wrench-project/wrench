/**
 * Copyright (c) 2017-2021. The WRENCH Team.
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
     * @param job_manager: the job manager that creates this job
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
    StandardJob::StandardJob(std::shared_ptr<JobManager> job_manager,
                             std::vector<WorkflowTask *> tasks,
                             std::map<WorkflowFile *, std::vector<std::shared_ptr<FileLocation>>> &file_locations,
                             std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>  >> &pre_file_copies,
                             std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>  >> &post_file_copies,
                             std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>  >> &cleanup_file_deletions)
            :
            Job("", std::move(job_manager)),
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
//        this->workflow = workflow;
        this->name = "standard_job_" + std::to_string(Job::getNewUniqueNumber());

    }

    /**
     * @brief Returns the minimum number of cores required to run the job (i.e., at least
     *        one task in the job cannot run if fewer cores than this minimum are available)
     * @return the number of cores
     */
    unsigned long StandardJob::getMinimumRequiredNumCores() const {
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
    unsigned long StandardJob::getMinimumRequiredMemory() const {
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
    unsigned long StandardJob::getNumTasks() const {
        return this->tasks.size();
    }

    /**
     * @brief Get the number of completed tasks in the job
     *
     * @return the number of completed tasks
     */
    unsigned long StandardJob::getNumCompletedTasks() const {
        unsigned long count = 0;
        for (auto const &t : this->tasks) {
            if (t->getState() == WorkflowTask::State::COMPLETED) {
                count++;
            }
        }
        return count;
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

    /**
     * @brief Instantiate a compound job
     */
    void StandardJob::createUnderlyingCompoundJob(const std::shared_ptr<ComputeService>& compute_service) {

        this->compound_job = nullptr;
        this->pre_overhead_action = nullptr;
        this->post_overhead_action = nullptr;
        this->pre_file_copy_actions.clear();
        this->task_file_read_actions.clear();
        this->task_compute_actions.clear();
        this->task_file_write_actions.clear();
        this->post_file_copy_actions.clear();
        this->cleanup_actions.clear();
        this->scratch_cleanup = nullptr;

        auto cjob = this->job_manager->createCompoundJob("cjob_for_" + this->getName());

        // Create pre- and post-overhead work units
        if (this->getPreJobOverheadInSeconds() > 0.0) {
            pre_overhead_action = cjob->addSleepAction("", this->getPreJobOverheadInSeconds());
        }

        if (this->getPostJobOverheadInSeconds() > 0.0) {
            post_overhead_action = cjob->addSleepAction("", this->getPostJobOverheadInSeconds());
        }

        // Create the pre- file copy actions
        for (auto const &pfc : this->pre_file_copies) {
            auto src_location = std::get<1>(pfc);
            auto dst_location = std::get<2>(pfc);
            if (src_location == FileLocation::SCRATCH) {
                src_location = FileLocation::LOCATION(compute_service->getScratch());
            }
            if (dst_location == FileLocation::SCRATCH) {
                dst_location = FileLocation::LOCATION(compute_service->getScratch());
            }

            pre_file_copy_actions.push_back(cjob->addFileCopyAction("", std::get<0>(pfc), src_location, dst_location));
        }

        // Create the post- file copy actions
        for (auto const &pfc : this->post_file_copies) {
            auto src_location = std::get<1>(pfc);
            auto dst_location = std::get<2>(pfc);
            if (src_location == FileLocation::SCRATCH) {
                src_location = FileLocation::LOCATION(compute_service->getScratch());
            }
            if (dst_location == FileLocation::SCRATCH) {
                dst_location = FileLocation::LOCATION(compute_service->getScratch());
            }

            post_file_copy_actions.push_back(cjob->addFileCopyAction("", std::get<0>(pfc), src_location, dst_location));
        }

        // Create the file cleanup actions
        for (auto const &fc: this->cleanup_file_deletions) {
            auto target_location = std::get<1>(fc);
            if (target_location == FileLocation::SCRATCH) {
                target_location = FileLocation::LOCATION(compute_service->getScratch());
            }
            cleanup_actions.push_back(cjob->addFileDeleteAction("", std::get<0>(fc), target_location));
        }

        // Create the task actions
        for (auto const &task : this->tasks) {
            auto compute_action = cjob->addComputeAction("task_" + task->getID(), task->getFlops(), task->getMemoryRequirement(),
                                                         task->getMinNumCores(), task->getMaxNumCores(), task->getParallelModel());
            task_compute_actions[task] = compute_action;
            task_file_read_actions[task] = {};
            for (auto const &f : task->getInputFiles()) {
                std::shared_ptr<Action> fread_action;
                if (this->file_locations.find(f) != this->file_locations.end()) {
                    std::vector<std::shared_ptr<FileLocation>> fixed_locations;
                    for (auto const &loc : this->file_locations[f]) {
                        if (loc == FileLocation::SCRATCH) {
                            fixed_locations.push_back(FileLocation::LOCATION(compute_service->getScratch()));
                        } else {
                            fixed_locations.push_back(loc);
                        }
                    }
                    fread_action = cjob->addFileReadAction("", f, fixed_locations);
                } else {
                    fread_action = cjob->addFileReadAction("", f, FileLocation::LOCATION(compute_service->getScratch()));
                }
                task_file_read_actions[task].push_back(fread_action);
                cjob->addActionDependency(fread_action, compute_action);
            }
            task_file_write_actions[task] = {};
            for (auto const &f : task->getOutputFiles()) {
                std::shared_ptr<Action> fwrite_action;
                if (this->file_locations.find(f) != this->file_locations.end()) {
                    std::vector<std::shared_ptr<FileLocation>> fixed_locations;
                    for (auto const &loc : this->file_locations[f]) {
                        if (loc == FileLocation::SCRATCH) {
                            fixed_locations.push_back(FileLocation::LOCATION(compute_service->getScratch()));
                        } else {
                            fixed_locations.push_back(loc);
                        }
                    }
                    fwrite_action = cjob->addFileWriteAction("", f, fixed_locations.at(0));  // TODO: The at(0) here is ok? I mean, what does it mean to write to multiple locations....
                } else {
                    fwrite_action = cjob->addFileWriteAction("", f, FileLocation::LOCATION(compute_service->getScratch()));
                }
                task_file_write_actions[task].push_back(fwrite_action);
                cjob->addActionDependency(compute_action, fwrite_action);
            }
        }

        // TODO replace this horror with some sjob function perhaps?
        bool need_scratch_clean = false;
        for (auto const &task : this->tasks) {
            for (auto const &f : task->getInputFiles()) {
                if (this->file_locations.find(f) == this->file_locations.end()) {
                    need_scratch_clean = true;
                    break;
                }
            }
            if (need_scratch_clean) {
                break;
            }
            for (auto const &f : task->getOutputFiles()) {
                if (this->file_locations.find(f) == this->file_locations.end()) {
                    need_scratch_clean = true;
                    break;
                }
            }
            if (need_scratch_clean) {
                break;
            }
        }

        // Create the scratch clean up actions
        if (need_scratch_clean) {
            // Does the lambda capture of cjob_file_locations work?
            auto lambda_execute = [this](const std::shared_ptr<wrench::ActionExecutor> &action_executor) {
                for (auto const &task : this->tasks) {
                    for (auto const &f : task->getInputFiles()) {
                        if (this->file_locations.find(f) == this->file_locations.end()) {
                            try {
                                auto scratch = this->getParentComputeService()->getScratch();
                                scratch->deleteFile(f, FileLocation::LOCATION(scratch));
                            } catch (ExecutionException &ignore) {
                                // ignore
                            }
                        }
                    }
                    for (auto const &f : task->getOutputFiles()) {
                        if (this->file_locations.find(f) == this->file_locations.end()) {
                            try {
                                auto scratch = this->getParentComputeService()->getScratch();
                                scratch->deleteFile(f, FileLocation::LOCATION(scratch));
                            } catch (ExecutionException &ignore) {
                                // ignore
                            }
                        }
                    }
                }
            };
            auto lambda_terminate = [](const std::shared_ptr<wrench::ActionExecutor> &action_executor) {};
            scratch_cleanup = cjob->addCustomAction("", lambda_execute, lambda_terminate);

        }

        // Add all inter-task dependencies
        for (auto const &parent_task : this->tasks) {
            for (auto const &child_task : parent_task->getChildren()) {
                if (task_compute_actions.find(child_task) == task_compute_actions.end()) {
                    continue;
                }
                std::vector<std::shared_ptr<Action>> parent_actions;
                if (not task_file_write_actions[parent_task].empty()) {
                    parent_actions = task_file_write_actions[parent_task];
                } else {
                    parent_actions = {task_compute_actions[parent_task]};
                }
                std::vector<std::shared_ptr<Action>> child_actions;
                if (not task_file_read_actions[child_task].empty()) {
                    child_actions = task_file_read_actions[child_task];
                } else {
                    child_actions = {task_compute_actions[child_task]};
                }
                for (auto const &parent_action : parent_actions) {
                    for (auto const &child_action: child_actions) {
                        cjob->addActionDependency(parent_action, child_action);
                    }
                }
            }
        }

        // Create dummy tasks
        std::shared_ptr<Action> pre_overhead_to_pre_file_copies = cjob->addSleepAction("", 0);
        std::shared_ptr<Action> pre_file_copies_to_tasks = cjob->addSleepAction("", 0);
        std::shared_ptr<Action> tasks_to_post_file_copies = cjob->addSleepAction("", 0);
        std::shared_ptr<Action> tasks_post_file_copies_to_cleanup = cjob->addSleepAction("", 0);
        std::shared_ptr<Action> cleanup_to_post_overhead = cjob->addSleepAction("", 0);
        cjob->addActionDependency(pre_overhead_to_pre_file_copies,pre_file_copies_to_tasks);
        cjob->addActionDependency(pre_file_copies_to_tasks, tasks_to_post_file_copies);
        cjob->addActionDependency(tasks_to_post_file_copies, tasks_post_file_copies_to_cleanup);
        cjob->addActionDependency(tasks_post_file_copies_to_cleanup, cleanup_to_post_overhead);

        // Add all dependencies, using the dummy tasks to help
        if (pre_overhead_action != nullptr) {
            cjob->addActionDependency(pre_overhead_action, pre_overhead_to_pre_file_copies);
        }
        if (not pre_file_copy_actions.empty()) {
            for (auto const &pfca : pre_file_copy_actions) {
                cjob->addActionDependency(pre_overhead_to_pre_file_copies, pfca);
                cjob->addActionDependency(pfca, pre_file_copies_to_tasks);
            }
        }

        if (not task_compute_actions.empty()) {
            for (auto const &tca : task_compute_actions) {
                WorkflowTask *task = tca.first;
                if (not task_file_read_actions[task].empty()) {
                    for (auto const &tfra : task_file_read_actions[task]) {
                        cjob->addActionDependency(pre_file_copies_to_tasks, tfra);
                    }
                } else {
                    cjob->addActionDependency(pre_file_copies_to_tasks, tca.second);
                }
                if (not task_file_write_actions[task].empty()) {
                    for (auto const &tfwa : task_file_write_actions[task]) {
                        cjob->addActionDependency(tfwa, tasks_to_post_file_copies);
                    }
                } else {
                    cjob->addActionDependency(tca.second, tasks_to_post_file_copies);
                }
            }
        }

        if (not post_file_copy_actions.empty()) {
            for (auto const &pfca : post_file_copy_actions) {
                cjob->addActionDependency(tasks_to_post_file_copies, pfca);
                cjob->addActionDependency(pfca, tasks_post_file_copies_to_cleanup);
            }
        }

        if (not cleanup_actions.empty()) {
            for (auto const &ca : cleanup_actions) {
                cjob->addActionDependency(tasks_post_file_copies_to_cleanup, ca);
                cjob->addActionDependency(ca, cleanup_to_post_overhead);
            }
        }

        if (post_overhead_action != nullptr) {
            cjob->addActionDependency(cleanup_to_post_overhead, post_overhead_action);
            if (scratch_cleanup != nullptr) {
                cjob->addActionDependency(post_overhead_action, scratch_cleanup);
            }
        } else {
            if (scratch_cleanup != nullptr) {
                cjob->addActionDependency(cleanup_to_post_overhead, scratch_cleanup);
            }
        }

        // Use the dummy tasks for "easy" dependencies and remove the dummies
        std::vector<std::shared_ptr<Action>> dummies = {pre_overhead_to_pre_file_copies, pre_file_copies_to_tasks, tasks_to_post_file_copies, tasks_post_file_copies_to_cleanup, cleanup_to_post_overhead};
        for (auto &dummy : dummies) {
            // propagate dependencies
            for (auto const &parent_action : dummy->getParents()) {
                for (auto const &child_action : dummy->getChildren()) {
                    cjob->addActionDependency(parent_action, child_action);
                }
            }
            // remove the dummy
            cjob->removeAction(dummy);
        }

//            cjob->printActionDependencies();
        this->compound_job = cjob;
    }

    void StandardJob::analyzeActions(std::vector<std::shared_ptr<Action>> actions,
                                     bool *at_least_one_failed,
                                     bool *at_least_one_killed,
                                     std::shared_ptr<FailureCause> *failure_cause,
                                     double *earliest_start_date,
                                     double *latest_end_date,
                                     double *earliest_failure_date) {

        *at_least_one_failed = false;
        *at_least_one_killed = false;
        *failure_cause = nullptr;
        *earliest_start_date = -1.0;
        *latest_end_date = -1.0;
        *earliest_failure_date = -1.0;

        for (const auto &action : actions) {
//            std::cerr << "   ANALYSING ACTION " << action->getName() << "    END DATE " << action->getEndDate() << "\n";

            // Set the dates
            if ((*earliest_start_date == -1.0) or ((action->getStartDate() < *earliest_start_date) and (action->getStartDate() != -1.0))) {
                *earliest_start_date = action->getStartDate();
            }
            if ((*latest_end_date == -1.0) or ((action->getEndDate() > *latest_end_date) and (action->getEndDate() != -1.0))) {
                *latest_end_date = action->getEndDate();
            }

            if (action->getState() == Action::State::FAILED || action->getState() == Action::State::KILLED) {
                *at_least_one_failed = true;
                if (not *failure_cause) {
                    *failure_cause = action->getFailureCause();
                }
                if ((*earliest_failure_date == -1.0) or
                    ((*earliest_failure_date > action->getEndDate()) and (action->getEndDate() != -1.0))) {
                    *earliest_failure_date = action->getEndDate();
                }
            }
        }
    }

    /**
     * @brief Compute all task updates based on the state of the underlying compound job (also updates timing information and other task information)
     * @param necessary_state_changes: the set of task state changes to apply
     * @param necessary_failure_count_increments: the set ot task failure count increments to apply
     * @param job_failure_cause: the job failure cause, if any
     * @param simulation: the simulation (to add timestamps!)
     */
    void StandardJob::processCompoundJobOutcome(std::map<WorkflowTask *, WorkflowTask::State> &state_changes,
                                                std::set<WorkflowTask *> &failure_count_increments,
                                                std::shared_ptr<FailureCause> &job_failure_cause,
                                                Simulation *simulation) {
        switch(this->state) {
            case StandardJob::State::PENDING:
            case StandardJob::State::RUNNING:
                throw std::runtime_error("StandardJob::processCompoundJobOutcome(): Cannot be called on a RUNNING/PENDING job");
            default:
                break;
        }


        // At this point all tasks are pending, so no matter what we need to change all states
        // So we provisionally make them all NOT_READY right now, which we may overwrite with
        // COMPLETED, and then and the level above may turn some of the NOT_READY into READY.
        for (auto const &t : this->tasks) {
            state_changes[t] = WorkflowTask::State::NOT_READY;
        }

        job_failure_cause = nullptr;

//        for (auto const &a: this->compound_job->getActions()) {
//            std::cerr << "ACTION " << a->getName() << ": STATE=" << Action::stateToString(a->getState()) << "\n";
//            if (a->getFailureCause()) {
//                std::cerr << "ACTION " << a->getName() << ": " << a->getFailureCause()->toString() << "\n";
//            } else {
//                std::cerr << "ACTION " << a->getName() << ": NO FAILURE CAUSE\n";
//            }
//        }

        /*
         * Look at Overhead action
         */
        if (this->pre_overhead_action) {
            if (this->pre_overhead_action->getState() == Action::State::KILLED) {
                job_failure_cause = std::make_shared<JobKilled>(this->shared_this);
                for (auto const &t : this->tasks) {
                    failure_count_increments.insert(t);
                }
                return;
            } else if (this->pre_overhead_action->getState() == Action::State::FAILED) {
                job_failure_cause = this->pre_overhead_action->getFailureCause();
                for (auto const &t : this->tasks) {
                    failure_count_increments.insert(t);
                }
                return;
            }
        }

        /*
         * Look at Pre-File copy actions
         */
        if (not this->pre_file_copy_actions.empty()) {
            bool at_least_one_failed, at_least_one_killed;
            std::shared_ptr<FailureCause> failure_cause;
            double earliest_start_date, latest_end_date, earliest_failure_date;
            this->analyzeActions(this->pre_file_copy_actions,
                                 &at_least_one_failed,
                                 &at_least_one_killed,
                                 &failure_cause,
                                 &earliest_start_date,
                                 &latest_end_date,
                                 &earliest_failure_date);
            if (at_least_one_killed) {
                job_failure_cause = std::make_shared<JobKilled>(this->shared_this);
                for (auto const &t : this->tasks) {
                    failure_count_increments.insert(t);
                }
                return;
            } else if (at_least_one_failed) {
                job_failure_cause = failure_cause;
                for (auto const &t : this->tasks) {
                    failure_count_increments.insert(t);
                }
                return;
            }

        }

        /*
         * Look at all the tasks
         */
        for (auto &t: this->tasks) {
            // Set a provisional start date
            t->setStartDate(-1.0);
            // Set the execution host
            t->setExecutionHost(this->task_compute_actions[t]->getExecutionHistory().top().execution_host);

            /*
             * Look at file-read actions
             */

            bool at_least_one_failed, at_least_one_killed;
            std::shared_ptr<FailureCause> failure_cause;
            double earliest_start_date, latest_end_date, earliest_failure_date;
            this->analyzeActions(this->task_file_read_actions[t],
                                 &at_least_one_failed,
                                 &at_least_one_killed,
                                 &failure_cause,
                                 &earliest_start_date,
                                 &latest_end_date,
                                 &earliest_failure_date);

            if (at_least_one_failed or at_least_one_killed) {
                t->updateStartDate(earliest_start_date);
                t->setReadInputStartDate(earliest_start_date);
                state_changes[t] = WorkflowTask::State::READY; // This may be changed to NOT_READY later based on other tasks
                if (at_least_one_killed) {
                    job_failure_cause = std::make_shared<JobKilled>(this->shared_this);
                    failure_count_increments.insert(t);
                    t->setFailureDate(earliest_failure_date);
                } else if (at_least_one_failed) {
                    t->setFailureDate(earliest_failure_date);
                    failure_count_increments.insert(t);
                    if (job_failure_cause == nullptr) job_failure_cause = failure_cause;
                }
                continue;
            }

            t->updateStartDate(earliest_start_date); // could be -1.0 if there were no input, but will be updated below
            simulation->getOutput().addTimestampTaskStart(latest_end_date, t);
            t->setReadInputStartDate(earliest_start_date);
            t->setReadInputEndDate(latest_end_date);

            /*
             * Look at the compute action
             */
            // Look at the compute action
            auto compute_action = this->task_compute_actions[t];
            t->setComputationStartDate(compute_action->getStartDate());  // could be -1.0
            t->setComputationEndDate(compute_action->getEndDate());      // could be -1.0
            t->setNumCoresAllocated(compute_action->getExecutionHistory().top().num_cores_allocated);
            if (t->getStartDate() == -1.0) {
                t->updateStartDate(t->getComputationStartDate());
            }

            if (compute_action->getState() != Action::State::COMPLETED) {
                if (not job_failure_cause) job_failure_cause = compute_action->getFailureCause();
                t->setFailureDate(compute_action->getEndDate());
                failure_count_increments.insert(t);
                state_changes[t] = WorkflowTask::State::READY; // This may be changed to NOT_READY later
                continue;
            }

            /*
             * Look at the file write actions
             */

            this->analyzeActions(this->task_file_write_actions[t],
                                 &at_least_one_failed,
                                 &at_least_one_killed,
                                 &failure_cause,
                                 &earliest_start_date,
                                 &latest_end_date,
                                 &earliest_failure_date);

            if (at_least_one_failed or at_least_one_killed) {
                t->setWriteOutputStartDate(earliest_start_date);
                state_changes[t] = WorkflowTask::State::READY; // This may be changed to NOT_READY later based on other tasks
                if (at_least_one_killed) {
                    job_failure_cause = std::make_shared<JobKilled>(this->shared_this);
                    failure_count_increments.insert(t);
                    t->setFailureDate(earliest_failure_date);
                } else if (at_least_one_failed) {
                    t->setFailureDate(earliest_failure_date);
                    failure_count_increments.insert(t);
                    if (job_failure_cause == nullptr) job_failure_cause = failure_cause;
                }
                continue;
            }
            if (earliest_start_date == -1) earliest_start_date = compute_action->getEndDate();
            if (latest_end_date == -1) latest_end_date = compute_action->getEndDate();
            t->setWriteOutputStartDate(earliest_start_date);
            t->setWriteOutputEndDate(latest_end_date);
            state_changes[t] = WorkflowTask::State::COMPLETED;
            t->setEndDate(latest_end_date);
            simulation->getOutput().addTimestampTaskCompletion(latest_end_date, t);
        }

        /*
        * Look at Post-File copy actions
        */
        if (not this->post_file_copy_actions.empty()) {
            bool at_least_one_failed, at_least_one_killed;
            std::shared_ptr<FailureCause> failure_cause;
            double earliest_start_date, latest_end_date, earliest_failure_date;
            this->analyzeActions(this->post_file_copy_actions,
                                 &at_least_one_failed,
                                 &at_least_one_killed,
                                 &failure_cause,
                                 &earliest_start_date,
                                 &latest_end_date,
                                 &earliest_failure_date);
            if (at_least_one_killed) {
                job_failure_cause = std::make_shared<JobKilled>(this->shared_this);
            } else if (at_least_one_failed) {
                job_failure_cause = failure_cause;
                return;
            }
        }

        /*
        * Look at Cleanup file_deletion actions
        */
        if (not this->cleanup_actions.empty()) {
            bool at_least_one_failed, at_least_one_killed;
            std::shared_ptr<FailureCause> failure_cause;
            double earliest_start_date, latest_end_date, earliest_failure_date;
            this->analyzeActions(this->cleanup_actions,
                                 &at_least_one_failed,
                                 &at_least_one_killed,
                                 &failure_cause,
                                 &earliest_start_date,
                                 &latest_end_date,
                                 &earliest_failure_date);
            if (at_least_one_killed) {
                job_failure_cause = std::make_shared<JobKilled>(this->shared_this);
                return;
            } else if (at_least_one_failed) {
                job_failure_cause = failure_cause;
                return;
            }
        }

        /** Let's not care about the SCRATCH cleanup action. If it failed, oh well **/

        return;
    }

    /**
     * @brief Apply updates to tasks
     * @param state_changes: state changes
     * @param failure_count_increments: failure_cound_increments
     */
    void StandardJob::applyTaskUpdates(map<WorkflowTask *, WorkflowTask::State> &state_changes,
                                       set<WorkflowTask *> &failure_count_increments) {

        // Update task states
        for (auto &state_update : state_changes) {
            WorkflowTask *task = state_update.first;
            task->setState(state_update.second);
        }

        // Update task readiness-es
        for (auto &state_update : state_changes) {
            WorkflowTask *task = state_update.first;
            task->updateReadiness();
            if (task->getState() == WorkflowTask::State::COMPLETED) {
                for (auto const &child : task->getChildren()) {
                    child->updateReadiness();
                }
            }
        }

        // Update task failure counts if any
        for (auto task : failure_count_increments) {
            task->incrementFailureCount();
        }

    }

    /**
     * @brief Determines whether the job's spec uses scratch space
     * @return
     */
    bool StandardJob::usesScratch() {

        for (const auto &fl : this->file_locations) {
            for (auto const &fl_l : fl.second) {
                if (fl_l == FileLocation::SCRATCH) {
                    return true;
                }
            }
        }

        for (auto const &task : this->tasks) {
            for (auto const &f : task->getInputFiles()) {
                if (this->file_locations.find(f) == this->file_locations.end()) {
                    return true;
                }
            }
            for (auto const &f : task->getOutputFiles()) {
                if (this->file_locations.find(f) == this->file_locations.end()) {
                    return true;
                }
            }
        }

        for (auto const &pfc : this->pre_file_copies) {
            if ((std::get<1>(pfc) == FileLocation::SCRATCH) or (std::get<2>(pfc) == FileLocation::SCRATCH)) {
                return true;
            }
        }

        for (auto const &pfc : this->post_file_copies) {
            if ((std::get<1>(pfc) == FileLocation::SCRATCH) or (std::get<2>(pfc) == FileLocation::SCRATCH)) {
                return true;
            }
        }
        return false;
    }

}
