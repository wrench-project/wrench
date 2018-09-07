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
#include <memory>

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
    class Workunit {

    public:

        Workunit(std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> pre_file_copies,
                 WorkflowTask * task,
                 std::map<WorkflowFile *, StorageService *> file_locations,
                 std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> post_file_copies,
                 std::set<std::tuple<WorkflowFile *, StorageService *>> cleanup_file_deletions);

        static void addDependency(Workunit *parent, Workunit *child);

        /** @brief The Workunits that depend on this Workunit */
        std::set<Workunit*> children;
        /** @brief The number of Workunits this Workunit depends on */
        unsigned long num_pending_parents;

        /** @brief File copies to perform before computational tasks begin */
        std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> pre_file_copies;
        /** @brief Computational task to perform */
        WorkflowTask *task = nullptr;
        /** @brief Locations where computational tasks should read/write files */
        std::map<WorkflowFile *, StorageService *> file_locations;
        /** @brief File copies to perform after computational tasks completes */
        std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> post_file_copies;
        /** @brief File deletions to perform last */
        std::set<std::tuple<WorkflowFile *, StorageService *>> cleanup_file_deletions;


        ~Workunit();
    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_WORKUNIT_H
