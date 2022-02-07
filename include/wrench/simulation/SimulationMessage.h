/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef WRENCH_SIMGRIDMESSAGES_H
#define WRENCH_SIMGRIDMESSAGES_H

#include <string>
#include <map>
#include <iostream>


namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
    * @brief Top-level class to describe a message communicated by processes in the simulation
    */
    class SimulationMessage {

    public:

        SimulationMessage( double payload);
        virtual ~SimulationMessage();

        virtual std::string getName();

        /** @brief The message name */
        //std::string name;//we dont need this anymore :)
        /** @brief The message size in bytes */
        double payload;
    };


    /***********************/
    /** \endcond           */
    /***********************/

};

#endif //WRENCH_SIMGRIDMESSAGES_H
