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

#include <iostream>
#include <cfloat>
#include <climits>

#include "wrench/services/Service.h"
#include "wrench/workflow/job/WorkflowJob.h"

namespace wrench {

    class Simulation;

    class StandardJob;

    class PilotJob;

    class StorageService;

    /**
     * @brief The compute service base class
     */
    class ComputeService : public Service {

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

        friend  class StandardJobExecutorTest;

        /***********************/
        /** \endcond          **/
        /***********************/

        friend  class Simulation;

    public:

        /** @brief A convenient constant to mean "use all cores on the physical host" whenever a number of cores
         *  is needed when instantiat services
         */
        static constexpr unsigned long ALL_CORES = ULONG_MAX;

        /** @brief A convenient constant to mean "use all ram on the physical host" whenever a ram capacity
         *  is needed when instantiating services
         */
        static constexpr double ALL_RAM = DBL_MAX;

        /** A static StorageService pointer to the SCRATCH space inside the compute service **/
        static StorageService* SCRATCH;

        /***********************/
        /** \cond DEVELOPER   **/
        /***********************/

        virtual ~ComputeService(){}

        void stop() override;

        void submitJob(WorkflowJob *job, std::map<std::string, std::string> = {});

        void terminateJob(WorkflowJob *job);

        void setLocalScratch();

        bool supportsStandardJobs();

        bool supportsPilotJobs();

        bool hasScratch();

        unsigned long getNumHosts();

        std::vector<unsigned long> getNumCores();

        std::vector<unsigned long> getNumIdleCores();

        std::vector<double> getMemoryCapacity();

        std::vector<double> getCoreFlopRate();

        double getTTL();

        double getScratchSize();

        double getFreeRemainingScratchSpace();

        void setDefaultStorageService(StorageService *storage_service);

        StorageService *getDefaultStorageService();

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

        virtual void
        submitStandardJob(StandardJob *job, std::map<std::string, std::string> &service_specific_arguments) = 0;

        virtual void submitPilotJob(PilotJob *job, std::map<std::string, std::string> &service_specific_arguments) = 0;


        virtual void terminateStandardJob(StandardJob *job) = 0;

        virtual void terminatePilotJob(PilotJob *job) = 0;

        ComputeService(std::string hostname,
                       std::string service_name,
                       std::string mailbox_name_prefix,
                       bool supports_standard_jobs,
                       bool supports_pilot_jobs,
                       double sratch_size = 0);

    protected:

        ComputeService(std::string hostname,
                       std::string service_name,
                       std::string mailbox_name_prefix,
                       bool supports_standard_jobs,
                       bool supports_pilot_jobs,
                       StorageService* scratch_space = nullptr);

//        virtual void processGetResourceInformation(const std::string &answer_mailbox) = 0;

//        virtual void processSubmitStandardJob(const std::string &answer_mailbox, StandardJob *job,
//                                              std::map<std::string, std::string> &service_specific_args) = 0;
//
//        virtual void processSubmitPilotJob(const std::string &answer_mailbox, PilotJob *job) = 0;

        /** @brief Whether the compute service supports pilot jobs */
        bool supports_pilot_jobs;
        /** @brief Whether the compute service supports standard jobs */
        bool supports_standard_jobs;

        /** @brief A scratch storage service associated to the compute service */
        StorageService* scratch_space_storage_service;

        StorageService* getScratch();

    private:

        std::map<std::string, std::vector<double>> getServiceResourceInformation();

        /***********************/
        /** \endcond          **/
        /***********************/
    };

    /***********************/
    /** \endcond           */
    /***********************/
};

#endif //SIMULATION_COMPUTESERVICE_H
