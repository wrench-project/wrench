/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_WORKUNITMULTICOREEXECUTOR_H
#define WRENCH_WORKUNITMULTICOREEXECUTOR_H


#include "wrench/services/Service.h"
#include <set>
#include "Workunit.h"

namespace wrench {

    class Simulation;
    class StorageService;
    class WorkflowFile;
    class WorkflowTask;
    class StorageService;
    class StandardJob;
    class WorkerThreadWork;
    class Workunit;
    class ComputeThread;
    class Job;
    class SimulationTimestampTaskFailure;

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief An service that performs a WorkUnit
     */
    class WorkunitExecutor : public Service {

    public:

        WorkunitExecutor(
                     std::string hostname,
                     unsigned long num_cores,
                     double ram_utilization,
                     std::string callback_mailbox,
                     std::shared_ptr<Workunit> workunit,
                     std::shared_ptr<StorageService> scratch_space,
                     std::shared_ptr<StandardJob>  job,
                     double thread_startup_overhead,
                     bool simulate_computation_as_sleep);

        void kill(bool job_termination);

        unsigned long getNumCores();
        std::shared_ptr<StandardJob> getJob();
        double getMemoryUtilization();
        std::set<WorkflowFile*> getFilesStoredInScratch();

        /** @brief The Workunit this WorkunitExecutor is supposed to perform */
        std::shared_ptr<Workunit> workunit;

    private:

        bool failure_timestamp_should_be_generated = false;
        bool task_completion_timestamp_should_be_generated = false;
        bool terminated_due_job_being_forcefully_terminated = false;
        bool task_start_timestamp_has_been_inserted = false;
        bool task_failure_time_stamp_has_already_been_generated = false;

        SimulationTimestampTaskFailure *foo;
        int main() override;
        void cleanup(bool has_returned_from_main, int return_value) override;

        void performWork(Workunit *work);

        void runMulticoreComputationForTask(WorkflowTask *task, bool simulate_computation_as_sleep);

        bool isUseOfScratchSpaceOK();
        bool areFileLocationsOK(WorkflowFile **offending_file);

        std::string callback_mailbox;
        unsigned long num_cores;
        double ram_utilization;
        double thread_startup_overhead;
        bool simulate_computation_as_sleep;

        std::shared_ptr<StorageService> scratch_space;

        std::set<WorkflowFile* > files_stored_in_scratch;
        std::vector<std::shared_ptr<ComputeThread>> compute_threads;

        // a reference to the job it is a part of (currently required for creating the /tmp directory in scratch space)
        std::shared_ptr<StandardJob> job;

    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_WORKUNITEXECUTOR_H
