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

#include <workflow/WorkflowTask.h>
#include "SimulationTimestamp.h"

namespace wrench {

    /**
    * @brief A class to represent the content of a "task completion" simulation event
    */
    class SimulationTimestampTaskCompletion {

    public:
        SimulationTimestampTaskCompletion(WorkflowTask *task) {
          this->task = task;
        }
        WorkflowTask *getTask() {
          return this->task;
        }
    private:
        WorkflowTask *task;
    };
};

#endif //WRENCH_SIMULATIONTIMESTAMPTYPES_H
