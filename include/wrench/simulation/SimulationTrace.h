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

#include <vector>
#include <map>

#include "wrench/simulation/SimulationTimestamp.h"

namespace wrench {


    /***********************/
    /** \cond              */
    /***********************/

    /** @brief A dummy top-level class derived by simulation output traces */
    class GenericSimulationTrace {

    public:
        virtual ~GenericSimulationTrace() {}

    };
    /***********************/
    /** \endcond           */
    /***********************/

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
     * @brief A template class to represent a trace of timestamps
     *
     * @tparam a particular SimulationTimestampXXXX class (defined in SimulationTimestampTypes.h)
     */
    template <class T> class SimulationTrace : public GenericSimulationTrace  {

    public:

        /**
         * @brief Append a timestamp to the trace
         *
         * @param timestamp: a pointer to a SimulationTimestamp<T> object
         * @tparam a particular SimulationTimestampXXXX class (defined in SimulationTimestampTypes.h)
         */
        void addTimestamp(SimulationTimestamp<T> *timestamp) {
          this->trace.push_back(timestamp);
        }

        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        /**
         * @brief Retrieve the trace as a vector of timestamps
         *
         * @tparam a particular SimulationTimestampXXXX class (defined in SimulationTimestampTypes.h)
         * @return a vector of pointers to SimulationTimestamp<T> objects
         */
        std::vector<SimulationTimestamp<T> *> getTrace() {
          return this->trace;
        }

        /**
         * @brief Destructor
         */
        ~SimulationTrace<T>() {
          for (auto s : this->trace) {
            delete s;
          }
        }

        /***********************/
        /** \endcond INTERNAL     */
        /***********************/

    private:
        std::vector<SimulationTimestamp<T> *> trace;

    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_SIMULATIONTRACE_H
