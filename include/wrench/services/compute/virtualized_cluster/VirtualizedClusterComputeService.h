/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef WRENCH_VIRTUALIZEDCLUSTERSERVICE_H
#define WRENCH_VIRTUALIZEDCLUSTERSERVICE_H

#include "wrench/services/compute/virtualized_cluster/VirtualizedClusterComputeServiceProperty.h"
#include "wrench/services/compute/virtualized_cluster/VirtualizedClusterComputeServiceMessagePayload.h"
#include "wrench/services/compute/cloud/CloudComputeService.h"
#include "wrench/simgrid_S4U_util/S4U_CommPort.h"

namespace wrench {

    class Simulation;

    class ComputeService;

    class CloudComputeService;

    /**
     * @brief A  virtualized cluster-based compute service
     */
    class VirtualizedClusterComputeService : public CloudComputeService {
    private:
        WRENCH_PROPERTY_COLLECTION_TYPE default_property_values = {
                {VirtualizedClusterComputeServiceProperty::VM_BOOT_OVERHEAD, "0"},
                {VirtualizedClusterComputeServiceProperty::SCRATCH_SPACE_BUFFER_SIZE, "0"}};

        WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE default_messagepayload_values = {
                {VirtualizedClusterComputeServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {VirtualizedClusterComputeServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {VirtualizedClusterComputeServiceMessagePayload::RESOURCE_DESCRIPTION_REQUEST_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {VirtualizedClusterComputeServiceMessagePayload::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {VirtualizedClusterComputeServiceMessagePayload::GET_EXECUTION_HOSTS_REQUEST_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {VirtualizedClusterComputeServiceMessagePayload::GET_EXECUTION_HOSTS_ANSWER_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {VirtualizedClusterComputeServiceMessagePayload::CREATE_VM_REQUEST_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {VirtualizedClusterComputeServiceMessagePayload::CREATE_VM_ANSWER_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {VirtualizedClusterComputeServiceMessagePayload::MIGRATE_VM_REQUEST_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {VirtualizedClusterComputeServiceMessagePayload::MIGRATE_VM_ANSWER_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {VirtualizedClusterComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {VirtualizedClusterComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {VirtualizedClusterComputeServiceMessagePayload::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {VirtualizedClusterComputeServiceMessagePayload::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {VirtualizedClusterComputeServiceMessagePayload::IS_THERE_AT_LEAST_ONE_HOST_WITH_AVAILABLE_RESOURCES_REQUEST_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {VirtualizedClusterComputeServiceMessagePayload::IS_THERE_AT_LEAST_ONE_HOST_WITH_AVAILABLE_RESOURCES_ANSWER_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
        };

    public:
        VirtualizedClusterComputeService(const std::string &hostname,
                                         std::vector<std::string> &execution_hosts,
                                         const std::string &scratch_space_mount_point,
                                         WRENCH_PROPERTY_COLLECTION_TYPE property_list = {},
                                         WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list = {});

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        using CloudComputeService::createVM;

        virtual std::string createVM(unsigned long num_cores,
                                     double ram_memory,
                                     const std::string &pm_name,
                                     WRENCH_PROPERTY_COLLECTION_TYPE property_list = {},
                                     WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list = {});

        virtual void migrateVM(const std::string &vm_name, const std::string &dest_pm_hostname);

        /***********************/
        /** \endcond          **/
        /***********************/

    protected:
        /***********************/
        /** \cond INTERNAL    */
        /***********************/

        friend class Simulation;

        int main() override;

        bool processNextMessage() override;

        virtual void processMigrateVM(S4U_CommPort *answer_commport,
                                      const std::string &vm_name,
                                      const std::string &dest_pm_hostname);

        /***********************/
        /** \endcond           */
        /***********************/
    };

}// namespace wrench

#endif//WRENCH_VIRTUALIZEDCLUSTERSERVICE_H
