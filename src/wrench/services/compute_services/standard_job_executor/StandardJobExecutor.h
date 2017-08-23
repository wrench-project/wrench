/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_MULTINODEMULTICORESTANDARDJOBEXECUTOR_H
#define WRENCH_MULTINODEMULTICORESTANDARDJOBEXECUTOR_H


#include <queue>
#include <deque>
#include <set>

#include <services/compute_services/ComputeService.h>

#include "WorkunitMulticoreExecutor.h"
#include "StandardJobExecutorProperty.h"
#include "Workunit.h"


namespace wrench {

    class Simulation;

    class StorageService;

    class FailureCause;

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**  @brief A base abstraction that knows how to execute a standard job
     *   on a multi-node multi-core platform.
     */
    class StandardJobExecutor : public S4U_DaemonWithMailbox {

    public:

        // Public Constructor
        StandardJobExecutor (
                Simulation *simulation,
                std::string callback_mailbox,
                std::string hostname,
                StandardJob *job,
                std::set<std::tuple<std::string, unsigned long>> compute_resources,
                StorageService *default_storage_service,
                std::map<std::string, std::string> plist = {});

        void kill();

    private:

        friend class Simulation;

        Simulation *simulation;
        std::string callback_mailbox;
        StandardJob *job;
        std::set<std::tuple<std::string, unsigned long>> compute_resources;
        int total_num_cores;
        StorageService *default_storage_service;

        // Core availabilities (for each hosts, how many cores are currently available on it)
        std::map<std::string, unsigned long> core_availabilities;

        // Vector of worker threads
        std::set<WorkunitMulticoreExecutor *> workunit_executors;

        // Work units
        std::set<std::shared_ptr<Workunit>> non_ready_workunits;
        std::deque<std::shared_ptr<Workunit>> ready_workunits;
        std::set<std::shared_ptr<Workunit>> running_workunits;
        std::set<std::shared_ptr<Workunit>> completed_workunits;

        // Property list
        std::map<std::string, std::string> property_list;

        std::map<std::string, std::string> default_property_values = {
                        {StandardJobExecutorProperty::WORKUNIT_EXECUTOR_STARTUP_OVERHEAD, "0"},
                        {StandardJobExecutorProperty::STANDARD_JOB_DONE_MESSAGE_PAYLOAD, "1024"},
                };

        int main();

        void setProperty(std::string property, std::string value);
        std::string getPropertyValueAsString(std::string property);
        double getPropertyValueAsDouble(std::string property);

        void processWorkunitExecutorCompletion(WorkunitMulticoreExecutor *workunit_executor,
                                               std::shared_ptr<Workunit> workunit);

        void processWorkunitExecutorFailure(WorkunitMulticoreExecutor *workunit_executor,
                                            std::shared_ptr<Workunit> workunit,
                                            std::shared_ptr<FailureCause> cause);

        bool processNextMessage();

        bool dispatchNextPendingWork();

        void createWorkunits();

    };

    /***********************/
    /** \endcond           */
    /***********************/
};


#endif //WRENCH_MULTINODEMULTICORESTANDARDJOBEXECUTOR_H
