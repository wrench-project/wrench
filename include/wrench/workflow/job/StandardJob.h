/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_MULTITASKJOB_H
#define WRENCH_MULTITASKJOB_H


#include <vector>
#include <map>
#include <set>
#include <vector>

#include "WorkflowJob.h"

namespace wrench {

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    class StorageService;

    class WorkflowFile;

    class WorkflowTask;

    /**
     * @brief A standard (i.e., non-pilot) workflow job that can be submitted to a ComputeService
     * by a WMS (via a JobManager)
     */
    class StandardJob : public WorkflowJob {

    public:
        /** @brief Standad job states */
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

        std::vector<WorkflowTask *> getTasks();

        unsigned long getMinimumRequiredNumCores();

        unsigned long getNumCompletedTasks();

        unsigned long getNumTasks();

        StandardJob::State getState();

        std::map<WorkflowFile *, StorageService *> getFileLocations();

        // Tasks to run


        /** @brief The job's computational tasks */
        std::vector<WorkflowTask *> tasks;
        /** @brief The job's total computational cost (in flops) */
        double total_flops;
        /** @brief The number of computational tasks that have completed */
        unsigned long num_completed_tasks;
        /** @brief The file locations that tasks should read/write files from/to */
        std::map<WorkflowFile *, StorageService *> file_locations;
        /** @brief The file copy operations to perform before computational tasks */
        std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> pre_file_copies;

        /** @brief The file copy operations to perform after computational tasks */
        std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> post_file_copies;
        /** @brief The file deletion operations to perform at the end */
        std::set<std::tuple<WorkflowFile *, StorageService *>> cleanup_file_deletions;

    private:

        friend class StandardJobExecutor;
        friend class BareMetalComputeService;

        void incrementNumCompletedTasks();

        friend class JobManager;

        StandardJob(Workflow *workflow,
                    std::vector<WorkflowTask *> tasks, std::map<WorkflowFile *, StorageService *> &file_locations,
                    std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> &pre_file_copies,
                    std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> &post_file_copies,
                    std::set<std::tuple<WorkflowFile *, StorageService *>> &cleanup_file_deletions);

        State state;

    };

    /***********************/
    /** \endcond           */
    /***********************/

};

#endif //WRENCH_MULTITASKJOB_H
