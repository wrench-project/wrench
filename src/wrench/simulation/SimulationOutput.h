/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_SIMULATIONOUTPUT_H
#define WRENCH_SIMULATIONOUTPUT_H


#include <typeinfo>

#include "SimulationTimestamp.h"
#include "SimulationTrace.h"

namespace wrench {

    class SimulationOutput {

    public:

        /**
         * @brief Retrieve a simulation trace (which should be filled in with timestamps)
         *        once the simulation has completed
         *
         * @tparam T: a particular SimulationTimestampXXXX class (defined in SimulationTimestampTypes.h)
         * @return a vector of pointers to SimulationTimestampXXXX objects
         */
        template <class T> std::vector<SimulationTimestamp<T> *> getTrace() {
          std::vector<SimulationTimestamp<T> *> non_generic_vector;
          SimulationTrace<T> *trace = (SimulationTrace<T> *)(this->traces[std::type_index(typeid(T))]);
          for (auto ts : trace->getTrace()) {
            non_generic_vector.push_back((SimulationTimestamp<T> *)ts);
          }
          return non_generic_vector;
        }

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        /**
         * @brief Append a simulation timestamp to a simulation trace
         *
         * @tparam T: a particular SimulationTimestampXXXX class (defined in SimulationTimestampTypes.h)
         * @param timestamp: a pointer to a SimulationTimestampXXXX object
         */
        template <class T> void addTimestamp(T *timestamp) {
          std::type_index type_index = std::type_index(typeid(T));
          if (!(this->traces[type_index])) {
            this->traces[type_index] = new SimulationTrace<T>();
          }
          ((SimulationTrace<T> *)(this->traces[type_index]))->addTimestamp(new SimulationTimestamp<T>(timestamp));
        }

        /***********************/
        /** \endcond          */
        /***********************/


    private:
        std::map<std::type_index, GenericSimulationTrace*> traces;
    };

};


#endif //WRENCH_SIMULATIONOUTPUT_H
