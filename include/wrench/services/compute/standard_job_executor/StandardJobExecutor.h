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
#include <set>

#include "wrench/services/compute/ComputeService.h"
#include "wrench/services/compute/standard_job_executor/WorkunitMulticoreExecutor.h"
#include "wrench/services/compute/standard_job_executor/StandardJobExecutorProperty.h"
#include "wrench/services/compute/standard_job_executor/Workunit.h"


namespace wrench {

    class Simulation;

    class StorageService;

    class FailureCause;

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**  @brief A base abstraction that knows how to execute a standard job
     *   on a multi-node multi-core platform. Note that when killed in the middle
     *   of computing, this service will set (internal) running tasks' states to FAILED, and
     *   likely the calling service will want to make failed tasks ready again. Also, this
     *   service does not increment task failure counts, as it does not know if the kill() was
     *   an actual failure (i.e., some timeout) or a feature (i.e., a WMS changing its mind)
     */
    class StandardJobExecutor : public Service {

    public:

        ~StandardJobExecutor();

        // Public Constructor
        StandardJobExecutor (
                Simulation *simulation,
                std::string callback_mailbox,
                std::string hostname,
                StandardJob *job,
                std::set<std::tuple<std::string, unsigned long, double>> compute_resources,
                std::map<std::string, std::string> plist = {},
                StorageService* scratch_space = nullptr,
                bool part_of_pilot_job = false);

        void kill();

        StandardJob *getJob();
        std::set<std::tuple<std::string, unsigned long, double>> getComputeResources();

        // Get the set of files stored in scratch space by a standardjob job
        std::set<WorkflowFile*> getFilesInScratch();

    private:

        friend class Simulation;
        void cleanup() override;

        std::string callback_mailbox;
        StandardJob *job;
        std::set<std::tuple<std::string, unsigned long, double>> compute_resources;
        int total_num_cores;
        double total_ram;
        StorageService *scratch_space;

        bool part_of_pilot_job;

        // Files stored in scratch
        std::set<WorkflowFile*> files_stored_in_scratch;

        // Core availabilities (for each hosts, how many cores are currently available on it)
        std::map<std::string, unsigned long> core_availabilities;
        // RAM availabilities (for each host, host many bytes of RAM are currently available on it)
        std::map<std::string, double> ram_availabilities;

        // Sets of workunit executors
        std::set<std::shared_ptr<WorkunitMulticoreExecutor>> running_workunit_executors;
        std::set<std::shared_ptr<WorkunitMulticoreExecutor>> finished_workunit_executors;
        std::set<std::shared_ptr<WorkunitMulticoreExecutor>> failed_workunit_executors;

        // Work units
        std::set<std::unique_ptr<Workunit>> non_ready_workunits;
        std::set<std::unique_ptr<Workunit>> ready_workunits;
        std::set<std::unique_ptr<Workunit>> running_workunits;
        std::set<std::unique_ptr<Workunit>> completed_workunits;

        // Property list
        std::map<std::string, std::string> property_list;

        std::map<std::string, std::string> default_property_values = {
                {StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD, "0"},
                {StandardJobExecutorProperty::STANDARD_JOB_DONE_MESSAGE_PAYLOAD, "1024"},
                {StandardJobExecutorProperty::STANDARD_JOB_FAILED_MESSAGE_PAYLOAD, "1024"},
                {StandardJobExecutorProperty::STANDARD_JOB_FILES_STORED_IN_SCRATCH, "1024"},
                {StandardJobExecutorProperty::CORE_ALLOCATION_ALGORITHM, "maximum"},
                {StandardJobExecutorProperty::TASK_SELECTION_ALGORITHM, "maximum_flops"},
                {StandardJobExecutorProperty::HOST_SELECTION_ALGORITHM, "best_fit"},
        };

        int main() override;

//        void setProperty(std::string property, std::string value);
//        std::string getPropertyValueAsString(std::string property);
//        double getPropertyValueAsDouble(std::string property);

        void processWorkunitExecutorCompletion(WorkunitMulticoreExecutor *workunit_executor,
                                               Workunit *workunit);

        void processWorkunitExecutorFailure(WorkunitMulticoreExecutor *workunit_executor,
                                            Workunit *workunit,
                                            std::shared_ptr<FailureCause> cause);

        bool processNextMessage();

        unsigned long computeWorkUnitMinNumCores(Workunit *wu);
        unsigned long computeWorkUnitDesiredNumCores(Workunit *wu);
        double computeWorkUnitMinMemory(Workunit *wu);

          void dispatchReadyWorkunits();

        void createWorkunits();

        std::vector<Workunit*> sortReadyWorkunits();

        //Clean up scratch
        void cleanUpScratch();

        //Store the list of files available in scratch
        void StoreListOfFilesInScratch();


    };

    /***********************/
    /** \endcond           */
    /***********************/
};


#endif //WRENCH_MULTINODEMULTICORESTANDARDJOBEXECUTOR_H
