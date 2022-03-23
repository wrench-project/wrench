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


#include <iostream>
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "wrench/simulation/SimulationTimestampTypes.h"

namespace wrench {

    /**
     * @brief A time-stamped simulation event stored in SimulationOutput
     *
     * @tparam a particular SimulationTimestampXXXX class (defined in SimulationTimestampTypes.h)
     */
    template<class T>
    class SimulationTimestamp {

    public:
        /**
         * Retrieve the timestamp's content
         *
         * @return a pointer to a object of class T, i.e., a particular SimulationTimestampXXXX class (defined in SimulationTimestampTypes.h)
         */
        T *const getContent() {
            return this->content.get();
        }

        /**
         * Retrieve the recorded time of the timestamp
         *
         * @return the recorded time of the timestamp
         */
        double getDate() {
            return this->getContent()->getDate();
        }
        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        /**
         * @brief Constructor
         * @param content: a pointer to a object of class T, i.e., a particular SimulationTimestampXXXX class (defined in SimulationTimestampTypes.h)
         */
        SimulationTimestamp(T *content) {
            this->content = std::unique_ptr<T>(content);
        }

        /***********************/
        /** \endcond           */
        /***********************/

    private:
        std::unique_ptr<T> content;
    };

};// namespace wrench

#endif//WRENCH_SIMULATIONTIMESTAMP_H
