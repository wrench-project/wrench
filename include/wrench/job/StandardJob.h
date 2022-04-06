/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_STANDARDJOB_H
#define WRENCH_STANDARDJOB_H


#include <map>
#include <set>
#include <vector>
#include <wrench/workflow/Workflow.h>
#include <wrench/workflow/WorkflowTask.h>
#include <wrench/data_file/DataFile.h>

#include "Job.h"

#include "wrench/services/storage/storage_helpers/FileLocation.h"

namespace wrench {

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    class DataFile;
    class WorkflowTask;
    class Action;

    /**
     * @brief A standard (i.e., non-pilot) workflow job that can be submitted to a ComputeService
     * by a WMS (via a JobManager)
     */
    class StandardJob : public Job, public std::enable_shared_from_this<StandardJob> {

    public:
        /** @brief Standard job states */
        enum State {
            /** @brief Not submitted yet */
            NOT_SUBMITTED,
            /** @brief Submitted but not running yet */
            PENDING,
            /** @brief Running */
            RUNNING,
            /** @brief Completed successfully */
            COMPLETED,
            /** @brief Failed */
            FAILED,
            /** @brief Terminated by submitter */
            TERMINATED
        };

        std::vector<std::shared_ptr<WorkflowTask>> getTasks() const;

        unsigned long getMinimumRequiredNumCores() const;

        unsigned long getMinimumRequiredMemory() const;

        unsigned long getNumCompletedTasks() const;

        unsigned long getNumTasks() const;

        StandardJob::State getState();


        std::map<std::shared_ptr<DataFile>, std::vector<std::shared_ptr<FileLocation>>> getFileLocations() const;
/***********************/
/** \cond INTERNAL     */
/***********************/

        bool usesScratch();

        /** @brief The job's computational tasks */
        std::vector<std::shared_ptr<WorkflowTask>> tasks;

        /** @brief The job's total computational cost (in flops) */
        double total_flops;

        /** @brief The file locations that tasks should read/write files from/to. Each file is given a list of locations, in preferred order */
        std::map<std::shared_ptr<DataFile>, std::vector<std::shared_ptr<FileLocation>>> file_locations;

        /** @brief The ordered file copy operations to perform before computational tasks */
        std::vector<std::tuple<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>>> pre_file_copies;
        /** @brief The ordered file copy operations to perform after computational tasks */
        std::vector<std::tuple<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>>> post_file_copies;
        /** @brief The ordered file deletion operations to perform at the end */
        std::vector<std::tuple<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>>> cleanup_file_deletions;

        /**
         * @brief Get the shared pointer for this object
         * @return a shared pointer to the object
         */
        std::shared_ptr<StandardJob> getSharedPtr() { return this->shared_from_this(); }

        double getPreJobOverheadInSeconds() const;
        void setPreJobOverheadInSeconds(double overhead);
        double getPostJobOverheadInSeconds() const;
        void setPostJobOverheadInSeconds(double overhead);


    private:
        friend class BareMetalComputeService;
        friend class JobManager;
        friend class ExecutionEvent;

        StandardJob(std::shared_ptr<JobManager> job_manager,
                    std::vector<std::shared_ptr<WorkflowTask>> tasks, std::map<std::shared_ptr<DataFile>, std::vector<std::shared_ptr<FileLocation>>> &file_locations,
                    std::vector<std::tuple<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>>> &pre_file_copies,
                    std::vector<std::tuple<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>>> &post_file_copies,
                    std::vector<std::tuple<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>>> &cleanup_file_deletions);

        void createUnderlyingCompoundJob(const std::shared_ptr<ComputeService> &compute_service);
        void processCompoundJobOutcome(std::map<std::shared_ptr<WorkflowTask>, WorkflowTask::State> &state_changes,
                                       std::set<std::shared_ptr<WorkflowTask>> &failure_count_increments,
                                       std::shared_ptr<FailureCause> &job_failure_cause,
                                       Simulation *simulation);
        void applyTaskUpdates(std::map<std::shared_ptr<WorkflowTask>, WorkflowTask::State> &state_changes,
                              std::set<std::shared_ptr<WorkflowTask>> &failure_count_increments);

        void analyzeActions(std::vector<std::shared_ptr<Action>> actions,
                            bool *at_least_one_failed,
                            bool *at_least_one_killed,
                            std::shared_ptr<FailureCause> *failure_cause,
                            double *earliest_start_date,
                            double *latest_end_date,
                            double *earliest_failure_date);


        State state;
        double pre_overhead = 0.0;
        double post_overhead = 0.0;

        std::shared_ptr<CompoundJob> compound_job;
        std::shared_ptr<Action> pre_overhead_action = nullptr;
        std::shared_ptr<Action> post_overhead_action = nullptr;
        std::vector<std::shared_ptr<Action>> pre_file_copy_actions;
        std::map<std::shared_ptr<WorkflowTask>, std::vector<std::shared_ptr<Action>>> task_file_read_actions;
        std::map<std::shared_ptr<WorkflowTask>, std::shared_ptr<Action>> task_compute_actions;
        std::map<std::shared_ptr<WorkflowTask>, std::vector<std::shared_ptr<Action>>> task_file_write_actions;
        std::vector<std::shared_ptr<Action>> post_file_copy_actions;
        std::vector<std::shared_ptr<Action>> cleanup_actions;
        std::shared_ptr<Action> scratch_cleanup = nullptr;

        /***********************/
        /** \endcond           */
        /***********************/
    };

    /***********************/
    /** \endcond           */
    /***********************/

};// namespace wrench

#endif//WRENCH_STANDARDJOB_H
