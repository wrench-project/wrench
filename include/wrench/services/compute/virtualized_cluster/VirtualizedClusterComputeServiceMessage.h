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

#include "../../../../../../../../../../Library/Developer/CommandLineTools/SDKs/MacOSX12.1.sdk/usr/include/c++/v1/vector"

#include "../ComputeServiceMessage.h"

namespace wrench {

    class BareMetalComputeService;

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Top-level class for messages received/sent by a VirtualizedClusterComputeService
     */
    class VirtualizedClusterComputeServiceMessage : public ComputeServiceMessage {
    protected:
        VirtualizedClusterComputeServiceMessage(const std::string &name, double payload);
    };


    /**
     * @brief A message sent to a VirtualizedClusterComputeService to request a VM migration
     */
    class VirtualizedClusterComputeServiceMigrateVMRequestMessage : public VirtualizedClusterComputeServiceMessage {
    public:
        VirtualizedClusterComputeServiceMigrateVMRequestMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                                                const std::string &vm_name,
                                                                const std::string &dest_pm_hostname,
                                                                double payload);

        /** @brief The name of the host on which the VM is currently executed */
        std::string vm_name;
        /** @brief The name of the host to which the VM should be migrated */
        std::string dest_pm_hostname;
        /** @brief The mailbox to which the answer message should be sent */
        simgrid::s4u::Mailbox *answer_mailbox;
    };

    /**
     * @brief A message sent by a VirtualizedClusterComputeService in answer to a VM migration request
     */
    class VirtualizedClusterComputeServiceMigrateVMAnswerMessage : public VirtualizedClusterComputeServiceMessage {
    public:
        VirtualizedClusterComputeServiceMigrateVMAnswerMessage(bool success,
                                                               std::shared_ptr<FailureCause> failure_cause,
                                                               double payload);

        /** @brief Whether the VM migration was successful or not */
        bool success;
        /** @brief A failure cause (or nullptr if success) */
        std::shared_ptr<FailureCause> failure_cause;

    };

    /***********************/
    /** \endcond           */
    /***********************/
}

#endif //WRENCH_VIRTUALIZEDCLUSTERSERVICEMESSAGE_H
