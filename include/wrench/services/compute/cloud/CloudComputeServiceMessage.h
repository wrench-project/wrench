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
#include "wrench/services/compute/bare_metal/BareMetalComputeService.h"

#include "wrench/services/compute/ComputeServiceMessage.h"

namespace wrench {

    class ComputeService;

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Top-level class for messages received/sent by a CloudComputeService
     */
    class CloudComputeServiceMessage : public ComputeServiceMessage {
    protected:
        CloudComputeServiceMessage(sg_size_t payload);
    };

    /**
     * @brief A message sent to a CloudComputeService to request the list of its execution hosts
     */
    class CloudComputeServiceGetExecutionHostsRequestMessage : public CloudComputeServiceMessage {
    public:
        CloudComputeServiceGetExecutionHostsRequestMessage(S4U_CommPort *answer_commport, sg_size_t payload);

        /** @brief The commport_name to which a reply should be sent */
        S4U_CommPort *answer_commport;
    };

    /**
     * @brief A message sent by a CloudComputeService in answer to a list of execution hosts request
     */
    class CloudComputeServiceGetExecutionHostsAnswerMessage : public CloudComputeServiceMessage {
    public:
        CloudComputeServiceGetExecutionHostsAnswerMessage(std::vector<std::string> &execution_hosts, sg_size_t payload);

        /** @brief The list of execution hosts */
        std::vector<std::string> execution_hosts;
    };

    /**
     * @brief A message sent to a CloudComputeService to request a VM creation
     */
    class CloudComputeServiceCreateVMRequestMessage : public CloudComputeServiceMessage {
    public:
        CloudComputeServiceCreateVMRequestMessage(S4U_CommPort *answer_commport,
                                                  unsigned long num_cores,
                                                  sg_size_t ram_memory,
                                                  const std::string &physical_host,
                                                  WRENCH_PROPERTY_COLLECTION_TYPE property_list,
                                                  WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE messagepayload_list,
                                                  sg_size_t payload);

    public:
        /** @brief The commport_name to which the answer message should be sent */
        S4U_CommPort *answer_commport;
        /** @brief The number of cores the service can use (0 means "use as many as there are cores on the host") */
        unsigned long num_cores;
        /** @brief The VM RAM memory_manager_service capacity (0 means "use all memory_manager_service available on the host", this can be lead to out of memory_manager_service issue) */
        sg_size_t ram_memory;
        /** @brief The physical host on which the VM should be created ("" means "the service picks") */
        std::string physical_host;
        /** @brief A property list for the bare_metal_standard_jobs that will run on the VM ({} means "use all defaults") */
        WRENCH_PROPERTY_COLLECTION_TYPE property_list;
        /** @brief A message payload list for the bare_metal_standard_jobs that will run on the VM ({} means "use all defaults") */
        WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE messagepayload_list;
    };

    /**
     * @brief A message sent by a CloudComputeService in answer to a VM creation request
     */
    class CloudComputeServiceCreateVMAnswerMessage : public CloudComputeServiceMessage {
    public:
        CloudComputeServiceCreateVMAnswerMessage(bool success, std::string &vm_name,
                                                 std::shared_ptr<FailureCause> failure_cause, sg_size_t payload);

        /** @brief Whether the VM creation was successful or not */
        bool success;
        /** @brief The VM name if success */
        std::string vm_name;
        /** @brief The cause of the failure, or nullptr on success */
        std::shared_ptr<FailureCause> failure_cause;
    };

    /**
     * @brief A message sent to a CloudComputeService to request a VM shutdown
     */
    class CloudComputeServiceShutdownVMRequestMessage : public CloudComputeServiceMessage {
    public:
        CloudComputeServiceShutdownVMRequestMessage(S4U_CommPort *answer_commport,
                                                    const std::string &vm_name,
                                                    bool send_failure_notifications,
                                                    ComputeService::TerminationCause termination_cause,
                                                    sg_size_t payload);

    public:
        /** @brief The commport_name to which the answer message should be sent */
        S4U_CommPort *answer_commport;
        /** @brief The name of the new VM host */
        std::string vm_name;
        /** @brief Whether to send failure notifications */
        bool send_failure_notifications;
        /** @brief Termination cause (in case failure notifications are sent) */
        ComputeService::TerminationCause termination_cause;
    };

    /**
     * @brief A message sent by a CloudComputeService in answer to a VM shutdown request
     */
    class CloudComputeServiceShutdownVMAnswerMessage : public CloudComputeServiceMessage {
    public:
        CloudComputeServiceShutdownVMAnswerMessage(bool success, std::shared_ptr<FailureCause> failure_cause,
                                                   sg_size_t payload);

        /** @brief Whether the VM shutdown was successful or not */
        bool success;
        /** @brief The cause of the failure, or nullptr on success */
        std::shared_ptr<FailureCause> failure_cause;
    };

    /**
     * @brief A message sent to a CloudComputeService to request a VM start
     */
    class CloudComputeServiceStartVMRequestMessage : public CloudComputeServiceMessage {
    public:
        CloudComputeServiceStartVMRequestMessage(S4U_CommPort *answer_commport,
                                                 const std::string &vm_name,
                                                 sg_size_t payload);

    public:
        /** @brief The commport_name to which the answer message should be sent */
        S4U_CommPort *answer_commport;
        /** @brief The name of the VM  to start */
        std::string vm_name;
    };

    /**
     * @brief A message sent by a CloudComputeService in answer to a VM start request
     */
    class CloudComputeServiceStartVMAnswerMessage : public CloudComputeServiceMessage {
    public:
        CloudComputeServiceStartVMAnswerMessage(bool success,
                                                std::shared_ptr<BareMetalComputeService> cs,
                                                std::shared_ptr<FailureCause> failure_cause,
                                                sg_size_t payload);

        /** @brief Whether the VM start was successful or not */
        bool success;
        /** @brief The VM's compute service */
        std::shared_ptr<BareMetalComputeService> cs;
        /** @brief The failure cause (or nullptr if success) */
        std::shared_ptr<FailureCause> failure_cause;
    };

    /**
     * @brief A message sent to a CloudComputeService to request a VM suspend
     */
    class CloudComputeServiceSuspendVMRequestMessage : public CloudComputeServiceMessage {
    public:
        CloudComputeServiceSuspendVMRequestMessage(S4U_CommPort *answer_commport,
                                                   const std::string &vm_name,
                                                   sg_size_t payload);

    public:
        /** @brief The commport_name to which the answer message should be sent */
        S4U_CommPort *answer_commport;
        /** @brief The name of the new VM host */
        std::string vm_name;
    };

    /**
     * @brief A message sent by a CloudComputeService in answer to a VM suspend request
     */
    class CloudComputeServiceSuspendVMAnswerMessage : public CloudComputeServiceMessage {
    public:
        CloudComputeServiceSuspendVMAnswerMessage(bool success,
                                                  std::shared_ptr<FailureCause> failure_cause,
                                                  sg_size_t payload);

        /** @brief Whether the VM suspend was successful or not */
        bool success;
        /** @brief The failure cause (or nullptr if success) */
        std::shared_ptr<FailureCause> failure_cause;
    };

    /**
     * @brief A message sent to a CloudComputeService to request a VM resume
     */
    class CloudComputeServiceResumeVMRequestMessage : public CloudComputeServiceMessage {
    public:
        CloudComputeServiceResumeVMRequestMessage(S4U_CommPort *commport,
                                                  const std::string &vm_name,
                                                  sg_size_t payload);

    public:
        /** @brief The commport_name to which the answer message should be sent */
        S4U_CommPort *answer_commport;
        /** @brief The name of the VM host */
        std::string vm_name;
    };

    /**
     * @brief A message sent by a CloudComputeService in answer to a VM resume request
     */
    class CloudComputeServiceResumeVMAnswerMessage : public CloudComputeServiceMessage {
    public:
        CloudComputeServiceResumeVMAnswerMessage(bool success,
                                                 std::shared_ptr<FailureCause> failure_cause,
                                                 sg_size_t payload);

        /** @brief Whether the VM resume was successful or not */
        bool success;
        /** @brief The failure cause (or nullptr if success) */
        std::shared_ptr<FailureCause> failure_cause;
    };


    /**
    * @brief A message sent to a CloudComputeService to request a VM destruction
    */
    class CloudComputeServiceDestroyVMRequestMessage : public CloudComputeServiceMessage {
    public:
        CloudComputeServiceDestroyVMRequestMessage(S4U_CommPort *answer_commport,
                                                   const std::string &vm_name,
                                                   sg_size_t payload);

    public:
        /** @brief The commport_name to which the answer message should be sent */
        S4U_CommPort *answer_commport;
        /** @brief The name of the VM host */
        std::string vm_name;
    };

    /**
     * @brief A message sent by a CloudComputeService in answer to a VM destroy request
     */
    class CloudComputeServiceDestroyVMAnswerMessage : public CloudComputeServiceMessage {
    public:
        CloudComputeServiceDestroyVMAnswerMessage(bool success,
                                                  std::shared_ptr<FailureCause> failure_cause,
                                                  sg_size_t payload);

        /** @brief Whether the VM suspend was successful or not */
        bool success;
        /** @brief The failure cause (or nullptr if success) */
        std::shared_ptr<FailureCause> failure_cause;
    };

    /***********************/
    /** \endcond           */
    /***********************/
}// namespace wrench

#endif//WRENCH_CLOUDSERVICEMESSAGE_H
