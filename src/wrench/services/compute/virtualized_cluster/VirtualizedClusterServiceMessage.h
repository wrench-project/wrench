/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef WRENCH_VIRTUALIZEDCLUSTERSERVICEMESSAGE_H
#define WRENCH_VIRTUALIZEDCLUSTERSERVICEMESSAGE_H

#include <vector>

#include "wrench/services/compute/ComputeServiceMessage.h"

namespace wrench {

    class ComputeService;

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Top-level class for messages received/sent by a VirtualizedClusterService
     */
    class VirtualizedClusterServiceMessage : public ComputeServiceMessage {
    protected:
        VirtualizedClusterServiceMessage(const std::string &name, double payload);
    };

    /**
     * @brief A message sent to a VirtualizedClusterService to request a VM migration
     */
    class VirtualizedClusterServiceMigrateVMRequestMessage : public VirtualizedClusterServiceMessage {
    public:
        VirtualizedClusterServiceMigrateVMRequestMessage(const std::string &answer_mailbox,
                                                         const std::string &vm_hostname,
                                                         const std::string &dest_pm_hostname,
                                                         double payload);

        /** @brief The name of the host on which the VM is currently executed */
        std::string vm_hostname;
        /** @brief The name of the host to which the VM should be migrated */
        std::string dest_pm_hostname;
        /** @brief The mailbox to which the answer message should be sent */
        std::string answer_mailbox;
    };

    /**
     * @brief A message sent by a VirtualizedClusterService in answer to a VM migration request
     */
    class VirtualizedClusterServiceMigrateVMAnswerMessage : public VirtualizedClusterServiceMessage {
    public:
        VirtualizedClusterServiceMigrateVMAnswerMessage(bool success, double payload);

        /** @brief Whether the VM migration was successful or not */
        bool success;
    };

    /***********************/
    /** \endcond           */
    /***********************/
}

#endif //WRENCH_VIRTUALIZEDCLUSTERSERVICEMESSAGE_H
