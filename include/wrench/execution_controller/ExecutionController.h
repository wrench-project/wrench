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
#include "wrench/managers/DataMovementManager.h"
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

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
     * @brief An abstraction of an execution controller, i.e., a running process that interacts
     * with other services to accomplish some computational goal
     */
    class ExecutionController : public Service {


    public:
        virtual std::shared_ptr<JobManager> createJobManager();
        virtual std::shared_ptr<DataMovementManager> createDataMovementManager();
        std::shared_ptr<EnergyMeterService> createEnergyMeter(const std::map<std::string, double> &measurement_periods);
        std::shared_ptr<EnergyMeterService> createEnergyMeter(const std::vector<std::string> &hostnames, double measurement_period);
        std::shared_ptr<BandwidthMeterService> createBandwidthMeter(const std::map<std::string, double> &measurement_periods);
        std::shared_ptr<BandwidthMeterService> createBandwidthMeter(const std::vector<std::string> &linknames, double measurement_period);


        std::shared_ptr<ExecutionEvent> waitForNextEvent();
        std::shared_ptr<ExecutionEvent> waitForNextEvent(double timeout);

        void waitForAndProcessNextEvent();
        bool waitForAndProcessNextEvent(double timeout);

        virtual void processEventCompoundJobFailure(std::shared_ptr<CompoundJobFailedEvent>);
        virtual void processEventCompoundJobCompletion(std::shared_ptr<CompoundJobCompletedEvent>);

        virtual void processEventStandardJobCompletion(std::shared_ptr<StandardJobCompletedEvent>);
        virtual void processEventStandardJobFailure(std::shared_ptr<StandardJobFailedEvent>);


        virtual void processEventPilotJobStart(std::shared_ptr<PilotJobStartedEvent>);
        virtual void processEventPilotJobExpiration(std::shared_ptr<PilotJobExpiredEvent>);

        virtual void processEventFileCopyCompletion(std::shared_ptr<FileCopyCompletedEvent>);
        virtual void processEventFileCopyFailure(std::shared_ptr<FileCopyFailedEvent>);

        virtual void processEventTimer(std::shared_ptr<TimerEvent>);

    protected:
        ExecutionController(
                const std::string &hostname,
                const std::string& suffix);


        void setTimer(double date, std::string message);


    private:
        friend class Simulation;
        friend class DataMovementManager;
        friend class JobManager;

    private:
        virtual int main() = 0;
    };

    /***********************/
    /** \endcond           */
    /***********************/

};// namespace wrench


#endif//WRENCH_EXECUTIONCONTROLLER_H
