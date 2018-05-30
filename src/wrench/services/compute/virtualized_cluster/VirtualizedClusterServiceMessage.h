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
     * @brief A message sent to a VirtualizedClusterService to request the list of its execution hosts
     */
    class VirtualizedClusterServiceGetExecutionHostsRequestMessage : public VirtualizedClusterServiceMessage {
    public:
        VirtualizedClusterServiceGetExecutionHostsRequestMessage(const std::string &answer_mailbox, double payload);

        /** @brief The mailbox to which a reply should be sent */
        std::string answer_mailbox;
    };

    /**
     * @brief A message sent by a VirtualizedClusterService in answer to a list of execution hosts request
     */
    class VirtualizedClusterServiceGetExecutionHostsAnswerMessage : public VirtualizedClusterServiceMessage {
    public:
        VirtualizedClusterServiceGetExecutionHostsAnswerMessage(std::vector<std::string> &execution_hosts,
                                                                double payload);

        /** @brief The list of execution hosts */
        std::vector<std::string> execution_hosts;
    };

    /**
     * @brief A message sent to a VirtualizedClusterService to request a VM creation
     */
    class VirtualizedClusterServiceCreateVMRequestMessage : public VirtualizedClusterServiceMessage {
    public:
        VirtualizedClusterServiceCreateVMRequestMessage(const std::string &answer_mailbox,
                                                        const std::string &pm_hostname,
                                                        const std::string &vm_hostname,
                                                        bool supports_standard_jobs,
                                                        bool supports_pilot_jobs,
                                                        unsigned long num_cores,
                                                        double ram_memory,
                                                        std::map<std::string, std::string> &property_list,
                                                        double payload);

    public:
        /** @brief The mailbox to which the answer message should be sent */
        std::string answer_mailbox;
        /** @brief The name of the physical machine host */
        std::string pm_hostname;
        /** @brief The name of the new VM host */
        std::string vm_hostname;
        /** @brief  True if the compute service should support standard jobs */
        bool supports_standard_jobs;
        /** @brief  True if the compute service should support pilot jobs */
        bool supports_pilot_jobs;
        /** @brief The number of cores the service can use (0 means "use as many as there are cores on the host") */
        unsigned long num_cores;
        /** @brief The VM RAM memory capacity (0 means "use all memory available on the host", this can be lead to out of memory issue) */
        double ram_memory;
        /** @brief A property list ({} means "use all defaults") */
        std::map<std::string, std::string> property_list;
    };

    /**
     * @brief A message sent by a VirtualizedClusterService in answer to a VM creation request
     */
    class VirtualizedClusterServiceCreateVMAnswerMessage : public VirtualizedClusterServiceMessage {
    public:
        VirtualizedClusterServiceCreateVMAnswerMessage(bool success, double payload);

        /** @brief Whether the VM creation was successful or not */
        bool success;
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
