/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_SERVICE_H
#define WRENCH_SERVICE_H


#include <string>
#include <map>

#include <simgrid_S4U_util/S4U_DaemonWithMailbox.h>

namespace wrench {

    class Simulation;

    /**
     * @brief A top-level service class
     */
    class Service : public S4U_DaemonWithMailbox {

    public:

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        /** @brief Service states */
        enum State {
            UP,
            DOWN,
        };

        virtual void stop();
        std::string getHostname();
        bool isUp();

        std::string getPropertyValueAsString(std::string);
        double getPropertyValueAsDouble(std::string);

        /***********************/
        /** \endcond           */
        /***********************/

        /***********************/
        /** \cond INTERNAL     */
        /***********************/

        void setStateToDown();
        void setSimulation(Simulation *simulation);

        /***********************/
        /** \endcond           */
        /***********************/

    protected:

        /***********************/
        /** \cond INTERNAL     */
        /***********************/

        friend class Simulation;

        Service(std::string process_name_prefix, std::string mailbox_name_prefix);

        // Property stuff
        void setProperty(std::string, std::string);
        /** @brief The service's property list */
        std::map<std::string, std::string> property_list;

        /** @brief The service's state */
        State state;

        /** @brief The service's name */
        std::string name;

        /** @brief The name of the host that runs the service */
        std::string hostname;

        /** @brief The simulation */
        Simulation *simulation;

        /***********************/
        /** \endcond           */
        /***********************/

    };
};


#endif //WRENCH_SERVICE_H
