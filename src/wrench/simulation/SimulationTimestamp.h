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

#include "SimulationTimestampTypes.h"

namespace wrench {

    template <class T> class SimulationTimestamp {

    public:

        SimulationTimestamp(T *content) {
          // TODO: Make content a unique_ptr to make memory mamangement better
          this->content = content;
        }

        double getDate() {
          return this->date;
        }

        T *getContent() {
          return this->content;
        }

    private:
        double date = -1.0;
        T *content;

    };






};


#endif //WRENCH_SIMULATIONTIMESTAMP_H
