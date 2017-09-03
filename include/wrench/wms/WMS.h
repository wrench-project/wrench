/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_WMS_H
#define WRENCH_WMS_H

#include "wrench/simgrid_S4U_util/S4U_Daemon.h"
#include "wrench/wms/scheduler/Scheduler.h"
#include "DynamicOptimization.h"
#include "StaticOptimization.h"
#include "wrench/wms/scheduler/PilotJobScheduler.h"
#include "wrench/workflow/Workflow.h"
#include "wrench/workflow/WorkflowExecutionEvent.h"

namespace wrench {

    class Simulation;

    /**
     * @brief A top-level class that defines a workflow management system
     */
    class WMS : public S4U_Daemon {

    public:
        void addStaticOptimization(std::unique_ptr<StaticOptimization>);

        void addDynamicOptimization(std::unique_ptr<DynamicOptimization>);

        void setPilotJobScheduler(std::unique_ptr<PilotJobScheduler>);

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

        WMS(Workflow *workflow,
            std::unique_ptr<Scheduler> scheduler,
            std::string hostname,
            std::string suffix);

        void runDynamicOptimizations();

        void runStaticOptimizations();

        void waitForAndProcessNextEvent();

        // workflow execution event processors
        virtual void processEventUnsupportedJobType(std::unique_ptr<WorkflowExecutionEvent>);

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

        /** @brief The current simulation */
        Simulation *simulation;
        /** @brief The workflow to execute */
        Workflow *workflow;

        /** @brief The selected scheduler */
        std::unique_ptr<Scheduler> scheduler;
        /** @brief The pilot job scheduler */
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
