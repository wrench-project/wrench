/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef WRENCH_CLOUDSERVICEMESSAGE_H
#define WRENCH_CLOUDSERVICEMESSAGE_H

#include <vector>
#include <wrench/services/compute/bare_metal/BareMetalComputeService.h>

#include "wrench/services/compute/ComputeServiceMessage.h"

namespace wrench {

    class ComputeService;

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Top-level class for messages received/sent by a CloudService
     */
    class CloudServiceMessage : public ComputeServiceMessage {
    protected:
        CloudServiceMessage(const std::string &name, double payload);
    };

    /**
     * @brief A message sent to a CloudService to request the list of its execution hosts
     */
    class CloudServiceGetExecutionHostsRequestMessage : public CloudServiceMessage {
    public:
        CloudServiceGetExecutionHostsRequestMessage(const std::string &answer_mailbox, double payload);

        /** @brief The mailbox to which a reply should be sent */
        std::string answer_mailbox;
    };

    /**
     * @brief A message sent by a CloudService in answer to a list of execution hosts request
     */
    class CloudServiceGetExecutionHostsAnswerMessage : public CloudServiceMessage {
    public:
        CloudServiceGetExecutionHostsAnswerMessage(std::vector<std::string> &execution_hosts, double payload);

        /** @brief The list of execution hosts */
        std::vector<std::string> execution_hosts;
    };

    /**
     * @brief A message sent to a CloudService to request a VM creation
     */
    class CloudServiceCreateVMRequestMessage : public CloudServiceMessage {
    public:
        CloudServiceCreateVMRequestMessage(const std::string &answer_mailbox,
                                           unsigned long num_cores,
                                           double ram_memory,
                                           double payload);

    public:
        /** @brief The mailbox to which the answer message should be sent */
        std::string answer_mailbox;
        /** @brief The number of cores the service can use (0 means "use as many as there are cores on the host") */
        unsigned long num_cores;
        /** @brief The VM RAM memory capacity (0 means "use all memory available on the host", this can be lead to out of memory issue) */
        double ram_memory;
    };

    /**
     * @brief A message sent by a CloudService in answer to a VM creation request
     */
    class CloudServiceCreateVMAnswerMessage : public CloudServiceMessage {
    public:
        CloudServiceCreateVMAnswerMessage(bool success, std::string &vm_name, std::shared_ptr<FailureCause> failure_cause, double payload);

        /** @brief Whether the VM creation was successful or not */
        bool success;
        /** @brief The VM name if success */
        std::string vm_name;
        /** @brief The cause of the failure, or nullptr on success */
        std::shared_ptr<FailureCause> failure_cause;
    };

    /**
     * @brief A message sent to a CloudService to request a VM shutdown
     */
    class CloudServiceShutdownVMRequestMessage : public CloudServiceMessage {
    public:
        CloudServiceShutdownVMRequestMessage(const std::string &answer_mailbox,
                                             const std::string &vm_name,
                                             double payload);

    public:
        /** @brief The mailbox to which the answer message should be sent */
        std::string answer_mailbox;
        /** @brief The name of the new VM host */
        std::string vm_name;
    };

    /**
     * @brief A message sent by a CloudService in answer to a VM shutdown request
     */
    class CloudServiceShutdownVMAnswerMessage : public CloudServiceMessage {
    public:
        CloudServiceShutdownVMAnswerMessage(bool success, std::shared_ptr<FailureCause> failure_cause, double payload);

        /** @brief Whether the VM shutdown was successful or not */
        bool success;
        /** @brief The cause of the failure, or nullptr on success */
        std::shared_ptr<FailureCause> failure_cause;
    };

    /**
     * @brief A message sent to a CloudService to request a VM start
     */
    class CloudServiceStartVMRequestMessage : public CloudServiceMessage {
    public:
        CloudServiceStartVMRequestMessage(const std::string &answer_mailbox,
                                          const std::string &vm_name,
                                          const std::string &pm_name,
                                          double payload);

    public:
        /** @brief The mailbox to which the answer message should be sent */
        std::string answer_mailbox;
        /** @brief The name of the VM  to start */
        std::string vm_name;
        /** @brief The name of the physical host on which to start the VM (or "" if up to the service") */
        std::string pm_name;
    };

    /**
     * @brief A message sent by a CloudService in answer to a VM start request
     */
    class CloudServiceStartVMAnswerMessage : public CloudServiceMessage {
    public:
        CloudServiceStartVMAnswerMessage(bool success,
                                         std::shared_ptr<BareMetalComputeService> cs,
                                         std::shared_ptr<FailureCause> failure_cause,
                                         double payload);

        /** @brief Whether the VM start was successful or not */
        bool success;
        /** @brief The VM's compute service */
        std::shared_ptr<BareMetalComputeService> cs;
        /** @brief The failure cause (or nullptr if success) */
        std::shared_ptr<FailureCause> failure_cause;
    };

    /**
     * @brief A message sent to a CloudService to request a VM suspend
     */
    class CloudServiceSuspendVMRequestMessage : public CloudServiceMessage {
    public:
        CloudServiceSuspendVMRequestMessage(const std::string &answer_mailbox,
                                            const std::string &vm_name,
                                            double payload);

    public:
        /** @brief The mailbox to which the answer message should be sent */
        std::string answer_mailbox;
        /** @brief The name of the new VM host */
        std::string vm_name;
    };

    /**
     * @brief A message sent by a CloudService in answer to a VM suspend request
     */
    class CloudServiceSuspendVMAnswerMessage : public CloudServiceMessage {
    public:
        CloudServiceSuspendVMAnswerMessage(bool success,
                                           std::shared_ptr<FailureCause> failure_cause,
                                           double payload);

        /** @brief Whether the VM suspend was successful or not */
        bool success;
        /** @brief The failure cause (or nullptr if success) */
        std::shared_ptr<FailureCause> failure_cause;
    };

    /**
     * @brief A message sent to a CloudService to request a VM resume
     */
    class CloudServiceResumeVMRequestMessage : public CloudServiceMessage {
    public:
        CloudServiceResumeVMRequestMessage(const std::string &answer_mailbox,
                                           const std::string &vm_name,
                                           double payload);

    public:
        /** @brief The mailbox to which the answer message should be sent */
        std::string answer_mailbox;
        /** @brief The name of the VM host */
        std::string vm_name;
    };

    /**
     * @brief A message sent by a CloudService in answer to a VM resume request
     */
    class CloudServiceResumeVMAnswerMessage : public CloudServiceMessage {
    public:
        CloudServiceResumeVMAnswerMessage(bool success,
                                          std::shared_ptr<FailureCause> failure_cause,
                                          double payload);

        /** @brief Whether the VM resume was successful or not */
        bool success;
        /** @brief The failure cause (or nullptr if success) */
        std::shared_ptr<FailureCause> failure_cause;
    };






    /**
    * @brief A message sent to a CloudService to request a VM destruction
    */
    class CloudServiceDestroyVMRequestMessage : public CloudServiceMessage {
    public:
        CloudServiceDestroyVMRequestMessage(const std::string &answer_mailbox,
                                            const std::string &vm_name,
                                            double payload);

    public:
        /** @brief The mailbox to which the answer message should be sent */
        std::string answer_mailbox;
        /** @brief The name of the VM host */
        std::string vm_name;
    };

    /**
     * @brief A message sent by a CloudService in answer to a VM destroy request
     */
    class CloudServiceDestroyVMAnswerMessage : public CloudServiceMessage {
    public:
        CloudServiceDestroyVMAnswerMessage(bool success,
                                           std::shared_ptr<FailureCause> failure_cause,
                                           double payload);

        /** @brief Whether the VM suspend was successful or not */
        bool success;
        /** @brief The failure cause (or nullptr if success) */
        std::shared_ptr<FailureCause> failure_cause;
    };

    /***********************/
    /** \endcond           */
    /***********************/
}

#endif //WRENCH_CLOUDSERVICEMESSAGE_H
