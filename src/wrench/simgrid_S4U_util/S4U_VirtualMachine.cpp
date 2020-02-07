/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include <wrench/simgrid_S4U_util/S4U_VirtualMachine.h>

#include "wrench/logging/TerminalOutput.h"
#include "wrench/simgrid_S4U_util/S4U_VirtualMachine.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include <simgrid/plugins/live_migration.h>


WRENCH_LOG_NEW_DEFAULT_CATEGORY(s4u_virtual_machine, "Log category for S4U_VirtualMachine");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param vm_name: the name of the VM
     * @param num_cores: the number of cores the VM can use
     * @param ram_memory: the VM RAM memory capacity
     * @param property_list: a property list ({} means use all defaults)
     * @param messagepayload_list: a message payload list ({} means use all defaults)
     */
    S4U_VirtualMachine::S4U_VirtualMachine(const std::string &vm_name,
                                           unsigned long num_cores, double ram_memory,
                                           std::map<std::string, std::string> property_list,
                                           std::map<std::string, double> messagepayload_list) :
            vm_name(vm_name), num_cores(num_cores), ram_memory(ram_memory),
            property_list(property_list), messagepayload_list(messagepayload_list) {

        this->state = State::DOWN;
    }

    /**
    * @brief Get the physical hostname
    * @return the physical hostname
    */
    std::string S4U_VirtualMachine::getPhysicalHostname() {
        return this->pm_name;
//        if (this->state == S4U_VirtualMachine::DOWN) {
//           throw std::runtime_error("S4U_VirtualMachine::getPhysicalHostname(): no physical host for VM '" + this->vm_name + "'");
//        } else {
//            return this->vm->get_pm()->get_name();
//        }
    }

    /**
       * @brief Get the number of cores
       * @return the number of cores
       */
    unsigned long S4U_VirtualMachine::getNumCores() {
        return this->num_cores;
    }

    /**
    * @brief Get the memory consumption
    * @return the memory consumption
    */
    double S4U_VirtualMachine::getMemory() {
        return this->ram_memory;
    }

    /**
    * @brief Start the VM
    * @param pm_name: the physical host name
    */
    void S4U_VirtualMachine::start(std::string &pm_name) {

        if (this->state != State::DOWN) {
            throw std::runtime_error(
                    "S4U_VirtualMachine::suspend(): Cannot suspend a VM that's in state " + this->getStateAsString());
        }

        simgrid::s4u::Host *physical_host = simgrid::s4u::Host::by_name_or_null(pm_name);
        if (physical_host == nullptr) {
            throw std::invalid_argument("S4U_VirtualMachine::start(): unknown physical host '" + pm_name + "'");
        }
        this->vm = new simgrid::s4u::VirtualMachine(this->vm_name,
                                                    physical_host,
                                                    (int) this->num_cores,
                                                    (size_t) this->ram_memory);
        this->vm->start();
        this->state = State::RUNNING;
        this->pm_name = pm_name;
    }

    /**
     * @brief Suspend the VM
     */
    void S4U_VirtualMachine::suspend() {
        if (this->state != State::RUNNING) {
            throw std::runtime_error(
                    "S4U_VirtualMachine::suspend(): Cannot suspend a VM that's in state " + this->getStateAsString());
        }
        this->vm->suspend();
        this->state = State::SUSPENDED;
    }

    /**
     * @brief Resume the VM
     */
    void S4U_VirtualMachine::resume() {
        if (this->state != State::SUSPENDED) {
            throw std::runtime_error(
                    "S4U_VirtualMachine::resume(): Cannot resume a VM that's in state " + this->getStateAsString());
        }
        this->vm->resume();
        this->state = State::RUNNING;
    }

    /**
     * @brief Shutdown the VM
     */
    void S4U_VirtualMachine::shutdown() {
        if (this->state == State::DOWN) {
            throw std::runtime_error(
                    "S4U_VirtualMachine::shutdown(): Cannot shutdown a VM that's in state " + this->getStateAsString());
        }
        this->state = State::DOWN; // Doing this first before a possible context switch
        this->vm->shutdown();
        this->vm->destroy();
        this->pm_name = "";
    }

    /**
     * @brief Get the VM's state
     * @return a state
     */
    S4U_VirtualMachine::State S4U_VirtualMachine::getState() {
        return this->state;
    }

    /**
     * @brief Get the VM's state as a string
     * @return a state as a string
     */
    std::string S4U_VirtualMachine::getStateAsString() {
        switch (this->state) {
            case State::DOWN:
                return "DOWN";
            case State::RUNNING:
                return "RUNNING";
            case State::SUSPENDED:
                return "SUSPENDED";
            default:
                return "???";
        }
    }

    /**
     * @brief Migrate the VM
     * @param dest_pm_name: the name of the host to which to migrate the VM
     */
    void S4U_VirtualMachine::migrate(const std::string &dest_pm_name) {

        std::string src_pm_hostname = this->vm->get_pm()->get_name();
        simgrid::s4u::Host *dest_pm = simgrid::s4u::Host::by_name_or_null(dest_pm_name);

        double mig_sta = simgrid::s4u::Engine::get_clock();
        sg_vm_migrate(this->vm, dest_pm);
        double mig_end = simgrid::s4u::Engine::get_clock();
        this->pm_name = dest_pm_name;
        WRENCH_INFO("%s migrated to %s in %g s", src_pm_hostname.c_str(), dest_pm_name.c_str(), mig_end - mig_sta);
    }

    /**
     * @brief Get the property list for the BareMetalComputeService that is to run on the VM ({} means "use all defaults")
     * @return a property list
     */
    std::map<std::string, std::string> S4U_VirtualMachine::getPropertyList() {
        return this->property_list;
    }

    /**
     * @brief Get the message payload list for the BareMetalComputeService that will run on the VM ({} means "use all defaults")
     * @return a message payload list
     */
    std::map<std::string, double> S4U_VirtualMachine::getMessagePayloadList() {
        return this->messagepayload_list;
    }

}
