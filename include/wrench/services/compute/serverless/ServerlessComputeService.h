/**
* Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef SERVERLESSCOMPUTESERVICE_H
#define SERVERLESSCOMPUTESERVICE_H

#include <wrench/managers/function_manager/Function.h>
#include <wrench/managers/function_manager/FunctionManager.h>
#include <wrench/managers/function_manager/RegisteredFunction.h>
#include "wrench/services/compute/serverless/ServerlessComputeServiceMessagePayload.h"
#include "wrench/services/compute/serverless/Invocation.h"
#include "wrench/services/compute/ComputeService.h"
#include "wrench/services/compute/bare_metal/BareMetalComputeService.h"
#include "wrench/simgrid_S4U_util/S4U_CommPort.h"
#include "wrench/services/compute/serverless/ServerlessComputeServiceProperty.h"
#include "wrench/services/compute/serverless/ServerlessScheduler.h"
#include "wrench/services/compute/serverless/ServerlessStateOfTheSystem.h"

namespace wrench {
    /**
         * @brief A serverless compute service that manages a set of compute hosts and
         *        controls access to their resource via function registration/invocation operations.
         */
    class ServerlessComputeService : public ComputeService {
    private:
        WRENCH_PROPERTY_COLLECTION_TYPE default_property_values = {
            {ServerlessComputeServiceProperty::CONTAINER_STARTUP_OVERHEAD, "0"},
            {ServerlessComputeServiceProperty::SCRATCH_SPACE_BUFFER_SIZE, "0"}
        };

        WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE default_messagepayload_values = {
            {
                ServerlessComputeServiceMessagePayload::FUNCTION_REGISTER_REQUEST_MESSAGE_PAYLOAD,
                S4U_CommPort::default_control_message_size
            },
            {
                ServerlessComputeServiceMessagePayload::FUNCTION_REGISTER_ANSWER_MESSAGE_PAYLOAD,
                S4U_CommPort::default_control_message_size
            },
            {
                ServerlessComputeServiceMessagePayload::FUNCTION_INVOKE_REQUEST_MESSAGE_PAYLOAD,
                S4U_CommPort::default_control_message_size
            },
            {
                ServerlessComputeServiceMessagePayload::FUNCTION_INVOKE_ANSWER_MESSAGE_PAYLOAD,
                S4U_CommPort::default_control_message_size
            },
            {
                ServerlessComputeServiceMessagePayload::FUNCTION_COMPLETION_MESSAGE_PAYLOAD,
                S4U_CommPort::default_control_message_size
            },
        };

    public:
        ServerlessComputeService(const std::string& hostname,
                                 const std::vector<std::string>& compute_hosts,
                                 const std::string& head_node_storage_mount_point,
                                 const std::shared_ptr<ServerlessScheduler>& scheduler,
                                 const WRENCH_PROPERTY_COLLECTION_TYPE& property_list = {},
                                 const WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE& messagepayload_list = {});

        bool supportsStandardJobs() override;
        bool supportsCompoundJobs() override;
        bool supportsPilotJobs() override;

    protected:
        friend class FunctionManager;

        std::shared_ptr<Invocation> invokeFunction(const std::shared_ptr<RegisteredFunction>& registered_function,
                                                   const std::shared_ptr<FunctionInput>& input,
                                                   S4U_CommPort* notify_commport);

        std::shared_ptr<RegisteredFunction> registerFunction(const std::shared_ptr<Function>& function,
                                                             double time_limit_in_seconds,
                                                             sg_size_t disk_space_limit_in_bytes,
                                                             sg_size_t RAM_limit_in_bytes,
                                                             sg_size_t ingress_in_bytes,
                                                             sg_size_t egress_in_bytes);

    private:
        std::shared_ptr<ServerlessScheduler> _scheduler;
        std::shared_ptr<ServerlessStateOfTheSystem> _state_of_the_system;

        int main() override;

        void submitCompoundJob(std::shared_ptr<CompoundJob> job,
                               const std::map<std::string, std::string>& service_specific_args)
        override;

        void terminateCompoundJob(std::shared_ptr<CompoundJob> job)
        override;

        void processFunctionRegistrationRequest(S4U_CommPort* answer_commport,
                                                const std::shared_ptr<Function>& function,
                                                double time_limit,
                                                sg_size_t disk_space_limit_in_bytes,
                                                sg_size_t ram_limit_in_bytes,
                                                sg_size_t ingress_in_bytes,
                                                sg_size_t egress_in_bytes);

        void processFunctionInvocationRequest(S4U_CommPort* answer_commport,
                                              const std::shared_ptr<RegisteredFunction>& registered_function,
                                              const std::shared_ptr<FunctionInput>& input,
                                              S4U_CommPort* notify_commport);

        void processImageDownloadCompletion(const std::shared_ptr<Action>& action,
                                            const std::shared_ptr<DataFile>& image_file);

        void admitInvocations();
        std::shared_ptr<SchedulingDecisions> invokeScheduler() const;
        void initiateImageCopiesAndLoads(const std::shared_ptr<SchedulingDecisions>& decisions);
        void dispatchInvocations(const std::shared_ptr<SchedulingDecisions>& decisions);

        bool processNextMessage(bool& do_scheduling);

        std::map<std::string, double> constructResourceInformation(const std::string& key) override;


        void startHeadStorageService();
        void startComputeHostsServices();
        std::shared_ptr<StorageService> startInvocationStorageService(
            const std::shared_ptr<Invocation>& invocation,
            const std::string& target_host);

        void initiateImageDownloadFromRemote(const std::shared_ptr<Invocation>& invocation);
        void initiateImageCopyToComputeHost(const std::string& compute_host, const std::shared_ptr<DataFile>& image);
        void initiateImageLoadAtComputeHost(const std::string& computeHost, const std::shared_ptr<DataFile>& image);

        bool invocationCanBeStarted(const std::shared_ptr<Invocation>& invocation, const std::string& hostname) const;

        bool dispatchInvocation(const std::shared_ptr<Invocation>& invocation, const std::string& target_host);
    };
};

#endif //SERVERLESSCOMPUTESERVICE_H
