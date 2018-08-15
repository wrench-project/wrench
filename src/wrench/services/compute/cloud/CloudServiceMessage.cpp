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
     * @param pm_hostname: the name of the physical machine host
     * @param vm_hostname: the name of the new VM host
     * @param num_cores: the number of cores the service can use (use ComputeService::ALL_CORES to use all cores
     *                   available on the host)
     * @param ram_memory: the VM's RAM memory capacity (use ComputeService::ALL_RAM to use all RAM available on the
     *                    host, this can be lead to an out of memory issue)
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    CloudServiceCreateVMRequestMessage::CloudServiceCreateVMRequestMessage(
            const std::string &answer_mailbox,
            const std::string &pm_hostname,
            const std::string &vm_hostname,
            unsigned long num_cores,
            double ram_memory,
            std::map<std::string, std::string> &property_list,
            std::map<std::string, std::string> &messagepayload_list,
            double payload) :
            CloudServiceMessage("CREATE_VM_REQUEST", payload),
            num_cores(num_cores), ram_memory(ram_memory),
            property_list(property_list), messagepayload_list(messagepayload_list) {

      if (answer_mailbox.empty() || pm_hostname.empty() || vm_hostname.empty()) {
        throw std::invalid_argument(
                "CloudServiceCreateVMRequestMessage::CloudServiceCreateVMRequestMessage(): Invalid arguments");
      }
      this->answer_mailbox = answer_mailbox;
      this->pm_hostname = pm_hostname;
      this->vm_hostname = vm_hostname;
    }

    /**
     * @brief Constructor
     *
     * @param success: whether the VM creation was successful or not
     * @param payload: the message size in bytes
     */
    CloudServiceCreateVMAnswerMessage::CloudServiceCreateVMAnswerMessage(bool success,
                                                                         double payload) :
            CloudServiceMessage("CREATE_VM_ANSWER", payload), success(success) {}

}
