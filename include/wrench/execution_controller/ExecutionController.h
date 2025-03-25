/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_EXECUTIONCONTROLLER_H
#define WRENCH_EXECUTIONCONTROLLER_H

#include "wrench/services/metering/EnergyMeterService.h"
#include "wrench/services/metering/BandwidthMeterService.h"
#include "wrench/services/Service.h"
#include "wrench/managers/data_movement_manager/DataMovementManager.h"
#include "wrench/services/compute/cloud/CloudComputeService.h"
#include "wrench/execution_events/CompoundJobFailedEvent.h"
#include "wrench/execution_events/CompoundJobCompletedEvent.h"
#include "wrench/execution_events/StandardJobCompletedEvent.h"
#include "wrench/execution_events/StandardJobFailedEvent.h"
#include "wrench/execution_events/PilotJobStartedEvent.h"
#include "wrench/execution_events/PilotJobExpiredEvent.h"
#include "wrench/execution_events/FileCopyCompletedEvent.h"
#include "wrench/execution_events/FileCopyFailedEvent.h"
#include "wrench/execution_events/FileReadCompletedEvent.h"
#include "wrench/execution_events/FileReadFailedEvent.h"
#include "wrench/execution_events/FileWriteCompletedEvent.h"
#include "wrench/execution_events/FileWriteFailedEvent.h"
#include "wrench/execution_events/TimerEvent.h"
#include "wrench/workflow/Workflow.h"

namespace wrench {

    class Simulation;

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
     * @brief An abstraction of an execution controller, i.e., a running process that interacts
     * with other services to accomplish some computational goal. The simulation will terminate
     * when all execution controllers have terminated.
     */
    class ExecutionController : public Service {


    public:
        virtual std::shared_ptr<JobManager> createJobManager();
        virtual std::shared_ptr<DataMovementManager> createDataMovementManager();
        std::shared_ptr<EnergyMeterService> createEnergyMeter(const std::map<std::string, double> &measurement_periods);
        std::shared_ptr<EnergyMeterService> createEnergyMeter(const std::vector<std::string> &hostnames, double measurement_period);
        std::shared_ptr<BandwidthMeterService> createBandwidthMeter(const std::map<std::string, double> &measurement_periods);
        std::shared_ptr<BandwidthMeterService> createBandwidthMeter(const std::vector<std::string> &link_names, double measurement_period);


        std::shared_ptr<ExecutionEvent> waitForNextEvent();
        std::shared_ptr<ExecutionEvent> waitForNextEvent(double timeout);

        void waitForAndProcessNextEvent();
        bool waitForAndProcessNextEvent(double timeout);

        virtual void processEventCompoundJobFailure(const std::shared_ptr<CompoundJobFailedEvent> &event);
        virtual void processEventCompoundJobCompletion(const std::shared_ptr<CompoundJobCompletedEvent> &event);

        virtual void processEventStandardJobCompletion(const std::shared_ptr<StandardJobCompletedEvent> &event);
        virtual void processEventStandardJobFailure(const std::shared_ptr<StandardJobFailedEvent> &event);


        virtual void processEventPilotJobStart(const std::shared_ptr<PilotJobStartedEvent> &event);
        virtual void processEventPilotJobExpiration(const std::shared_ptr<PilotJobExpiredEvent> &event);

        virtual void processEventFileCopyCompletion(const std::shared_ptr<FileCopyCompletedEvent> &event);
        virtual void processEventFileCopyFailure(const std::shared_ptr<FileCopyFailedEvent> &event);

        virtual void processEventTimer(const std::shared_ptr<TimerEvent> &event);

        void setDaemonized(bool daemonized);

        ExecutionController(
                const std::string &hostname,
                const std::string &suffix);

        void setTimer(double date, std::string message);

    protected:
    private:
        friend class Simulation;
        friend class DataMovementManager;
        friend class JobManager;

        bool daemonized_ = false;

    private:
        virtual int main() = 0;
    };

    /***********************/
    /** \endcond           */
    /***********************/

}// namespace wrench


#endif//WRENCH_EXECUTIONCONTROLLER_H
