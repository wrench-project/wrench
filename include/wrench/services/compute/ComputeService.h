/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef SIMULATION_COMPUTESERVICE_H
#define SIMULATION_COMPUTESERVICE_H

#include <map>

#include "wrench/services/Service.h"
#include "wrench/workflow/job/WorkflowJob.h"

namespace wrench {


    class Simulation;

    class StandardJob;

    class PilotJob;

    class StorageService;

    /**
     * @brief  A top-level class that defines a compute service.
     */
    class ComputeService : public Service {

    public:

        /***********************/
        /** \cond DEVELOPER   **/
        /***********************/

        void stop();

        void runJob(WorkflowJob *job);

        void terminateJob(WorkflowJob *job);

        bool supportsStandardJobs();

        bool supportsPilotJobs();

        virtual double getCoreFlopRate();

        virtual unsigned long getNumCores();

        virtual unsigned long getNumIdleCores();

        virtual double getTTL();

        void setDefaultStorageService(StorageService *storage_service);

        StorageService *getDefaultStorageService();

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

        virtual void submitStandardJob(StandardJob *job);

        virtual void submitPilotJob(PilotJob *job);

        virtual void terminateStandardJob(StandardJob *job);

        virtual void terminatePilotJob(PilotJob *job);

        ComputeService(std::string service_name,
                       std::string mailbox_name_prefix,
                       StorageService *default_storage_service);

    protected:

        /** @brief Whether the compute service supports pilot jobs */
        bool supports_pilot_jobs;
        /** @brief Whether the compute service supports standard jobs */
        bool supports_standard_jobs;

        /** @brief The default storage service associated to the compute service (nullptr if none) */
        StorageService *default_storage_service;

        /***********************/
        /** \endcond          **/
        /***********************/

    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //SIMULATION_COMPUTESERVICE_H
