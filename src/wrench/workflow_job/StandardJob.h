/**
 * Copyright (c) 2017. The WRENCH Team.
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

#include "workflow_job/WorkflowJob.h"

namespace wrench {

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    class StorageService;

    class WorkflowFile;

    class WorkflowTask;

    /**
     * @brief A standard (i.e., non-pilot) WorkflowJob
     */
    class StandardJob : public WorkflowJob {

    public:
        enum State {
            NOT_SUBMITTED,
            PENDING,
            RUNNING,
            COMPLETED,
            FAILED,
        };

        std::vector<WorkflowTask *> getTasks();

        void incrementNumCompletedTasks();

        unsigned long getNumCompletedTasks();

        unsigned long getNumTasks();

        std::map<WorkflowFile *, StorageService *> getFileLocations();

        // Tasks to run
        std::vector<WorkflowTask *> tasks;
        unsigned long num_completed_tasks;
        std::map<WorkflowFile *, StorageService *> file_locations;

        std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> pre_file_copies;
        std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> post_file_copies;
        std::set<std::tuple<WorkflowFile *, StorageService *>> cleanup_file_deletions;

        ~StandardJob();

    private:
        friend class JobManager;

        StandardJob(std::vector<WorkflowTask *> tasks, std::map<WorkflowFile *, StorageService *> file_locations,
                    std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> pre_file_copies,
                    std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> post_file_copies,
                    std::set<std::tuple<WorkflowFile *, StorageService *>> cleanup_file_deletions);


        State state;


    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_MULTITASKJOB_H
