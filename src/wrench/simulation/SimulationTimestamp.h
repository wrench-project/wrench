/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SIMULATIONTIMESTAMP_H
#define WRENCH_SIMULATIONTIMESTAMP_H


#include <workflow/WorkflowTask.h>
#include <simgrid_S4U_util/S4U_Simulation.h>

namespace wrench {

    /**
     * @brief A class to represent simulation events
     */
    struct SimulationTimestamp {

    public:
        enum Type {
            UNDEFINED,
            TASK_COMPLETION,
        };

        SimulationTimestamp(SimulationTimestamp::Type type) {
          this->date = S4U_Simulation::getClock();
          this->type = type;
        }

        SimulationTimestamp(SimulationTimestamp::Type type, WorkflowTask *task) {
          this->date = S4U_Simulation::getClock();
          this->type = type;
          this->task = task;
        }

        double getDate() {
          return this->date;
        }

        SimulationTimestamp::Type getType() {
          return this->type;
        }

        WorkflowTask *getTask() {
          return this->task;
        }

    private:
        double date = -1.0;
        SimulationTimestamp::Type type = SimulationTimestamp::Type::UNDEFINED;
        WorkflowTask *task = nullptr;

    };



};


#endif //WRENCH_SIMULATIONTIMESTAMP_H
