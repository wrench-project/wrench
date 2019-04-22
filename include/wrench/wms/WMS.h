/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_WMS_H
#define WRENCH_WMS_H

#include <wrench/managers/EnergyMeter.h>
#include "wrench/simgrid_S4U_util/S4U_Daemon.h"
#include "wrench/services/Service.h"
#include "wrench/services/compute/ComputeService.h"
#include "wrench/wms/DynamicOptimization.h"
#include "wrench/wms/StaticOptimization.h"
#include "wrench/wms/scheduler/PilotJobScheduler.h"
#include "wrench/wms/scheduler/StandardJobScheduler.h"
#include "wrench/workflow/Workflow.h"

namespace wrench {

    class Simulation;
    class ComputeService;
    class StorageService;
    class NetworkProximityService;
    class FileRegistryService;

    /**
     * @brief A workflow management system (WMS)
     */
    class WMS : public Service {

    public:
        void addWorkflow(Workflow *workflow, double start_time = 0);
        Workflow *getWorkflow();
        PilotJobScheduler *getPilotJobScheduler();
        StandardJobScheduler *getStandardJobScheduler();

        void addStaticOptimization(std::unique_ptr<StaticOptimization>);

        void addDynamicOptimization(std::unique_ptr<DynamicOptimization>);


    protected:

        /***********************/
        /** \cond DEVELOPER */
        /***********************/

        WMS(std::unique_ptr<StandardJobScheduler> standard_job_scheduler,
            std::unique_ptr<PilotJobScheduler> pilot_job_scheduler,
            const std::set<ComputeService *> &compute_services,
            const std::set<StorageService *> &storage_services,
            const std::set<NetworkProximityService *> &network_proximity_services,
            FileRegistryService *file_registry_service,
            const std::string &hostname,
            const std::string suffix);


        void checkDeferredStart();

        std::shared_ptr<JobManager> createJobManager();
        std::shared_ptr<DataMovementManager> createDataMovementManager();
        std::shared_ptr<EnergyMeter> createEnergyMeter(const std::map<std::string, double> &measurement_periods);
        std::shared_ptr<EnergyMeter> createEnergyMeter(const std::vector<std::string> &hostnames, double measurement_period);

        void runDynamicOptimizations();

        void runStaticOptimizations();

        std::set<ComputeService *> getAvailableComputeServices();
        std::set<StorageService *> getAvailableStorageServices();
        std::set<NetworkProximityService *> getAvailableNetworkProximityServices();
        FileRegistryService * getAvailableFileRegistryService();

        void waitForAndProcessNextEvent();
        bool waitForAndProcessNextEvent(double timeout);

        virtual void processEventStandardJobCompletion(std::unique_ptr<StandardJobCompletedEvent>);

        virtual void processEventStandardJobFailure(std::unique_ptr<StandardJobFailedEvent>);

        virtual void processEventPilotJobStart(std::unique_ptr<PilotJobStartedEvent>);

        virtual void processEventPilotJobExpiration(std::unique_ptr<PilotJobExpiredEvent>);

        virtual void processEventFileCopyCompletion(std::unique_ptr<FileCopyCompletedEvent>);

        virtual void processEventFileCopyFailure(std::unique_ptr<FileCopyFailedEvent>);

        /***********************/
        /** \endcond           */
        /***********************/

        /***********************/
        /** \cond INTERNAL     */
        /***********************/

    private:
        friend class Simulation;
        friend class DataMovementManager;
        friend class JobManager;

        /** @brief The workflow to execute */
        Workflow *workflow;
        /** @brief the WMS simulated start time */
        double start_time;
        /** @brief List of available compute services */
        std::set<ComputeService *> compute_services;
        /** @brief List of available storage services */
        std::set<StorageService *> storage_services;
        /** @brief List of available network proximity services */
        std::set<NetworkProximityService *> network_proximity_services;
        /** @brief The file registry service */
        FileRegistryService * file_registry_service;

        /** @brief The standard job scheduler */
        std::unique_ptr<StandardJobScheduler> standard_job_scheduler;
        /** @brief The standard job scheduler */
        std::unique_ptr<PilotJobScheduler> pilot_job_scheduler;

        /** @brief The enabled dynamic optimizations */
        std::vector<std::unique_ptr<DynamicOptimization>> dynamic_optimizations;
        /** @brief The enabled static optimizations */
        std::vector<std::unique_ptr<StaticOptimization>> static_optimizations;

        /***********************/
        /** \endcond           */
        /***********************/

    private:
        virtual int main() = 0;

    };
};


#endif //WRENCH_WMS_H
