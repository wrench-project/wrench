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
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    VirtualizedClusterServiceGetExecutionHostsRequestMessage::VirtualizedClusterServiceGetExecutionHostsRequestMessage(
            const std::string &answer_mailbox, double payload) : VirtualizedClusterServiceMessage(
            "GET_EXECUTION_HOSTS_REQUEST",
            payload) {

      if (answer_mailbox.empty()) {
        throw std::invalid_argument(
                "VirtualizedClusterServiceGetExecutionHostsRequestMessage::VirtualizedClusterServiceGetExecutionHostsRequestMessage(): "
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
    VirtualizedClusterServiceGetExecutionHostsAnswerMessage::VirtualizedClusterServiceGetExecutionHostsAnswerMessage(
            std::vector<std::string> &execution_hosts, double payload) : VirtualizedClusterServiceMessage(
            "GET_EXECUTION_HOSTS_ANSWER", payload), execution_hosts(execution_hosts) {}

    /**
     * @brief Constructor
     *
     * @param answer_mailbox: the mailbox to which to send the answer
     * @param pm_hostname: the name of the physical machine host
     * @param vm_hostname: the name of the new VM host
     * @param supports_standard_jobs: true if the compute service should support standard jobs
     * @param supports_pilot_jobs: true if the compute service should support pilot jobs
     * @param num_cores: the number of cores the service can use (0 means "use as many as there are cores on the host")
     * @param ram_memory: the VM RAM memory capacity (0 means "use all memory available on the host", this can be lead to out of memory issue)
     * @param plist: a property list ({} means "use all defaults")
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    VirtualizedClusterServiceCreateVMRequestMessage::VirtualizedClusterServiceCreateVMRequestMessage(
            const std::string &answer_mailbox,
            const std::string &pm_hostname,
            const std::string &vm_hostname,
            bool supports_standard_jobs,
            bool supports_pilot_jobs,
            unsigned long num_cores,
            double ram_memory,
            std::map<std::string, std::string> &plist,
            double payload) :
            VirtualizedClusterServiceMessage("CREATE_VM_REQUEST", payload),
            supports_standard_jobs(supports_standard_jobs),
            supports_pilot_jobs(supports_pilot_jobs), num_cores(num_cores), ram_memory(ram_memory), plist(plist) {

      if (answer_mailbox.empty() || pm_hostname.empty() || vm_hostname.empty()) {
        throw std::invalid_argument(
                "VirtualizedClusterServiceCreateVMRequestMessage::VirtualizedClusterServiceCreateVMRequestMessage(): Invalid arguments");
      }
      this->answer_mailbox = answer_mailbox;
      this->pm_hostname = pm_hostname;
      this->vm_hostname = vm_hostname;
    }

    /**
     * @brief Constructor
     *
     * @param success: whether the VM creation was successfull or not
     * @param payload: the message size in bytes
     */
    VirtualizedClusterServiceCreateVMAnswerMessage::VirtualizedClusterServiceCreateVMAnswerMessage(bool success,
                                                                                                   double payload) :
            VirtualizedClusterServiceMessage("CREATE_VM_ANSWER", payload), success(success) {}

}
