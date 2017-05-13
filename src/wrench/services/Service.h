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


    class Service : public S4U_DaemonWithMailbox {

    public:

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        virtual void stop();

        /***********************/
        /** \endcond           */
        /***********************/

    protected:

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

        Service(std::string process_name_prefix, std::string mailbox_name_prefix);

        // Property stuff
        void setProperty(std::string, std::string);
        std::string getPropertyValueAsString(std::string);
        double getPropertyValueAsDouble(std::string);

    protected:
        std::map<std::string, std::string> property_list;

        enum State {
            UP,
            DOWN,
        };

        State state;

    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_SERVICE_H
