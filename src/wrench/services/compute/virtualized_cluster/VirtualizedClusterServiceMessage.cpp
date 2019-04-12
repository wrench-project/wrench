/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include "VirtualizedClusterServiceMessage.h"

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param name: the message name
     * @param payload: the message size in bytes
     */
    VirtualizedClusterServiceMessage::VirtualizedClusterServiceMessage(const std::string &name, double payload) :
            ComputeServiceMessage("VirtualizedClusterServiceMessage::" + name, payload) {
    }

    /**
     * @brief Constructor
     *
     * @param answer_mailbox: the mailbox to which to send the answer
     * @param vm_hostname: the name of the new VM host
     * @param dest_pm_hostname: the name of the destination physical machine host
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    VirtualizedClusterServiceMigrateVMRequestMessage::VirtualizedClusterServiceMigrateVMRequestMessage(
            const std::string &answer_mailbox,
            const std::string &vm_hostname,
            const std::string &dest_pm_hostname,
            double payload) :
            VirtualizedClusterServiceMessage("MIGRATE_VM_REQUEST", payload) {

        if (answer_mailbox.empty() || dest_pm_hostname.empty() || vm_hostname.empty()) {
            throw std::invalid_argument(
                    "VirtualizedClusterServiceMigrateVMRequestMessage::VirtualizedClusterServiceMigrateVMRequestMessage(): Invalid arguments");
        }
        this->answer_mailbox = answer_mailbox;
        this->vm_hostname = vm_hostname;
        this->dest_pm_hostname = dest_pm_hostname;
    }

    /**
     * @brief Constructor
     *
     * @param success: whether the VM migration was successful or not
     * @param payload: the message size in bytes
     */
    VirtualizedClusterServiceMigrateVMAnswerMessage::VirtualizedClusterServiceMigrateVMAnswerMessage(bool success,
                                                                                                     double payload) :
            VirtualizedClusterServiceMessage("MIGRATE_VM_ANSWER", payload), success(success) {}

    /**
     * @brief Constructor
     *
     * @param answer_mailbox: the mailbox to which to send the answer
     * @param pm_hostname: the name of the physical host on which to create the VM
     * @param vm_hostname: the requested VM name
     * @param num_cores: the requested number of cores
     * @param ram_memory: the requested ram
     * @param property_list: a property list
     * @param messagepayload_list: a message payload list
     * @param payload: the message payload
     */
    VirtualizedClusterServiceCreateVMRequestMessage::VirtualizedClusterServiceCreateVMRequestMessage(
            const std::string &answer_mailbox, const std::string &pm_hostname, const std::string &vm_hostname, unsigned long num_cores,
            double ram_memory, std::map<std::string, std::string> &property_list,
            std::map<std::string, std::string> &messagepayload_list, double payload) :
            VirtualizedClusterServiceMessage("CREATE_VM_REQUEST", payload), answer_mailbox(answer_mailbox),
            pm_hostname(pm_hostname), vm_hostname(vm_hostname), num_cores(num_cores),
            ram_memory(ram_memory), property_list(property_list), messagepayload_list(messagepayload_list) {}


    /**
     * @brief Constructor
     *
     * @param success: whether the VM creation was a success
     * @param cs: the VM's compute service (or nullptr if failure)
     * @param failure_cause: the failure cause (or nullptr if success)
     * @param payload: the message paylaod
     */
    VirtualizedClusterServiceCreateVMAnswerMessage::VirtualizedClusterServiceCreateVMAnswerMessage(
            bool success,
            std::shared_ptr<BareMetalComputeService> cs,
            std::shared_ptr<FailureCause> failure_cause,
            double payload) :
            VirtualizedClusterServiceMessage("CREATE_VM_ANSWER", payload),
            success(success), cs(cs), failure_cause(failure_cause) {}

}
