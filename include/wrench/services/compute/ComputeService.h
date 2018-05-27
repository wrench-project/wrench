/**
 * Copyright (c) 2017-2018. The WRENCH Team.
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

        friend class StandardJobExecutorTest;

        /***********************/
        /** \endcond          **/
        /***********************/

        friend class Simulation;

    public:

        /** @brief A convenient constant to mean "use all cores of a physical host" whenever a number of cores
         *  is needed when instantiating compute services
         */
        static constexpr unsigned long ALL_CORES = ULONG_MAX;

        /** @brief A convenient constant to mean "use all ram of a physical host" whenever a ram capacity
         *  is needed when instantiating compute services
         */
        static constexpr double ALL_RAM = DBL_MAX;


        /***********************/
        /** \cond DEVELOPER   **/
        /***********************/

        /** @brief A convenient constant to mean "the scratch storage space" of a ComputeService. This is used
         *   to move data to a ComputeService's scratch storage space. **/
        static StorageService *SCRATCH;

        virtual ~ComputeService() {}

        void stop() override;

        void submitJob(WorkflowJob *job, std::map<std::string, std::string> = {});

        void terminateJob(WorkflowJob *job);

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

        /***********************/
        /** \endcond          **/
        /***********************/

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

        virtual void
        submitStandardJob(StandardJob *job, std::map<std::string, std::string> &service_specific_arguments) = 0;

        virtual void submitPilotJob(PilotJob *job, std::map<std::string, std::string> &service_specific_arguments) = 0;

        virtual void terminateStandardJob(StandardJob *job) = 0;

        virtual void terminatePilotJob(PilotJob *job) = 0;

        ComputeService(const std::string &hostname,
                       std::string service_name,
                       std::string mailbox_name_prefix,
                       bool supports_standard_jobs,
                       bool supports_pilot_jobs,
                       double sratch_size = 0);



    protected:

        ComputeService(const std::string &hostname,
                       std::string service_name,
                       std::string mailbox_name_prefix,
                       bool supports_standard_jobs,
                       bool supports_pilot_jobs,
                       StorageService *scratch_space = nullptr);

        /** @brief Whether the compute service supports pilot jobs */
        bool supports_pilot_jobs;
        /** @brief Whether the compute service supports standard jobs */
        bool supports_standard_jobs;

        /** @brief A scratch storage service associated to the compute service */
        StorageService *scratch_space_storage_service;

        /***********************/
        /** \endcond          **/
        /***********************/

        /***********************/
        /** \cond DEVELOPER   **/
        /***********************/

        StorageService *getScratch();

        /***********************/
        /** \endcond          **/
        /***********************/

    private:

        std::map<std::string, std::vector<double>> getServiceResourceInformation();

    };


};

#endif //SIMULATION_COMPUTESERVICE_H
