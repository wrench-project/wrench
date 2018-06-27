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
        SimulationTimestampType() {
            this->date = S4U_Simulation::getClock();
        }

        double getDate() {
            return this->date;
        }

        SimulationTimestampType *getEndpoint() {
            return this->endpoint;
        }

    protected:
        SimulationTimestampType *endpoint = nullptr;

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
        SimulationTimestampTask(WorkflowTask *task) {
            this->task = task;
        }

        ~SimulationTimestampTask() {

        }

        /***********************/
        /** \endcond           */
        /***********************/

        /**
         * @brief Retrieve the task that has completed
         *
         * @return the task
         */
        WorkflowTask *getTask() {
          return this->task;
        }


    protected:
        static std::map<std::string, SimulationTimestampTask* const&> pending_task_timestamps;

        void setEndpoints(WorkflowTask *task) {
            auto pending_tasks_itr = pending_task_timestamps.find(task->getID());
            if (pending_tasks_itr != pending_task_timestamps.end()) {
                (*pending_tasks_itr).second->endpoint = this;
                this->endpoint = (*pending_tasks_itr).second->endpoint;
                pending_task_timestamps.erase(pending_tasks_itr);
            }
        }

    private:
        WorkflowTask *task;
    };

    class SimulationTimestampTaskStart : public SimulationTimestampTask {
    public:
        SimulationTimestampTaskStart(WorkflowTask *task) : SimulationTimestampTask(task) {
            pending_task_timestamps.insert(std::make_pair(task->getID(), this));
        }
    };

    class SimulationTimestampTaskFailure : public SimulationTimestampTask {
    public:
        SimulationTimestampTaskFailure(WorkflowTask *task) : SimulationTimestampTask(task){
            setEndpoints(task);
        }
    };

    class SimulationTimestampTaskCompletion : public SimulationTimestampTask {
    public:
        SimulationTimestampTaskCompletion(WorkflowTask *task) : SimulationTimestampTask(task) {
            setEndpoints(task);
        }
    };

};

#endif //WRENCH_SIMULATIONTIMESTAMPTYPES_H
