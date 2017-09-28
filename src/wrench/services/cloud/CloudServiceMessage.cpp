/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include "services/cloud/CloudServiceMessage.h"

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param name: the message name
     * @param payload: the message size in bytes
     */
    CloudServiceMessage::CloudServiceMessage(std::string name, double payload) :
            ComputeServiceMessage("CloudServiceMessage::" + name, payload) {
    }

    /**
     * @brief Constructor
     *
     * @param answer_mailbox: the mailbox to which to send the answer
     * @param pm_hostname: the name of the physical machine host
     * @param vm_hostname: the name of the new VM host
     * @param num_cores: the number of cores the service can use (0 means "use as many as there are cores on the host")
     * @param supports_standard_jobs: true if the compute service should support standard jobs
     * @param supports_pilot_jobs: true if the compute service should support pilot jobs
     * @param plist: a property list ({} means "use all defaults")
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    CloudServiceCreateVMRequestMessage::CloudServiceCreateVMRequestMessage(const std::string &answer_mailbox,
                                                                           const std::string &pm_hostname,
                                                                           const std::string &vm_hostname,
                                                                           int num_cores,
                                                                           bool supports_standard_jobs,
                                                                           bool supports_pilot_jobs,
                                                                           std::map<std::string, std::string> &plist,
                                                                           double payload) :
            CloudServiceMessage("CREATE_VM_REQUEST", payload), num_cores(num_cores),
            supports_standard_jobs(supports_standard_jobs), supports_pilot_jobs(supports_pilot_jobs), plist(plist) {

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
     * @param success: whether the VM creation succeeded
     * @param payload: the message size in bytes
     */
    CloudServiceCreateVMAnswerMessage::CloudServiceCreateVMAnswerMessage(
            bool success, double payload) :
            CloudServiceMessage("CREATE_VM_ANSWER", payload), success(success) {}
}
