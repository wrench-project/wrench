/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/workflow/job/StandardJob.h>
#include <wrench/workflow/WorkflowTask.h>
#include <wrench/workflow/Workflow.h>
#include <iostream>
#include "wrench/services/compute/workunit_executor/Workunit.h"
#include <wrench-dev.h>

WRENCH_LOG_NEW_DEFAULT_CATEGORY(workunit, "Log category for Workunit");


namespace wrench {

    /**
    * @brief Constructor
    * @param job: the job this workunit belongs to
    * @param pre_file_copies: a set of file copy actions to perform in sequence first
    * @param task: a WorkflowTask
    * @param file_locations: locations where tasks should read/write files
    * @param post_file_copies: a set of file copy actions to perform in sequence after all tasks
    * @param cleanup_file_deletions: a set of file deletion actions to perform last
    */
    Workunit::Workunit(
            StandardJob *job,
            std::set<std::tuple<WorkflowFile *, std::shared_ptr<StorageService>, std::shared_ptr<StorageService>>> pre_file_copies,
            WorkflowTask *task,
            std::map<WorkflowFile *, std::shared_ptr<StorageService>> file_locations,
            std::set<std::tuple<WorkflowFile *, std::shared_ptr<StorageService>, std::shared_ptr<StorageService> >> post_file_copies,
            std::set<std::tuple<WorkflowFile *, std::shared_ptr<StorageService> >> cleanup_file_deletions) {

        this->num_pending_parents = 0;

        // Double-check that there is no weirdness
        for (auto const &pfc : pre_file_copies) {
            auto file = std::get<0>(pfc);
            auto src = std::get<1>(pfc);
            auto dst = std::get<2>(pfc);
            if ((file == nullptr) || (src == nullptr) || (dst == nullptr)) {
                throw std::invalid_argument("Workunit::Workunit(): invalid pre file copy spec");
            }
        }

        for (auto const &fl : file_locations) {
            auto file = std::get<0>(fl);
            auto ss = std::get<1>(fl);
            if ((file == nullptr)  || (ss == nullptr)) {
                throw std::invalid_argument("Workunit::Workunit(): invalid file location spec");
            }
        }

        for (auto const &pfc : post_file_copies) {
            auto file = std::get<0>(pfc);
            auto src = std::get<1>(pfc);
            auto dst = std::get<2>(pfc);
            if ((file == nullptr) || (src == nullptr) || (dst == nullptr)) {
                throw std::invalid_argument("Workunit::Workunit(): invalid post file copy spec");
            }
        }

        for (auto const &cd : cleanup_file_deletions) {
            auto file = std::get<0>(cd);
            auto ss = std::get<1>(cd);
            if ((file == nullptr)  || (ss == nullptr)) {
                throw std::invalid_argument("Workunit::Workunit(): invalid file cleanup spec");
            }
        }

        this->job = job;
        this->pre_file_copies = pre_file_copies;
        this->task = task;
        this->file_locations = file_locations;
        this->post_file_copies = post_file_copies;
        this->cleanup_file_deletions = cleanup_file_deletions;

    }

    /**
     * @brief Add a dependency between two work units (does nothing
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
    }

    /**
     * @brief Create all work units for a newly dispatched job
     * @param job: the job
     * @return A set of work units
     */
    std::set<std::shared_ptr<Workunit>> Workunit::createWorkunits(StandardJob *job) {

        std::shared_ptr<Workunit> pre_file_copies_work_unit = nullptr;
        std::vector<std::shared_ptr<Workunit>> task_work_units;
        std::shared_ptr<Workunit> post_file_copies_work_unit = nullptr;
        std::shared_ptr<Workunit> cleanup_workunit = nullptr;

        std::set<std::shared_ptr<Workunit>> all_work_units;

        // Create the cleanup workunit, if any
        if (not job->cleanup_file_deletions.empty()) {
            cleanup_workunit = std::make_shared<Workunit>(job,
                                                          (std::set<std::tuple<WorkflowFile *, std::shared_ptr<StorageService>, std::shared_ptr<StorageService> >>) {},
                                                          nullptr,
                                                          (std::map<WorkflowFile *, std::shared_ptr<StorageService> >) {},
                                                          (std::set<std::tuple<WorkflowFile *, std::shared_ptr<StorageService>, std::shared_ptr<StorageService> >>) {},
                                                          job->cleanup_file_deletions);
        }

        // Create the pre_file_copies work unit, if any
        if (not job->pre_file_copies.empty()) {
            pre_file_copies_work_unit = std::make_shared<Workunit>(job,
                                                                   job->pre_file_copies,
                                                                   nullptr,
                                                                   (std::map<WorkflowFile *, std::shared_ptr<StorageService> >) {},
                                                                   (std::set<std::tuple<WorkflowFile *, std::shared_ptr<StorageService>, std::shared_ptr<StorageService> >>) {},
                                                                   (std::set<std::tuple<WorkflowFile *, std::shared_ptr<StorageService> >>) {});
        }

        // Create the post_file_copies work unit, if any
        if (not job->post_file_copies.empty()) {
            post_file_copies_work_unit = std::make_shared<Workunit>(job,
                                                                    (std::set<std::tuple<WorkflowFile *, std::shared_ptr<StorageService>, std::shared_ptr<StorageService> >>) {},
                                                                    nullptr,
                                                                    (std::map<WorkflowFile *, std::shared_ptr<StorageService> >) {},
                                                                    job->post_file_copies,
                                                                    (std::set<std::tuple<WorkflowFile *, std::shared_ptr<StorageService> >>) {});
        }

        // Create the task work units, if any
        for (auto const &task : job->tasks) {
            task_work_units.push_back(std::make_shared<Workunit>(job,
                                                                 (std::set<std::tuple<WorkflowFile *, std::shared_ptr<StorageService>, std::shared_ptr<StorageService> >>) {},
                                                                 task,
                                                                 job->file_locations,
                                                                 (std::set<std::tuple<WorkflowFile *, std::shared_ptr<StorageService>, std::shared_ptr<StorageService> >>) {},
                                                                 (std::set<std::tuple<WorkflowFile *, std::shared_ptr<StorageService> >>) {}));
        }

        // Add dependencies between task work units, if any
        for (auto const &task_work_unit : task_work_units) {
            const WorkflowTask *task = task_work_unit->task;

            if (task->getInternalState() != WorkflowTask::InternalState::TASK_READY) {
                std::vector<WorkflowTask *> current_task_parents = task->getWorkflow()->getTaskParents(task);

                for (auto const &potential_parent_task_work_unit : task_work_units) {
                    if (std::find(current_task_parents.begin(), current_task_parents.end(),
                                  potential_parent_task_work_unit->task) != current_task_parents.end()) {
                        Workunit::addDependency(potential_parent_task_work_unit, task_work_unit);
                    }
                }
            }
        }

        // Add dependencies from pre copies to possible successors
        if (pre_file_copies_work_unit != nullptr) {
            if (not task_work_units.empty()) {
                for (auto const &twu: task_work_units) {
                    Workunit::addDependency(pre_file_copies_work_unit, twu);
                }
            } else if (post_file_copies_work_unit != nullptr) {
                Workunit::addDependency(pre_file_copies_work_unit, post_file_copies_work_unit);
            } else if (cleanup_workunit != nullptr) {
                Workunit::addDependency(pre_file_copies_work_unit, cleanup_workunit);
            }
        }

        // Add dependencies from tasks to possible successors
        for (auto const &twu: task_work_units) {
            if (post_file_copies_work_unit != nullptr) {
                Workunit::addDependency(twu, post_file_copies_work_unit);
            } else if (cleanup_workunit != nullptr) {
                Workunit::addDependency(twu, cleanup_workunit);
            }
        }

        // Add dependencies from post copies to possible successors
        if (post_file_copies_work_unit != nullptr) {
            if (cleanup_workunit != nullptr) {
                Workunit::addDependency(post_file_copies_work_unit, cleanup_workunit);
            }
        }

        // Create a list of all work units
        if (pre_file_copies_work_unit) all_work_units.insert(pre_file_copies_work_unit);
        for (auto const &twu : task_work_units) {
            all_work_units.insert(twu);
        }
        if (post_file_copies_work_unit) all_work_units.insert(post_file_copies_work_unit);
        if (cleanup_workunit) all_work_units.insert(cleanup_workunit);

        task_work_units.clear();

        return all_work_units;

    }

    /**
     * @brief Retrieve the standard job this workunit belongs to
     * @return a standard job
     */
    StandardJob *Workunit::getJob() {
        return this->job;
    }

};
