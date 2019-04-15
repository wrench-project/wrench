/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include "CloudServiceMessage.h"
#include <iostream>

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param name: the message name
     * @param payload: the message size in bytes
     */
    CloudServiceMessage::CloudServiceMessage(const std::string &name, double payload) :
            ComputeServiceMessage("CloudServiceMessage::" + name, payload) {
    }

    /**
     * @brief Constructor
     *
     * @param answer_mailbox: the mailbox to which to send the answer
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    CloudServiceGetExecutionHostsRequestMessage::CloudServiceGetExecutionHostsRequestMessage(
            const std::string &answer_mailbox, double payload) : CloudServiceMessage(
            "GET_EXECUTION_HOSTS_REQUEST",
            payload) {

        if (answer_mailbox.empty()) {
            throw std::invalid_argument(
                    "CloudServiceGetExecutionHostsRequestMessage::CloudServiceGetExecutionHostsRequestMessage(): "
                    "Invalid arguments");
        }
        this->answer_mailbox = answer_mailbox;
    }

    /**
     * @brief Constructor
     *
     * @param execution_hosts: the hosts available for running virtual machines
     * @param payload: the message size in bytes
     */
    CloudServiceGetExecutionHostsAnswerMessage::CloudServiceGetExecutionHostsAnswerMessage(
            std::vector<std::string> &execution_hosts, double payload) : CloudServiceMessage(
            "GET_EXECUTION_HOSTS_ANSWER", payload), execution_hosts(execution_hosts) {}

    /**
     * @brief Constructor
     *
     * @param answer_mailbox: the mailbox to which to send the answer
     * @param num_cores: the number of cores the service can use (use ComputeService::ALL_CORES to use all cores
     *                   available on the host)
     * @param ram_memory: the VM's RAM memory capacity (use ComputeService::ALL_RAM to use all RAM available on the
     *                    host, this can be lead to an out of memory issue)
     * @param property_list: a property list for the BareMetalService that will run on the VM ({} means "use all defaults")
     * @param messagepayload_list: a message payload list for the BareMetalService that will run on the VM ({} means "use all defaults")
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    CloudServiceCreateVMRequestMessage::CloudServiceCreateVMRequestMessage(
            const std::string &answer_mailbox,
            unsigned long num_cores,
            double ram_memory,
            std::map<std::string, std::string> property_list,
            std::map<std::string, std::string> messagepayload_list,
            double payload) :
            CloudServiceMessage("CREATE_VM_REQUEST", payload),
            num_cores(num_cores), ram_memory(ram_memory), property_list(property_list), messagepayload_list(messagepayload_list)  {

        if (answer_mailbox.empty() || (ram_memory < 0.0)) {
//        std::cerr << answer_mailbox << " - " << pm_hostname << " - " << vm_name << std::endl;
            throw std::invalid_argument(
                    "CloudServiceCreateVMRequestMessage::CloudServiceCreateVMRequestMessage(): Invalid arguments");
        }
        this->answer_mailbox = answer_mailbox;
    }

    /**
     * @brief Constructor
     *
     * @param success: whether the VM creation was successful or not
     * @param vm_name: the name of the created VM (if success)
     * @param failure_cause: the cause of the failure (or nullptr if success)
     * @param payload: the message size in bytes
     */
    CloudServiceCreateVMAnswerMessage::CloudServiceCreateVMAnswerMessage(bool success,
                                                                         std::string &vm_name,
                                                                         std::shared_ptr<FailureCause> failure_cause,
                                                                         double payload) :
            CloudServiceMessage("CREATE_VM_ANSWER", payload), success(success), vm_name(vm_name), failure_cause(failure_cause) {}

    /**
     * @brief Constructor
     *
     * @param answer_mailbox: the mailbox to which to send the answer
     * @param vm_name: the name of the VM host
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    CloudServiceShutdownVMRequestMessage::CloudServiceShutdownVMRequestMessage(
            const std::string &answer_mailbox,
            const std::string &vm_name,
            double payload) :
            CloudServiceMessage("SHUTDOWN_VM_REQUEST", payload) {

        if (answer_mailbox.empty() || vm_name.empty()) {
            throw std::invalid_argument(
                    "CloudServiceShutdownVMRequestMessage::CloudServiceShutdownVMRequestMessage(): Invalid arguments");
        }
        this->answer_mailbox = answer_mailbox;
        this->vm_name = vm_name;
    }

    /**
     * @brief Constructor
     *
     * @param success: whether the VM shutdown succeeded
     * @param failure_cause: the cause of the failure (or nullptr if success)
     * @param payload: the message size in bytes
     */
    CloudServiceShutdownVMAnswerMessage::CloudServiceShutdownVMAnswerMessage(bool success,
                                                                             std::shared_ptr<FailureCause> failure_cause,
                                                                             double payload) :
            CloudServiceMessage("SHUTDOWN_VM_ANSWER", payload), success(success), failure_cause(failure_cause)  {}

    /**
     * @brief Constructor
     *
     * @param answer_mailbox: the mailbox to which to send the answer
     * @param vm_name: the name of the VM host
     * @param pm_name: the name of the physical host on which to start the VM (or "" if up to the service)
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    CloudServiceStartVMRequestMessage::CloudServiceStartVMRequestMessage(
            const std::string &answer_mailbox,
            const std::string &vm_name,
            const std::string &pm_name,
            double payload) :
            CloudServiceMessage("START_VM_REQUEST", payload) {

        if (answer_mailbox.empty() || vm_name.empty()) {
            throw std::invalid_argument(
                    "CloudServiceStartVMRequestMessage::CloudServiceStartVMRequestMessage(): Invalid arguments");
        }
        this->answer_mailbox = answer_mailbox;
        this->vm_name = vm_name;
        this->pm_name = pm_name;
    }

    /**
     * @brief Constructor
     *
     * @param success: whether the VM start succeeded
     * @param failure_cause: the cause of the failure (or nullptr if success)
     * @param payload: the message size in bytes
     */
    CloudServiceStartVMAnswerMessage::CloudServiceStartVMAnswerMessage(bool success,
                                                                       std::shared_ptr<BareMetalComputeService> cs,
                                                                       std::shared_ptr<FailureCause> failure_cause,
                                                                       double payload) :
            CloudServiceMessage("START_VM_ANSWER", payload), success(success), cs(cs), failure_cause(failure_cause) {}

    /**
     * @brief Constructor
     *
     * @param answer_mailbox: the mailbox to which to send the answer
     * @param vm_name: the name of the VM host
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    CloudServiceSuspendVMRequestMessage::CloudServiceSuspendVMRequestMessage(
            const std::string &answer_mailbox,
            const std::string &vm_name,
            double payload) :
            CloudServiceMessage("SUSPEND_VM_REQUEST", payload) {

        if (answer_mailbox.empty() || vm_name.empty()) {
            throw std::invalid_argument(
                    "CloudServiceSuspendVMRequestMessage::CloudServiceSuspendVMRequestMessage(): Invalid arguments");
        }
        this->answer_mailbox = answer_mailbox;
        this->vm_name = vm_name;
    }

    /**
     * @brief Constructor
     *
     * @param success: whether the VM suspend succeeded
     * @param failure_cause: a failure cause (or nullptr if success)
     * @param payload: the message size in bytes
     */
    CloudServiceSuspendVMAnswerMessage::CloudServiceSuspendVMAnswerMessage(bool success,
                                                                           std::shared_ptr<FailureCause> failure_cause,
                                                                           double payload) :
            CloudServiceMessage("SUSPEND_VM_ANSWER", payload), success(success), failure_cause(failure_cause) {}

    /**
     * @brief Constructor
     *
     * @param answer_mailbox: the mailbox to which to send the answer
     * @param vm_name: the name of the VM host
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    CloudServiceResumeVMRequestMessage::CloudServiceResumeVMRequestMessage(
            const std::string &answer_mailbox,
            const std::string &vm_name,
            double payload) :
            CloudServiceMessage("RESUME_VM_REQUEST", payload) {

        if (answer_mailbox.empty() || vm_name.empty()) {
            throw std::invalid_argument(
                    "CloudServiceResumeVMRequestMessage::CloudServiceResumeVMRequestMessage(): Invalid arguments");
        }
        this->answer_mailbox = answer_mailbox;
        this->vm_name = vm_name;
    }

    /**
     * @brief Constructor
     *
     * @param success: whether the VM resume succeeded
     * @param failure_cause: a failure cause (or nullptr if success)
     * @param payload: the message size in bytes
     */
    CloudServiceResumeVMAnswerMessage::CloudServiceResumeVMAnswerMessage(bool success,
                                                                         std::shared_ptr<FailureCause> failure_cause,
                                                                         double payload) :
            CloudServiceMessage("RESUME_VM_ANSWER", payload), success(success), failure_cause(failure_cause)  {}


    /**
     * @brief Constructor
     *
     * @param answer_mailbox: the mailbox to which to send the answer
     * @param vm_name: the name of the VM host
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    CloudServiceDestroyVMRequestMessage::CloudServiceDestroyVMRequestMessage(
            const std::string &answer_mailbox,
            const std::string &vm_name,
            double payload) :
            CloudServiceMessage("DESTROY_VM_REQUEST", payload) {

        if (answer_mailbox.empty() || vm_name.empty()) {
            throw std::invalid_argument(
                    "CloudServiceDestroyVMRequestMessage::CloudServiceDestroyVMRequestMessage(): Invalid arguments");
        }
        this->answer_mailbox = answer_mailbox;
        this->vm_name = vm_name;
    }

    /**
     * @brief Constructor
     *
     * @param success: whether the VM was destroyed successfully
     * @param failure_cause: a failure cause (or nullptr if success)
     * @param payload: the message size in bytes
     */
    CloudServiceDestroyVMAnswerMessage::CloudServiceDestroyVMAnswerMessage(bool success,
                                                                           std::shared_ptr<FailureCause> failure_cause,
                                                                           double payload) :
            CloudServiceMessage("DESTROY_VM_ANSWER", payload), success(success), failure_cause(failure_cause) {}




}
