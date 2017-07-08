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

namespace wrench {

    class SimulationMessage;

    /** \cond INTERNAL */

    /** @brief This is a simple wrapper class around S4U */
    class S4U_PendingCommunication {
    public:
        S4U_PendingCommunication();

        std::unique_ptr<SimulationMessage> wait();

        static unsigned long waitForSomethingToHappen(
                std::vector<std::unique_ptr<S4U_PendingCommunication>> *pending_comms);

        simgrid::s4u::CommPtr comm_ptr;
        SimulationMessage *simulation_message;
    };

    /** \endcond */

};


#endif //WRENCH_S4U_PENDINGCOMMUNICATION_H
