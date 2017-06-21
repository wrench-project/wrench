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

#include <services/Service.h>
#include <workflow_job/WorkflowJob.h>

namespace wrench {

    /***********************/
    /** \cond DEVELOPER   **/
    /***********************/

    /* Forward References */

    class Simulation;

    class StandardJob;

    class PilotJob;

    class StorageService;

    /**
     * @brief Abstract implementation of a compute service.
     */
    class ComputeService : public Service {

    public:

        void stop();

        void runJob(WorkflowJob *job);

        bool canRunJob(WorkflowJob::Type job_type, unsigned long min_num_cores, double duration);

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
