/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef WRENCH_VIRTUALIZEDCLUSTERSERVICE_H
#define WRENCH_VIRTUALIZEDCLUSTERSERVICE_H

#include "wrench/services/compute/cloud/CloudComputeService.h"
#include "wrench/services/compute/virtualized_cluster/VirtualizedClusterComputeServiceProperty.h"
#include "wrench/services/compute/virtualized_cluster/VirtualizedClusterComputeServiceMessagePayload.h"


namespace wrench {

    class Simulation;

    class ComputeService;

    /**
     * @brief A  virtualized cluster-based compute service
     */
    class VirtualizedClusterComputeService : public CloudComputeService {

    private:
        std::map<std::string, std::string> default_property_values = {
                {VirtualizedClusterComputeServiceProperty::VM_BOOT_OVERHEAD_IN_SECONDS, "0.0"}
        };

        std::map<std::string, double> default_messagepayload_values = {
                {VirtualizedClusterComputeServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD,                  1024},
                {VirtualizedClusterComputeServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD,               1024},
                {VirtualizedClusterComputeServiceMessagePayload::RESOURCE_DESCRIPTION_REQUEST_MESSAGE_PAYLOAD, 1024},
                {VirtualizedClusterComputeServiceMessagePayload::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD,  1024},
                {VirtualizedClusterComputeServiceMessagePayload::GET_EXECUTION_HOSTS_REQUEST_MESSAGE_PAYLOAD,  1024},
                {VirtualizedClusterComputeServiceMessagePayload::GET_EXECUTION_HOSTS_ANSWER_MESSAGE_PAYLOAD,   1024},
                {VirtualizedClusterComputeServiceMessagePayload::CREATE_VM_REQUEST_MESSAGE_PAYLOAD,            1024},
                {VirtualizedClusterComputeServiceMessagePayload::CREATE_VM_ANSWER_MESSAGE_PAYLOAD,             1024},
                {VirtualizedClusterComputeServiceMessagePayload::MIGRATE_VM_REQUEST_MESSAGE_PAYLOAD,           1024},
                {VirtualizedClusterComputeServiceMessagePayload::MIGRATE_VM_ANSWER_MESSAGE_PAYLOAD,            1024},
                {VirtualizedClusterComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD,  1024},
                {VirtualizedClusterComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD,   1024},
                {VirtualizedClusterComputeServiceMessagePayload::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD,     1024},
                {VirtualizedClusterComputeServiceMessagePayload::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD,      1024}
        };

    public:
        VirtualizedClusterComputeService(const std::string &hostname,
                                  std::vector<std::string> &execution_hosts,
                                  double scratch_space_size,
                                  std::map<std::string, std::string> property_list = {},
                                  std::map<std::string, double> messagepayload_list = {});

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        using CloudComputeService::startVM;

        virtual std::shared_ptr<BareMetalComputeService> startVM(const std::string &vm_name, const std::string &pm_name);

        virtual void migrateVM(const std::string &vm_hostname, const std::string &dest_pm_hostname);

        /***********************/
        /** \endcond          **/
        /***********************/

    protected:

        /***********************/
        /** \cond INTERNAL    */
        /***********************/

        friend class Simulation;

        int main() override;

        virtual bool processNextMessage() override;

        virtual void processMigrateVM(const std::string &answer_mailbox,
                                      const std::string &vm_name,
                                      const std::string &dest_pm_hostname);

        /***********************/
        /** \endcond           */
        /***********************/
    };

}

#endif //WRENCH_VIRTUALIZEDCLUSTERSERVICE_H
