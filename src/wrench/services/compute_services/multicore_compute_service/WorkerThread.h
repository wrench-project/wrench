/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_WORKERTHREAD_H
#define WRENCH_WORKERTHREAD_H


#include <simgrid_S4U_util/S4U_DaemonWithMailbox.h>

namespace wrench {

    class StorageService;
    class WorkflowFile;
    class WorkflowTask;
    class StorageService;
    class StandardJob;

    class WorkerThread : public S4U_DaemonWithMailbox {

    public:

        WorkerThread(std::string hostname, std::string callback_mailbox,
                     StorageService *default_storage_service,
                     double startup_overhead = 0.0);

        void stop();

        void kill();

        void doWork(StandardJob *job,
                    std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> pre_file_copies,
                    std::vector<WorkflowTask *> tasks,
                    std::map<WorkflowFile*, StorageService*> file_locations,
                    std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> post_file_copies,
                    std::set<std::tuple<WorkflowFile *, StorageService *>> cleanup_file_deletions);

    private:

        int main();
        void performWork(std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> pre_file_copies,
                         std::vector<WorkflowTask *> tasks,
                         std::map<WorkflowFile*, StorageService*> file_locations,
                         std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> post_file_copies);

        std::string callback_mailbox;
        std::string hostname;
        double start_up_overhead;

        StorageService *default_storage_service;
    };

};


#endif //WRENCH_WORKERTHREAD_H
