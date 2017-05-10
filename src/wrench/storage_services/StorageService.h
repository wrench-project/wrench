/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_STORAGESERVICE_H
#define WRENCH_STORAGESERVICE_H


#include <string>

namespace wrench {

    /***********************/
    /** \cond DEVELOPER   **/
    /***********************/

    class Simulation;  // Forward ref

    /**
     * @brief Abstract implementation of a storage service.
     */
    class StorageService {

    public:


        virtual void stop();

        std::string getName();

        bool isUp();

        virtual double getCapacity() = 0;

        virtual double getFreeSpace() = 0;

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

        enum State {
            UP,
            DOWN,
        };

        StorageService(std::string service_name);

        void setStateToDown();

    protected:

        friend class Simulation;
        void setSimulation(Simulation *simulation);

        StorageService::State state;
        std::string service_name;
        Simulation *simulation;  // pointer to the simulation object

    private:


    };

    /***********************/
    /** \endcond           */
    /***********************/

};




#endif //WRENCH_STORAGESERVICE_H
