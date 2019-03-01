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
    class WorkflowJob;

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief An service that performs a WorkUnit
     */
    class WorkunitExecutor : public Service {

    public:

        WorkunitExecutor(
                     Simulation *simulation,
                     std::string hostname,
                     unsigned long num_cores,
                     double ram_utilization,
                     std::string callback_mailbox,
                     Workunit *workunit,
                     StorageService *scratch_space,
                     StandardJob* job,
                     double thread_startup_overhead,
                     bool simulate_computation_as_sleep);

        void kill();

        unsigned long getNumCores();
        StandardJob *getJob();
        double getMemoryUtilization();
        std::set<WorkflowFile*> getFilesStoredInScratch();

        /** @brief The Workunit this WorkunitExecutor is supposed to perform */
        Workunit *workunit;

    private:

        int main() override;
        void cleanup(bool has_returned_from_main, int return_value) override;

        void performWork(Workunit *work);

        void runMulticoreComputation(double flops, double parallel_efficiency, bool simulate_computation_as_sleep);

        std::string callback_mailbox;
        unsigned long num_cores;
        double ram_utilization;
        double thread_startup_overhead;
        bool simulate_computation_as_sleep;

        StorageService* scratch_space;

        std::set<WorkflowFile* > files_stored_in_scratch;
        std::vector<std::shared_ptr<ComputeThread>> compute_threads;

        // a reference to the job it is a part of (currently required for creating the /tmp directory in scratch space)
        StandardJob* job;

    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_WORKUNITEXECUTOR_H
