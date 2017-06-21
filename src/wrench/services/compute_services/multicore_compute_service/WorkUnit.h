/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_WORKUNIT_H
#define WRENCH_WORKUNIT_H


#include <tuple>
#include <set>
#include <map>
#include <vector>

namespace wrench {

    class StandardJob;
    class WorkflowFile;
    class StorageService;
    class WorkflowTask;

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief A class to describe a unit of work that's a sub-component of a StandardJob
     */
    class WorkUnit {

    public:

        WorkUnit(StandardJob *job,
                 std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> pre_file_copies,
                 std::vector<WorkflowTask *> tasks,
                 std::map<WorkflowFile *, StorageService *> file_locations,
                 std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> post_file_copies,
                 std::set<std::tuple<WorkflowFile *, StorageService *>> cleanup_file_deletions);

        static void addDependency(WorkUnit *parent, WorkUnit *child);

        /** @brief The job that this WorkUnit belongs to */
        StandardJob *job;
        /** @brief The WorkUnits that depend on this WorkUnit */
        std::set<WorkUnit *> children;
        /** @brief The number of WorkUnits this WorkUnit depends on */
        unsigned long num_pending_parents;

        /** @brief File copies to perform before computational tasks begin */
        std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> pre_file_copies;
        /** @brief Computational tasks to perform */
        std::vector<WorkflowTask *> tasks;
        /** @brief Locations where computational tasks should read/write files */
        std::map<WorkflowFile *, StorageService *> file_locations;
        /** @brief File copies to perform after computational tasks completes */
        std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> post_file_copies;
        /** @brief File deletions to perform last */
        std::set<std::tuple<WorkflowFile *, StorageService *>> cleanup_file_deletions;

    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_WORKUNIT_H
