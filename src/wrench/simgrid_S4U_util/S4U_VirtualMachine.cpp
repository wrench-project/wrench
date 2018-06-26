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

      this->vm = new simgrid::s4u::VirtualMachine(vm_hostname.c_str(),
                                                  simgrid::s4u::Host::by_name(pm_hostname),
                                                  num_cores,
                                                  ram_memory);

      this->vm->set_ramsize(ram_memory);
      this->vm->start();
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
     * @brief Stop the virtual machine
     */
    void S4U_VirtualMachine::stop() {
      this->vm->destroy();
    }
}
