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

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief An actor that knows how to perform a work unit
     */
    class WorkunitMulticoreExecutor : public Service {

    public:

        WorkunitMulticoreExecutor(
                     Simulation *simulation,
                     std::string hostname,
                     unsigned long num_cores,
                     double ram_utilization,
                     std::string callback_mailbox,
                     Workunit *workunit,
                     StorageService *scratch_space,
                     double thread_startup_overhead = 0.0);

        void kill();

        unsigned long getNumCores();
        double getMemoryUtilization();
        std::set<WorkflowFile*> getFilesStoredInScratch();

        /** @brief The Workunit this WorkunitExecutor is supposed to perform */
        Workunit *workunit;

    private:

        int main();

        void performWork(Workunit *work);

        void runMulticoreComputation(double flops, double parallel_efficiency);

        std::string callback_mailbox;
        unsigned long num_cores;
        double ram_utilization;
        double thread_startup_overhead;

        StorageService* scratch_space;

        std::set<WorkflowFile* > files_stored_in_scratch;
        std::vector<std::shared_ptr<ComputeThread>> compute_threads;

    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_WORKUNITEXECUTOR_H
