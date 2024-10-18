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
        VirtualizedClusterComputeServiceMessage(sg_size_t payload);
    };


    /**
     * @brief A message sent to a VirtualizedClusterComputeService to request a VM migration
     */
    class VirtualizedClusterComputeServiceMigrateVMRequestMessage : public VirtualizedClusterComputeServiceMessage {
    public:
        VirtualizedClusterComputeServiceMigrateVMRequestMessage(S4U_CommPort *answer_commport,
                                                                const std::string &vm_name,
                                                                const std::string &dest_pm_hostname,
                                                                sg_size_t payload);

        /** @brief The name of the host on which the VM is currently executed */
        std::string vm_name;
        /** @brief The name of the host to which the VM should be migrated */
        std::string dest_pm_hostname;
        /** @brief The commport_name to which the answer message should be sent */
        S4U_CommPort *answer_commport;
    };

    /**
     * @brief A message sent by a VirtualizedClusterComputeService in answer to a VM migration request
     */
    class VirtualizedClusterComputeServiceMigrateVMAnswerMessage : public VirtualizedClusterComputeServiceMessage {
    public:
        VirtualizedClusterComputeServiceMigrateVMAnswerMessage(bool success,
                                                               std::shared_ptr<FailureCause> failure_cause,
                                                               sg_size_t payload);

        /** @brief Whether the VM migration was successful or not */
        bool success;
        /** @brief A failure cause (or nullptr if success) */
        std::shared_ptr<FailureCause> failure_cause;
    };

    /***********************/
    /** \endcond           */
    /***********************/
}// namespace wrench

#endif//WRENCH_VIRTUALIZEDCLUSTERSERVICEMESSAGE_H
