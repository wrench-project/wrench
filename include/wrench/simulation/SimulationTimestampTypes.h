/**
* Copyright (c) 2017. The WRENCH Team.
*
* This program is free software: you can redistribute it and/or modify
        * it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/

#ifndef WRENCH_SIMULATIONTIMESTAMPTYPES_H
#define WRENCH_SIMULATIONTIMESTAMPTYPES_H

#include "wrench/workflow/WorkflowTask.h"

namespace wrench {

    class WorkflowTask;

    class SimulationTimestampType {
    public:
        SimulationTimestampType();
        virtual ~SimulationTimestampType() {}
        double getDate();
        SimulationTimestampType* getEndpoint();

    protected:
        SimulationTimestampType *endpoint = nullptr;
        virtual void setEndpoints() = 0;

    private:
        double date = -1.0;
    };

    /**
    * @brief A base class for simulation timestamps regarding workflow tasks
    */
    class SimulationTimestampTask : public SimulationTimestampType {

    public:

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        /**
         * @brief Constructor
         * @param task: a workflow task
         */
        SimulationTimestampTask(WorkflowTask *);

        /***********************/
        /** \endcond           */
        /***********************/

        /**
         * @brief Retrieve the task that has completed
         *
         * @return the task
         */
        WorkflowTask *getTask();

    protected:
        static std::map<std::string, SimulationTimestampTask *> pending_task_timestamps;
        void setEndpoints();

    private:
        WorkflowTask *task;
    };

    class SimulationTimestampTaskStart : public SimulationTimestampTask {
    public:
        SimulationTimestampTaskStart(WorkflowTask *);
    };
    class SimulationTimestampTaskFailure : public SimulationTimestampTask {
    public:
        SimulationTimestampTaskFailure(WorkflowTask *);
    };

    class SimulationTimestampTaskCompletion : public SimulationTimestampTask {
    public:
        SimulationTimestampTaskCompletion(WorkflowTask *);
    };

};

#endif //WRENCH_SIMULATIONTIMESTAMPTYPES_H
