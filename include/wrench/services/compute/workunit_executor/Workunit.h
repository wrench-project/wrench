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
    class FileLocation;

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief A class to describe a unit of work that's a sub-component of a StandardJob
     */
    class Workunit {

    public:

        Workunit(
                std::shared_ptr<StandardJob> job,
                std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>>> pre_file_copies,
                WorkflowTask *task,
                std::map<WorkflowFile *, std::shared_ptr<FileLocation>> file_locations,
                std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>>> post_file_copies,
                std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>>> cleanup_file_deletions);

        static void addDependency(std::shared_ptr<Workunit> parent, std::shared_ptr<Workunit> child);

        static std::set<std::shared_ptr<Workunit>> createWorkunits(std::shared_ptr<StandardJob> job);

        std::shared_ptr<StandardJob> getJob();

        /** @brief The StandardJob this Workunit belongs to */
        std::shared_ptr<StandardJob> job;

        /** @brief The Workunits that depend on this Workunit */
        std::set<std::shared_ptr<Workunit>> children;
        /** @brief The number of Workunits this Workunit depends on */
        unsigned long num_pending_parents;

        /** @brief File copies to perform before computational tasks begin */
        std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>>> pre_file_copies;
        /** @brief Computational task to perform */
        WorkflowTask *task = nullptr;
        /** @brief Locations where computational tasks should read/write files */
        std::map<WorkflowFile *, std::shared_ptr<FileLocation>> file_locations;
        /** @brief File copies to perform after computational tasks completes */
        std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>>> post_file_copies;
        /** @brief File deletions to perform last */
        std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>>> cleanup_file_deletions;


        ~Workunit();
    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_WORKUNIT_H
