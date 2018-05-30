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
#include <typeindex>
#include <iostream>

#include "wrench/simulation/SimulationTimestamp.h"
#include "wrench/simulation/SimulationTrace.h"

namespace wrench {

    /**
     * @brief A class that contains post-mortem simulation-generated data
     */
    class SimulationOutput {

    public:

        /**
         * @brief Retrieve a copy of a simulation output trace
         *        once the simulation has completed
         *
         * @tparam a particular SimulationTimestampXXXX class (defined in SimulationTimestampTypes.h)
         * @return a vector of pointers to SimulationTimestampXXXX instances
         */
        template <class T> std::vector<SimulationTimestamp<T> *> getTrace() {
          std::type_index type_index  = std::type_index(typeid(T));

          // Is the trace empty?
          if (this->traces.find(type_index) == this->traces.end()) {
            return {};
          }

          std::vector<SimulationTimestamp<T> *> non_generic_vector;
          SimulationTrace<T> *trace = (SimulationTrace<T> *)(this->traces[type_index]);
          for (auto ts : trace->getTrace()) {
            non_generic_vector.push_back((SimulationTimestamp<T> *)ts);
          }
          return non_generic_vector;
        }

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        /**
         * @brief Append a simulation timestamp to a simulation output trace
         *
         * @tparam a particular SimulationTimestampXXXX class (defined in SimulationTimestampTypes.h)
         * @param timestamp: a pointer to a SimulationTimestampXXXX object
         */
        template <class T> void addTimestamp(T *timestamp) {
          std::type_index type_index = std::type_index(typeid(T));
          if (this->traces.find(type_index) == this->traces.end()) {
            this->traces[type_index] = new SimulationTrace<T>();
          }
          ((SimulationTrace<T> *)(this->traces[type_index]))->addTimestamp(new SimulationTimestamp<T>(timestamp));
        }

        /***********************/
        /** \endcond          */
        /***********************/


        /***********************/
        /** \cond          */
        /***********************/
        ~SimulationOutput() {
          for (auto t : this->traces) {
            delete t.second;
          }
          this->traces.clear();
        }
        /***********************/
        /** \endcond          */
        /***********************/

    private:
        std::map<std::type_index, GenericSimulationTrace*> traces;
    };

};


#endif //WRENCH_SIMULATIONOUTPUT_H
