/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include "wrench/services/compute/virtualized_cluster/VirtualizedClusterComputeServiceMessage.h"

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param name: the message name
     * @param payload: the message size in bytes
     */
    VirtualizedClusterComputeServiceMessage::VirtualizedClusterComputeServiceMessage(const std::string &name,
                                                                                     double payload) :
            ComputeServiceMessage(payload) {
    }

    /**
     * @brief Constructor
     *
     * @param answer_mailbox: the mailbox to which to send the answer
     * @param vm_name: the name of the new VM host
     * @param dest_pm_hostname: the name of the destination physical machine host
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    VirtualizedClusterComputeServiceMigrateVMRequestMessage::VirtualizedClusterComputeServiceMigrateVMRequestMessage(
            simgrid::s4u::Mailbox *answer_mailbox,
            const std::string &vm_name,
            const std::string &dest_pm_hostname,
            double payload) :
            VirtualizedClusterComputeServiceMessage("MIGRATE_VM_REQUEST", payload) {

        if ((answer_mailbox == nullptr) || dest_pm_hostname.empty() || vm_name.empty()) {
            throw std::invalid_argument(
                    "VirtualizedClusterComputeServiceMigrateVMRequestMessage::VirtualizedClusterComputeServiceMigrateVMRequestMessage(): Invalid arguments");
        }
        this->answer_mailbox = answer_mailbox;
        this->vm_name = vm_name;
        this->dest_pm_hostname = dest_pm_hostname;
    }

    /**
     * @brief Constructor
     *
     * @param success: whether the VM migration was successful or not
     * @param failure_cause: a failure cause (or nullptr if success)
     * @param payload: the message size in bytes
     */
    VirtualizedClusterComputeServiceMigrateVMAnswerMessage::VirtualizedClusterComputeServiceMigrateVMAnswerMessage(
            bool success,
            std::shared_ptr<FailureCause> failure_cause,
            double payload) :
            VirtualizedClusterComputeServiceMessage("MIGRATE_VM_ANSWER", payload), success(success),
            failure_cause(failure_cause) {}


}
