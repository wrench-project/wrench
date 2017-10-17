/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef WRENCH_CLOUDSERVICEMESSAGE_H
#define WRENCH_CLOUDSERVICEMESSAGE_H

#include "services/compute/ComputeServiceMessage.h"

namespace wrench {

    class ComputeService;

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Top-level CloudServiceMessage class
     */
    class CloudServiceMessage : public ComputeServiceMessage {
    protected:
        CloudServiceMessage(const std::string &name, double payload);
    };

    /**
     * @brief CloudServiceCreateVMRequestMessage class
     */
    class CloudServiceCreateVMRequestMessage : public CloudServiceMessage {
    public:
        CloudServiceCreateVMRequestMessage(const std::string &answer_mailbox,
                                           const std::string &pm_hostname,
                                           const std::string &vm_hostname,
                                           int num_cores,
                                           bool supports_standard_jobs,
                                           bool supports_pilot_jobs,
                                           std::map<std::string, std::string> &plist,
                                           double payload);

        std::string pm_hostname;
        std::string vm_hostname;
        int num_cores;
        bool supports_standard_jobs;
        bool supports_pilot_jobs;
        std::map<std::string, std::string> plist;
        std::string answer_mailbox;
    };

    /**
     * @brief CloudServiceCreateVMAnswerMessage class
     */
    class CloudServiceCreateVMAnswerMessage : public CloudServiceMessage {
    public:
        CloudServiceCreateVMAnswerMessage(const std::string &vm_hostname, double payload);

        std::string vm_hostname;
    };

    /***********************/
    /** \endcond           */
    /***********************/
}

#endif //WRENCH_CLOUDSERVICEMESSAGE_H
