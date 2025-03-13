/**
 * Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

 #include <wrench/services/compute/serverless/ServerlessComputeService.h>
 #include <wrench/services/compute/serverless/ServerlessComputeServiceMessage.h>
 #include <wrench/services/compute/serverless/ServerlessComputeServiceMessagePayload.h>
 #include <wrench/services/compute/serverless/Invocation.h>
 #include <wrench/services/compute/serverless/ServerlessStateOfTheSystem.h>
 #include <wrench/managers/function_manager/Function.h>
 #include <wrench/logging/TerminalOutput.h>
 #include <wrench/exceptions/ExecutionException.h>
 #include <wrench/failure_causes/NotAllowed.h>
 #include <wrench/failure_causes/FunctionNotFound.h>
 
 #include <utility>
 
 #include "wrench/action/CustomAction.h"
 #include "wrench/services/ServiceMessage.h"
 #include "wrench/services/helper_services/action_executor/ActionExecutor.h"
 #include "wrench/services/storage/simple/SimpleStorageService.h"
 #include "wrench/simulation/Simulation.h"
#include "ServerlessStateOfTheSystem.h"
 
 WRENCH_LOG_CATEGORY(wrench_core_serverless_service, "Log category for Serverless Compute Service");
 
 namespace wrench {

    ServerlessStateOfTheSystem::ServerlessStateOfTheSystem(std::vector<std::string> compute_hosts) {
        _compute_hosts = std::move(compute_hosts);
        for (const auto& compute_host : _compute_hosts) {
            _available_cores[compute_host] = S4U_Simulation::getHostNumCores(compute_host);
        }
    }

    ServerlessStateOfTheSystem::~ServerlessStateOfTheSystem() {
        // Any necessary cleanup (if needed) goes here.
    }

    // getters
    // TODO: code commenting
    std::vector<std::string> wrench::ServerlessStateOfTheSystem::getComputeHosts() { return _compute_hosts; }
    std::map<std::string, unsigned long> getAvailableCores() { return _available_cores; }
    std::queue<std::shared_ptr<Invocation>> getNewInvocations() { return _newInvocations; }
    std::map<std::shared_ptr<DataFile>, std::queue<std::shared_ptr<Invocation>>> getAdmittedInvocations() { return _admittedInvocations; }
    std::queue<std::shared_ptr<Invocation>> getScheduableInvocations() { return _schedulableInvocations; }
    std::queue<std::shared_ptr<Invocation>> getScheduledInvocations() { return _scheduledInvocations; }
    std::queue<std::shared_ptr<Invocation>> getRunningInvocations() { return _runningInvocations; }
    std::queue<std::shared_ptr<Invocation>> getFinishedInvocations() { return _finishedInvocations; }
    std::unordered_map<std::string, std::shared_ptr<StorageService>> getComputeStorages() { return _compute_storages; }
    std::shared_ptr<StorageService> getHeadStorageService() { return _head_storage_service; }
    std::set<std::shared_ptr<DataFile>> getDownloadedImageFiles() { return _downloaded_image_files; }
    sg_size_t getFreeSpaceOnHeadStorage() { return _free_space_on_head_storage; }

 }; // namespace wrench
 