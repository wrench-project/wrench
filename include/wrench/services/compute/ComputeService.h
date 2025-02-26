/**
 * Copyright (c) 2017-2019. The WRENCH Team.
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
#include <limits>

#include "wrench/services/Service.h"
#include "wrench/job/Job.h"
#include "wrench/job/StandardJob.h"
#include "wrench/job/PilotJob.h"
#include "wrench/job/CompoundJob.h"

namespace wrench {

    class Simulation;

    class StorageService;

    /**
     * @brief The compute service base class
     */
    class ComputeService : public Service {

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

        friend class Simulation;

        /***********************/
        /** \endcond          **/
        /***********************/


    public:
        /** @brief A convenient constant to mean "use all cores of a physical host" whenever a number of cores
         *  is needed when instantiating compute services
         */
        static constexpr unsigned long ALL_CORES = ULONG_MAX;

        /** @brief A convenient constant to mean "use all ram of a physical host" whenever a ram capacity
         *  is needed when instantiating compute services
         */
        static constexpr sg_size_t ALL_RAM = LONG_MAX;

        /***********************/
        /** \cond DEVELOPER   **/
        /***********************/


        /**
         * @brief Job termination cause enum
         */
        enum TerminationCause {
            TERMINATION_NONE,
            TERMINATION_COMPUTE_SERVICE_TERMINATED,
            TERMINATION_JOB_KILLED,
            TERMINATION_JOB_TIMEOUT
        };


        virtual ~ComputeService() {}

        void stop() override;

        virtual void stop(bool send_failure_notifications, ComputeService::TerminationCause termination_cause);

        void terminateJob(const std::shared_ptr<CompoundJob> &job);

        /**
         * @brief Returns true if the service supports standard jobs
         * @return true or false
         */
        virtual bool supportsStandardJobs() = 0;

        /**
         * @brief Returns true if the service supports pilot jobs
         * @return true or false
         */
        virtual bool supportsCompoundJobs() = 0;

        /**
         * @brief Returns true if the service supports compound jobs
         * @return true or false
         */
        virtual bool supportsPilotJobs() = 0;

        virtual bool hasScratch() const;

        unsigned long getNumHosts(bool simulate_it = false);

        std::vector<std::string> getHosts(bool simulate_it = false);

        std::map<std::string, unsigned long> getPerHostNumCores(bool simulate_it = false);

        unsigned long getTotalNumCores(bool simulate_it = false);

        std::map<std::string, unsigned long> getPerHostNumIdleCores(bool simulate_it = false);

        virtual unsigned long getTotalNumIdleCores(bool simulate_it = false);

        virtual bool isThereAtLeastOneHostWithIdleResources(unsigned long num_cores, sg_size_t ram);

        std::map<std::string, double> getMemoryCapacity(bool simulate_it = false);

        std::map<std::string, double> getPerHostAvailableMemoryCapacity(bool simulate_it = false);

        std::map<std::string, double> getCoreFlopRate(bool simulate_it = false);

        double getTotalScratchSpaceSize() const;

        double getFreeScratchSpaceSize() const;


        /***********************/
        /** \endcond          **/
        /***********************/

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/


        /**
         * @brief Method to submit a compound job to the service
         *
         * @param job: The job being submitted
         * @param service_specific_arguments: the set of service-specific arguments
         */
        virtual void
        submitCompoundJob(std::shared_ptr<CompoundJob> job, const std::map<std::string, std::string> &service_specific_arguments) = 0;


        /**
         * @brief Method to terminate a compound job
         * @param job: the standard job
         */
        virtual void terminateCompoundJob(std::shared_ptr<CompoundJob> job) = 0;

        std::shared_ptr<StorageService> getScratch();


        ComputeService(const std::string &hostname,
                       const std::string &service_name,
                       const std::string &scratch_space_mount_point);

    protected:
        friend class JobManager;

        void submitJob(const std::shared_ptr<CompoundJob> &job, const std::map<std::string, std::string> & = {});

        virtual void validateServiceSpecificArguments(const std::shared_ptr<CompoundJob> &job,
                                                      std::map<std::string, std::string> &service_specific_args);

        virtual void validateJobsUseOfScratch(std::map<std::string, std::string> &service_specific_args);

        ComputeService(const std::string &hostname,
                       const std::string &service_name,
                       std::shared_ptr<StorageService> scratch_space);

        /** @brief A scratch storage service associated to the compute service */
        std::shared_ptr<StorageService> scratch_space_storage_service;

        void startScratchStorageService();

        /**
         * @brief Construct a dict for resource information
         * @param key: the desired key
         * @return a dictionary
         */
        virtual std::map<std::string, double> constructResourceInformation(const std::string &key) = 0;

        /***********************/
        /** \endcond          **/
        /***********************/

        /***********************/
        /** \cond DEVELOPER   **/
        /***********************/

        //        std::shared_ptr<StorageService> getScratchSharedPtr();

        /***********************/
        /** \endcond          **/
        /***********************/

    private:
        std::string scratch_space_mount_point;
        //        std::shared_ptr<StorageService> scratch_space_storage_service_shared_ptr;

        std::map<std::string, double> getServiceResourceInformation(const std::string &desired_entries, bool simulate_it);
    };


}// namespace wrench

#endif//SIMULATION_COMPUTESERVICE_H
