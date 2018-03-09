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

#include "wrench/simgrid_S4U_util/S4U_Daemon.h"
#include "wrench/wms/DynamicOptimization.h"
#include "wrench/wms/StaticOptimization.h"
#include "wrench/wms/scheduler/PilotJobScheduler.h"
#include "wrench/wms/scheduler/StandardJobScheduler.h"
#include "wrench/workflow/Workflow.h"
#include "wrench/workflow/execution_events/WorkflowExecutionEvent.h"
//#include "wrench/managers/DataMovementManager.h"
//#include "wrench/managers/JobManager.h"

namespace wrench {

    class Simulation;

    /**
     * @brief A top-level class that defines a workflow management system (WMS)
     */
    class WMS : public S4U_Daemon {

    public:
        void addWorkflow(Workflow *workflow, double start_time = 0);
        Workflow *getWorkflow();


        void addStaticOptimization(std::unique_ptr<StaticOptimization>);

        void addDynamicOptimization(std::unique_ptr<DynamicOptimization>);

        /***********************/
        /** \cond DEVELOPER */
        /***********************/

        std::string getHostname();

        /***********************/
        /** \endcond           */
        /***********************/

    protected:

        /***********************/
        /** \cond DEVELOPER */
        /***********************/

        WMS(std::unique_ptr<StandardJobScheduler> standard_job_scheduler,
            std::unique_ptr<PilotJobScheduler> pilot_job_scheduler,
            const std::set<ComputeService *> &compute_services,
            const std::set<StorageService *> &storage_services,
            const std::string &hostname,
            const std::string suffix);

        void start(std::shared_ptr<WMS> this_service);

        void checkDeferredStart();

        std::shared_ptr<JobManager> createJobManager();

        std::shared_ptr<DataMovementManager> createDataMovementManager();

        void runDynamicOptimizations();

        void runStaticOptimizations();

        std::set<ComputeService *> getRunningComputeServices();

        void waitForAndProcessNextEvent();

        // workflow execution event processors
        virtual void processEventStandardJobCompletion(std::unique_ptr<WorkflowExecutionEvent>);

        virtual void processEventStandardJobFailure(std::unique_ptr<WorkflowExecutionEvent>);

        virtual void processEventPilotJobStart(std::unique_ptr<WorkflowExecutionEvent>);

        virtual void processEventPilotJobExpiration(std::unique_ptr<WorkflowExecutionEvent>);

        virtual void processEventFileCopyCompletion(std::unique_ptr<WorkflowExecutionEvent>);

        virtual void processEventFileCopyFailure(std::unique_ptr<WorkflowExecutionEvent>);

        /***********************/
        /** \endcond           */
        /***********************/

        /***********************/
        /** \cond INTERNAL     */
        /***********************/

        friend class Simulation;
        friend class DataMovementManager;
        friend class JobManager;

        /** @brief The current simulation */
        Simulation *simulation;
        /** @brief The workflow to execute */
        Workflow *workflow;
        /** @brief the WMS simulated start time */
        double start_time;
        /** @brief List of available compute services */
        std::set<ComputeService *> compute_services;
        /** @brief List of available storage services */
        std::set<StorageService *> storage_services;

        /** @brief The standard job scheduler */
        std::unique_ptr<StandardJobScheduler> standard_job_scheduler = nullptr;
        /** @brief The standard job scheduler */
        std::unique_ptr<PilotJobScheduler> pilot_job_scheduler = nullptr;

        /** @brief The enabled dynamic optimizations */
        std::vector<std::unique_ptr<DynamicOptimization>> dynamic_optimizations;
        /** @brief The enabled static optimizations */
        std::vector<std::unique_ptr<StaticOptimization>> static_optimizations;

        void setSimulation(Simulation *simulation);

        /***********************/
        /** \endcond           */
        /***********************/

    private:
        virtual int main() = 0;

        std::string hostname;
    };
};


#endif //WRENCH_WMS_H
