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


#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "wrench/simulation/SimulationTimestampTypes.h"

namespace wrench {

    /**
     * @brief A simulation-generated timestamp
     *
     * @tparam T: a particular SimulationTimestampXXXX class (defined in SimulationTimestampTypes.h)
     */
    template<class T>
    class SimulationTimestamp {

    public:

        /**
         * Retrieve the timestamp's date
         *
         * @return the date (as a number of seconds since the beginning of the simulation)
         */
        double getDate() {
          return this->date;
        }

        /**
         * Retrieve the timestamp's content
         *
         * @return a pointer to a object of class T, i.e., a particular SimulationTimestampXXXX class (defined in SimulationTimestampTypes.h)
         */
        T *getContent() {
          return this->content;
        }

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        /**
         * @brief Constructor
         * @param content: a pointer to a object of class T, i.e., a particular SimulationTimestampXXXX class (defined in SimulationTimestampTypes.h)
         */
        SimulationTimestamp(T *content) {
          // TODO: Make content a unique_ptr to make memory management better
          this->content = content;
          this->date = S4U_Simulation::getClock();
        }

        /***********************/
        /** \endcond           */
        /***********************/

    private:
        double date = -1.0;
        T *content;

    };

};

#endif //WRENCH_SIMULATIONTIMESTAMP_H
