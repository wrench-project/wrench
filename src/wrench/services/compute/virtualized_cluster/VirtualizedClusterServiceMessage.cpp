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

}
