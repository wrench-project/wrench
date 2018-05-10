/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef WRENCH_S4U_VIRTUALMACHINE_H
#define WRENCH_S4U_VIRTUALMACHINE_H

#include <simgrid/s4u/VirtualMachine.hpp>

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief A wrapper for the simgrid::s4u::VirtualMachine class
     */
    class S4U_VirtualMachine {

    public:
        S4U_VirtualMachine(const std::string &vm_hostname,
                           const std::string &pm_hostname,
                           unsigned long num_cores,
                           double ram_memory);

        simgrid::s4u::VirtualMachine *get();

        simgrid::s4u::Host *getPm();

        void stop();

    private:
        /** @brief a pointer to the simgrid::s4u::VirtualMachine object */
        simgrid::s4u::VirtualMachine *vm;
    };

    /***********************/
    /** \endcond           */
    /***********************/
}

#endif //WRENCH_S4U_VIRTUALMACHINE_H
