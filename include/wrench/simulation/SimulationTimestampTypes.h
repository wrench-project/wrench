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

namespace wrench {

    class WorkflowTask;

    /**
    * @brief A base class for simulation timestamps regarding workflow tasks
    */
    class SimulationTimestampTask {

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


    private:
        WorkflowTask *task;
    };

    class SimulationTimestampTaskStart : public SimulationTimestampTask {
    public:
        SimulationTimestampTaskStart(WorkflowTask *task) : SimulationTimestampTask(task) {

        }
    };

    class SimulationTimestampTaskFailure : public SimulationTimestampTask {
    public:
        SimulationTimestampTaskFailure(WorkflowTask *task) : SimulationTimestampTask(task) {

        }
    };

    class SimulationTimestampTaskCompletion : public SimulationTimestampTask {
    public:
        SimulationTimestampTaskCompletion(WorkflowTask *task) : SimulationTimestampTask(task) {

        }
    };

};

#endif //WRENCH_SIMULATIONTIMESTAMPTYPES_H
