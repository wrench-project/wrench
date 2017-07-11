/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_WORKUNITEXECUTOR_H
#define WRENCH_WORKUNITEXECUTOR_H


#include <simgrid_S4U_util/S4U_DaemonWithMailbox.h>

namespace wrench {

    class Simulation;
    class StorageService;
    class WorkflowFile;
    class WorkflowTask;
    class StorageService;
    class StandardJob;
    class WorkerThreadWork;
    class WorkUnit;

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief An actor that knows how to perform a work unit
     */
    class WorkUnitExecutor : public S4U_DaemonWithMailbox {

    public:

        WorkUnitExecutor(Simulation *simulation,
                     std::string hostname, std::string callback_mailbox,
                     std::shared_ptr<WorkUnit> work,
                     StorageService *default_storage_service,
                     double startup_overhead = 0.0);


        void kill();

        /** @brief The WorkUnit this WorkUnitExecutor is supposed to perform */
        std::shared_ptr<WorkUnit> work;

    private:
        int main();

        Simulation *simulation;

        void performWork(std::shared_ptr<WorkUnit> work);
        std::string callback_mailbox;
        std::string hostname;
        double start_up_overhead;

        StorageService *default_storage_service;

    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_WORKUNITEXECUTOR_H
