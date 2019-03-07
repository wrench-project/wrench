/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include "wrench/simgrid_S4U_util/S4U_VirtualMachine.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param vm_hostname: the name of the VM host
     * @param pm_hostname: the name of the physical machine host
     * @param num_cores: the number of cores the VM can use
     * @param ram_memory: the VM RAM memory capacity
     */
    S4U_VirtualMachine::S4U_VirtualMachine(const std::string &vm_hostname, const std::string &pm_hostname,
                                           unsigned long num_cores, double ram_memory) {

        this->vm = new simgrid::s4u::VirtualMachine(vm_hostname,
                                                    simgrid::s4u::Host::by_name(pm_hostname),
                                                    (int)num_cores,
                                                    (size_t)ram_memory);

        // Henri commented out the call below because The RAM size is passed to the constructor above...
        // this->vm->set_ramsize((size_t)ram_memory);
        start();
    }

    /**
     * @brief Get a pointer to the simgrid::s4u::VirtualMachine object
     *
     * @return a pointer to the simgrid::s4u::VirtualMachine object
     */
    simgrid::s4u::VirtualMachine *S4U_VirtualMachine::get() {
        return this->vm;
    }

    /**
     * @brief Get a pointer to the physical machine host
     * @return a pointer to the physical machine host
     */
    simgrid::s4u::Host *S4U_VirtualMachine::getPm() {
        return this->vm->get_pm();
    }

    /**
     * @brief Start the virtual machine
     */
    void S4U_VirtualMachine::start() {
        this->vm->start();
    }

    /**
     * @brief Suspend the virtual machine
     */
    void S4U_VirtualMachine::suspend() {
        this->vm->suspend();
    }

    /**
     * @brief Resume the virtual machine
     */
    void S4U_VirtualMachine::resume() {
        this->vm->resume();
    }

    /**
     * @brief Shutdown the virtual machine
     */
    void S4U_VirtualMachine::shutdown() {
        this->vm->shutdown();
    }

    /**
     * @brief Stop the virtual machine
     */
    void S4U_VirtualMachine::stop() {
        this->vm->destroy();
    }

    /**
     * @brief Check whether the VM is running
     * @return True if the VM is running, false otherwise
     */
    bool S4U_VirtualMachine::isRunning() {
        return this->vm->get_state() == simgrid::s4u::VirtualMachine::state::RUNNING;
    }

    /**
     * @brief Check whether the VM is suspended
     * @return True if the VM is suspended, false otherwise
     */
    bool S4U_VirtualMachine::isSuspended() {
        return this->vm->get_state() == simgrid::s4u::VirtualMachine::state::SUSPENDED;
    }
}
