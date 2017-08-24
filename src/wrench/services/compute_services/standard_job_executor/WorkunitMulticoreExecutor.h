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


#include <simgrid_S4U_util/S4U_DaemonWithMailbox.h>

namespace wrench {

    class Simulation;
    class StorageService;
    class WorkflowFile;
    class WorkflowTask;
    class StorageService;
    class StandardJob;
    class WorkerThreadWork;
    class Workunit;

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief An actor that knows how to perform a work unit
     */
    class WorkunitMulticoreExecutor : public S4U_DaemonWithMailbox {

    public:

        WorkunitMulticoreExecutor(
                     Simulation *simulation,
                     std::string hostname,
                     unsigned long num_cores,
                     std::string callback_mailbox,
                     std::shared_ptr<Workunit> workunit,
                     StorageService *default_storage_service,
                     double thread_startup_overhead = 0.0);

        void kill();

        /** @brief The Workunit this WorkunitExecutor is supposed to perform */
        std::shared_ptr<Workunit> workunit;

    private:
        int main();

        void performWork(std::shared_ptr<Workunit> work);

        void runMulticoreComputation(double flops, double parallel_efficiency);


            Simulation *simulation;
        std::string callback_mailbox;
        std::string hostname;
        unsigned long num_cores;
        double thread_startup_overhead;

        StorageService *default_storage_service;

    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_WORKUNITEXECUTOR_H
