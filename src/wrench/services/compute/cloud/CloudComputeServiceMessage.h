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
        CloudComputeServiceMessage(const std::string &name, double payload);
    };

    /**
     * @brief A message sent to a CloudComputeService to request the list of its execution hosts
     */
    class CloudComputeServiceGetExecutionHostsRequestMessage : public CloudComputeServiceMessage {
    public:
        CloudComputeServiceGetExecutionHostsRequestMessage(simgrid::s4u::Mailbox *answer_mailbox, double payload);

        /** @brief The mailbox to which a reply should be sent */
        simgrid::s4u::Mailbox *answer_mailbox;
    };

    /**
     * @brief A message sent by a CloudComputeService in answer to a list of execution hosts request
     */
    class CloudComputeServiceGetExecutionHostsAnswerMessage : public CloudComputeServiceMessage {
    public:
        CloudComputeServiceGetExecutionHostsAnswerMessage(std::vector<std::string> &execution_hosts, double payload);

        /** @brief The list of execution hosts */
        std::vector<std::string> execution_hosts;
    };

    /**
     * @brief A message sent to a CloudComputeService to request a VM creation
     */
    class CloudComputeServiceCreateVMRequestMessage : public CloudComputeServiceMessage {
    public:
        CloudComputeServiceCreateVMRequestMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                                  unsigned long num_cores,
                                                  double ram_memory,
                                                  std::string desired_vm_name,
                                                  std::unordered_map<std::string, std::string> property_list,
                                                  std::unordered_map<std::string, double> messagepayload_list,
                                                  double payload);

    public:
        /** @brief The mailbox to which the answer message should be sent */
        simgrid::s4u::Mailbox *answer_mailbox;
        /** @brief The number of cores the service can use (0 means "use as many as there are cores on the host") */
        unsigned long num_cores;
        /** @brief The VM RAM memory_manager_service capacity (0 means "use all memory_manager_service available on the host", this can be lead to out of memory_manager_service issue) */
        double ram_memory;
        /** @brief The desired name for the VM ("" means "pick for me") */
        std::string desired_vm_name;
        /** @brief A property list for the bare_metal_standard_jobs that will run on the VM ({} means "use all defaults") */
        std::unordered_map<std::string, std::string> property_list;
        /** @brief A message payload list for the bare_metal_standard_jobs that will run on the VM ({} means "use all defaults") */
        std::unordered_map<std::string, double> messagepayload_list;
    };

    /**
     * @brief A message sent by a CloudComputeService in answer to a VM creation request
     */
    class CloudComputeServiceCreateVMAnswerMessage : public CloudComputeServiceMessage {
    public:
        CloudComputeServiceCreateVMAnswerMessage(bool success, std::string &vm_name,
                                                 std::shared_ptr<FailureCause> failure_cause, double payload);

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
        CloudComputeServiceShutdownVMRequestMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                                    const std::string &vm_name,
                                                    bool send_failure_notifications,
                                                    ComputeService::TerminationCause termination_cause,
                                                    double payload);

    public:
        /** @brief The mailbox to which the answer message should be sent */
        simgrid::s4u::Mailbox *answer_mailbox;
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
                                                   double payload);

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
        CloudComputeServiceStartVMRequestMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                                 const std::string &vm_name,
                                                 const std::string &pm_name,
                                                 double payload);

    public:
        /** @brief The mailbox to which the answer message should be sent */
        simgrid::s4u::Mailbox *answer_mailbox;
        /** @brief The name of the VM  to start */
        std::string vm_name;
        /** @brief The name of the physical host on which to start the VM (or "" if up to the service") */
        std::string pm_name;
    };

    /**
     * @brief A message sent by a CloudComputeService in answer to a VM start request
     */
    class CloudComputeServiceStartVMAnswerMessage : public CloudComputeServiceMessage {
    public:
        CloudComputeServiceStartVMAnswerMessage(bool success,
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
     * @brief A message sent to a CloudComputeService to request a VM suspend
     */
    class CloudComputeServiceSuspendVMRequestMessage : public CloudComputeServiceMessage {
    public:
        CloudComputeServiceSuspendVMRequestMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                                   const std::string &vm_name,
                                                   double payload);

    public:
        /** @brief The mailbox to which the answer message should be sent */
        simgrid::s4u::Mailbox *answer_mailbox;
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
                                                  double payload);

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
        CloudComputeServiceResumeVMRequestMessage(simgrid::s4u::Mailbox *mailbox,
                                                  const std::string &vm_name,
                                                  double payload);

    public:
        /** @brief The mailbox to which the answer message should be sent */
        simgrid::s4u::Mailbox *answer_mailbox;
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
                                                 double payload);

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
        CloudComputeServiceDestroyVMRequestMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                                   const std::string &vm_name,
                                                   double payload);

    public:
        /** @brief The mailbox to which the answer message should be sent */
        simgrid::s4u::Mailbox *answer_mailbox;
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
