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

#include "wrench/services/metering/EnergyMeterService.h"
#include "wrench/services/metering/BandwidthMeterService.h"
#include "wrench/execution_controller/ExecutionController.h"
#include "wrench/wms/DynamicOptimization.h"
#include "wrench/wms/StaticOptimization.h"
#include "wrench/wms/scheduler/PilotJobScheduler.h"
#include "wrench/wms/scheduler/StandardJobScheduler.h"
#include "wrench/services/compute/cloud/CloudComputeService.h"
#include "wrench/execution_events/CompoundJobFailedEvent.h"
#include "wrench/execution_events/CompoundJobCompletedEvent.h"
#include "wrench/execution_events/StandardJobCompletedEvent.h"
#include "wrench/execution_events/StandardJobFailedEvent.h"
#include "wrench/execution_events/PilotJobStartedEvent.h"
#include "wrench/execution_events/PilotJobExpiredEvent.h"
#include "wrench/execution_events/FileCopyCompletedEvent.h"
#include "wrench/execution_events/FileCopyFailedEvent.h"
#include "wrench/execution_events/TimerEvent.h"
#include "wrench/workflow/Workflow.h"

namespace wrench {

    class Simulation;
    class ComputeService;
    class CloudComputeService;
    class VirtualizedClusterComputeService;
    class StorageService;
    class NetworkProximityService;
    class FileRegistryService;

    /**
     * @brief A workflow management system (WMS)
     */
    class WMS : public ExecutionController {

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
            const std::set<std::shared_ptr<ComputeService>> &compute_services,
            const std::set<std::shared_ptr<StorageService>> &storage_services,
            const std::set<std::shared_ptr<NetworkProximityService>> &network_proximity_services,
            std::shared_ptr<FileRegistryService> file_registry_service,
            const std::string &hostname,
            const std::string suffix);


        void checkDeferredStart();

        void runDynamicOptimizations();

        void runStaticOptimizations();

        /**
         * @brief Obtain the list of compute services available to the WMS
         * @tparam T: ComputeService, or any if derived classes (but for CloudComputeService)
         * @return a set of compute services
         */
        template <class T>
        std::set<std::shared_ptr<T>> getAvailableComputeServices() {
            bool is_cloud = (std::type_index(typeid(T)) == std::type_index(typeid(CloudComputeService)));
            std::set<std::shared_ptr<T>> to_return;
            for (auto const &h : this->compute_services) {
                if (not is_cloud) {
                    auto shared_ptr = std::dynamic_pointer_cast<T>(h);
                    if (shared_ptr) {
                        to_return.insert(shared_ptr);
                    }
                } else {
                    auto shared_ptr_cloud = std::dynamic_pointer_cast<T>(h);
                    auto shared_ptr_vc = std::dynamic_pointer_cast<VirtualizedClusterComputeService>(h);
                    if (shared_ptr_cloud and (not shared_ptr_vc)) {
                        to_return.insert(shared_ptr_cloud);
                    }
                }
            }
            return to_return;
        }


        std::set<std::shared_ptr<StorageService>> getAvailableStorageServices();
        std::set<std::shared_ptr<NetworkProximityService>> getAvailableNetworkProximityServices();
        std::shared_ptr<FileRegistryService> getAvailableFileRegistryService();

        void waitForAndProcessNextEvent();
        bool waitForAndProcessNextEvent(double timeout);

        virtual void processEventStandardJobCompletion(std::shared_ptr<StandardJobCompletedEvent>);

        virtual void processEventStandardJobFailure(std::shared_ptr<StandardJobFailedEvent>);

        virtual void processEventPilotJobStart(std::shared_ptr<PilotJobStartedEvent>);

        virtual void processEventPilotJobExpiration(std::shared_ptr<PilotJobExpiredEvent>);

        virtual void processEventFileCopyCompletion(std::shared_ptr<FileCopyCompletedEvent>);

        virtual void processEventFileCopyFailure(std::shared_ptr<FileCopyFailedEvent>);

        virtual void processEventTimer(std::shared_ptr<TimerEvent>);

        std::shared_ptr<JobManager> createJobManager() override;
        std::shared_ptr<DataMovementManager> createDataMovementManager() override;

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
        std::set<std::shared_ptr<ComputeService>> compute_services;
        /** @brief List of available storage services */
        std::set<std::shared_ptr<StorageService>> storage_services;
        /** @brief List of available network proximity services */
        std::set<std::shared_ptr<NetworkProximityService>> network_proximity_services;
        /** @brief The file registry service */
        std::shared_ptr<FileRegistryService> file_registry_service;

        /** @brief The standard job scheduler */
        std::shared_ptr<StandardJobScheduler> standard_job_scheduler;
        /** @brief The standard job scheduler */
        std::shared_ptr<PilotJobScheduler> pilot_job_scheduler;

        /** @brief The enabled dynamic optimizations */
        std::vector<std::unique_ptr<DynamicOptimization>> dynamic_optimizations;
        /** @brief The enabled static optimizations */
        std::vector<std::unique_ptr<StaticOptimization>> static_optimizations;



        /***********************/
        /** \endcond           */
        /***********************/

//    private:
//        virtual int main() override = 0 ;

    };

};


#endif //WRENCH_WMS_H
