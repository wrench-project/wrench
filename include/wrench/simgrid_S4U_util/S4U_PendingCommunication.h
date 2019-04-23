/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_S4U_PENDINGCOMMUNICATION_H
#define WRENCH_S4U_PENDINGCOMMUNICATION_H


#include <vector>
#include <simgrid/s4u/Comm.hpp>
#include <wrench/util/MessageManager.h>

//#include "S4U_PendingCommunication.h"

namespace wrench {

    class SimulationMessage;

    /*******************/
    /** \cond INTERNAL */
    /*******************/

    /** @brief This is a simple wrapper class around S4U asynchronous communication checking methods */
    class S4U_PendingCommunication {
    public:
        S4U_PendingCommunication(std::string mailbox);

        std::unique_ptr<SimulationMessage> wait();

        static unsigned long waitForSomethingToHappen(
                std::vector<std::unique_ptr<S4U_PendingCommunication>> pending_comms,
                double timeout);

        static unsigned long waitForSomethingToHappen(
                std::vector<S4U_PendingCommunication*> pending_comms,
                double timeout);

        ~S4U_PendingCommunication();

        /** @brief The SimGrid communication handle */
        simgrid::s4u::CommPtr comm_ptr;
        /** @brief The message */
        std::unique_ptr<SimulationMessage> simulation_message;
        /** @brief The mailbox name */
        std::string mailbox_name;
    };

    /*******************/
    /** \endcond */
    /*******************/

};


#endif //WRENCH_S4U_PENDINGCOMMUNICATION_H
