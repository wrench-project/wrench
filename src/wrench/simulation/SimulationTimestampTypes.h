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
    * @brief A class to represent the content of a "task completion" simulation event
    */
    class SimulationTimestampTaskCompletion {

    public:

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        /**
         * @brief Constructor
         * @param task: a pointer to a WorkflowTask object
         */
        SimulationTimestampTaskCompletion(WorkflowTask *task) {
          this->task = task;
        }

        /***********************/
        /** \endcond           */
        /***********************/

        /**
         * @brief Retrieve the task that has completed
         *
         * @return a pointer to a WorkflowTask object
         */
        WorkflowTask *getTask() {
          return this->task;
        }

    private:
        WorkflowTask *task;
    };
};

#endif //WRENCH_SIMULATIONTIMESTAMPTYPES_H
