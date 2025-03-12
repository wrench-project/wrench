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

namespace wrench {

/**
     * @brief A serverless compute service that manages a set of compute hosts and
     *        controls access to their resource via function registration/invocation operations.
     */
    class ServerlessComputeService : public ComputeService {

    private:
        WRENCH_PROPERTY_COLLECTION_TYPE default_property_values = {
            {ServerlessComputeServiceProperty::SCRATCH_SPACE_BUFFER_SIZE, "0"}
        };

    public:

        ServerlessComputeService(const std::string &hostname,
                                 std::vector<std::string> compute_hosts,
                                 std::string head_node_storage_mount_point,
                                 WRENCH_PROPERTY_COLLECTION_TYPE property_list = {},
                                 WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE messagepayload_list = {});

        bool supportsStandardJobs() override;
        bool supportsCompoundJobs() override;
        bool supportsPilotJobs() override;

    protected:

        friend class FunctionManager;

        std::shared_ptr<Invocation> invokeFunction(std::shared_ptr<Function> function, 
                                                   std::shared_ptr<FunctionInput> input, 
                                                   S4U_CommPort *notify_commport);

        bool registerFunction(std::shared_ptr<Function> function, 
                              double time_limit_in_seconds, 
                              sg_size_t disk_space_limit_in_bytes, 
                              sg_size_t RAM_limit_in_bytes, 
                              sg_size_t ingress_in_bytes, 
                              sg_size_t egress_in_bytes);

    private:

        int main() override;

        void submitCompoundJob(std::shared_ptr<CompoundJob> job,
                               const std::map<std::string, std::string> &service_specific_args) 
                               override;

        void terminateCompoundJob(std::shared_ptr<CompoundJob> job) 
                                  override;

        void processFunctionRegistrationRequest(S4U_CommPort *answer_commport, 
                                                std::shared_ptr<Function> function, double time_limit, 
                                                sg_size_t disk_space_limit_in_bytes, 
                                                sg_size_t ram_limit_in_bytes, 
                                                sg_size_t ingress_in_bytes, 
                                                sg_size_t egress_in_bytes);

        void processFunctionInvocationRequest(S4U_CommPort *answer_commport, 
                                              std::shared_ptr<Function> function, 
                                              std::shared_ptr<FunctionInput> input, 
                                              S4U_CommPort *notify_commport);

        void processImageDownloadCompletion(const std::shared_ptr<Action>& action, const std::shared_ptr<DataFile>& image_file);

        void admitInvocations();
        void scheduleInvocations();
        void dispatchInvocations();

        bool processNextMessage();

        std::map<std::string, double> constructResourceInformation(const std::string &key) override;
        std::shared_ptr<wrench::ServerlessScheduler> _scheduler;

        void startHeadStorageService();
        void startComputeHostsServices();
        void initiateImageDownloadFromRemote(const std::shared_ptr<Invocation>& invocation);
        void dispatchFunctionInvocation(const std::shared_ptr<Invocation>& invocation);
        void initiateImageCopyToComputeHost(const std::string& computeHost, std::shared_ptr<DataFile> image);
        void initiateImageRemovalFromComputeHost(const std::string& computeHost, std::shared_ptr<DataFile> image);

        std::shared_ptr<StorageService> startInvocationStorageService(const std::shared_ptr<Invocation>& invocation);


        // map of Registered functions sorted by function name
        std::map<std::string, std::shared_ptr<RegisteredFunction>> _registeredFunctions;
        // vector of compute host names
        std::vector<std::string> _compute_hosts;

        std::map<std::string, unsigned long> _available_cores;
        std::map<std::shared_ptr<Invocation>, std::string> _scheduling_decisions;

        // queue of function invocations waiting to be processed
        std::queue<std::shared_ptr<Invocation>> _newInvocations;
        // queues of function invocations whose images are being downloaded
        std::map<std::shared_ptr<DataFile>, std::queue<std::shared_ptr<Invocation>>> _admittedInvocations;
        // queue of function invocations whose images have been downloaded
        std::queue<std::shared_ptr<Invocation>> _schedulableInvocations;
        // queue of function invocations whose are scheduled on a host and whose
        // images are being copied there
        std::queue<std::shared_ptr<Invocation>> _scheduledInvocations;
        // queue of function invocations currently running
        std::queue<std::shared_ptr<Invocation>> _runningInvocations;
        // queue of function invocations that have finished executing
        std::queue<std::shared_ptr<Invocation>> _finishedInvocations;

        std::string _head_storage_service_mount_point;
        // std::vector<std::shared_ptr<BareMetalComputeService>> _compute_services;
        std::unordered_map<std::string, std::shared_ptr<StorageService>> _compute_storages;
        std::shared_ptr<StorageService> _head_storage_service;
        std::set<std::shared_ptr<DataFile>> _being_downloaded_image_files;
        std::set<std::shared_ptr<DataFile>> _downloaded_image_files;
        sg_size_t _free_space_on_head_storage; // We keep track of it ourselves to avoid concurrency shennanigans

        WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE default_messagepayload_values = {
            {ServerlessComputeServiceMessagePayload::FUNCTION_REGISTER_REQUEST_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size}
        };

    };

};

#endif //SERVERLESSCOMPUTESERVICE_H
