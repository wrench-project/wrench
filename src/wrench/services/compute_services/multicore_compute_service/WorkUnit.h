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

    class WorkUnit {

    public:

        WorkUnit(StandardJob *job,
                 std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> pre_file_copies,
                 std::vector<WorkflowTask *> tasks,
                 std::map<WorkflowFile *, StorageService *> file_locations,
                 std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> post_file_copies,
                 std::set<std::tuple<WorkflowFile *, StorageService *>> cleanup_file_deletions);

        static void addDependency(WorkUnit *parent, WorkUnit *child);

        StandardJob *job;
        std::set<WorkUnit *> children;
        unsigned long num_pending_parents;

        std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> pre_file_copies;
        std::vector<WorkflowTask *> tasks;
        std::map<WorkflowFile *, StorageService *> file_locations;
        std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> post_file_copies;
        std::set<std::tuple<WorkflowFile *, StorageService *>> cleanup_file_deletions;

    };

};


#endif //WRENCH_WORKUNIT_H
