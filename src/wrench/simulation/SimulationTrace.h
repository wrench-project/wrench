/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SIMULATIONTRACE_H
#define WRENCH_SIMULATIONTRACE_H


#include "SimulationTimestamp.h"


namespace wrench {

    class GenericSimulationTrace {

    };

    template <class T> class SimulationTrace : public GenericSimulationTrace  {

    public:
        void addTimestamp(SimulationTimestamp<T> *timestamp) {
          this->trace.push_back(timestamp);
        }

        std::vector<SimulationTimestamp<T> *> getTrace() {
          return this->trace;
        }

    private:
        std::vector<SimulationTimestamp<T> *> trace;

    };

};


#endif //WRENCH_SIMULATIONTRACE_H
