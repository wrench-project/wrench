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

 WRENCH_LOG_CATEGORY(wrench_core_serverless_state_of_the_system, "Log category for Serverless State of the System");
 
 namespace wrench {

    ServerlessStateOfTheSystem::ServerlessStateOfTheSystem(const std::vector<std::string>& compute_hosts)
        : _compute_hosts(compute_hosts),
          _head_storage_service(nullptr),
          _free_space_on_head_storage(0) {
        for (const auto& compute_host : _compute_hosts) {
            _available_cores[compute_host] = S4U_Simulation::getHostNumCores(compute_host);
        }

        for (const auto& compute_host : _compute_hosts) {
            _being_copied_images[compute_host] = {};
            _copied_images[compute_host] = {};
        }
    }

    ServerlessStateOfTheSystem::~ServerlessStateOfTheSystem() = default;

    // getters
    // TODO: code commenting
    const std::vector<std::string>& ServerlessStateOfTheSystem::getComputeHosts() { return _compute_hosts; }
    std::map<std::string, unsigned long> ServerlessStateOfTheSystem::getAvailableCores() { return _available_cores; }
    std::queue<std::shared_ptr<Invocation>> ServerlessStateOfTheSystem::getNewInvocations() { return _newInvocations; }
    std::map<std::shared_ptr<DataFile>, std::queue<std::shared_ptr<Invocation>>> ServerlessStateOfTheSystem::getAdmittedInvocations() { return _admittedInvocations; }
    std::queue<std::shared_ptr<Invocation>> ServerlessStateOfTheSystem::getScheduableInvocations() { return _schedulableInvocations; }
    std::queue<std::shared_ptr<Invocation>> ServerlessStateOfTheSystem::getScheduledInvocations() { return _scheduledInvocations; }
    std::queue<std::shared_ptr<Invocation>> ServerlessStateOfTheSystem::getRunningInvocations() { return _runningInvocations; }
    std::queue<std::shared_ptr<Invocation>> ServerlessStateOfTheSystem::getFinishedInvocations() { return _finishedInvocations; }
    std::unordered_map<std::string, std::shared_ptr<StorageService>> ServerlessStateOfTheSystem::getComputeStorages() { return _compute_storages; }
    std::shared_ptr<StorageService> ServerlessStateOfTheSystem::getHeadStorageService() { return _head_storage_service; }
    std::set<std::shared_ptr<DataFile>> ServerlessStateOfTheSystem::getDownloadedImageFiles() { return _downloaded_image_files; }
    sg_size_t ServerlessStateOfTheSystem::getFreeSpaceOnHeadStorage() { return _free_space_on_head_storage; }
    
    std::set<std::shared_ptr<DataFile>> ServerlessStateOfTheSystem::getImagesBeingCopiedToNode(const std::string& node) {
        if (_being_copied_images.find(node) != _being_copied_images.end()) {
            return _being_copied_images[node];
        }
        return {};
    }
    
    std::set<std::shared_ptr<DataFile>> ServerlessStateOfTheSystem::getImagesCopiedToNode(const std::string& node) {
        if (_copied_images.find(node) != _copied_images.end()) {
            return _copied_images[node];
        }
        return {};
    }
    
    bool ServerlessStateOfTheSystem::isImageOnNode(const std::string& node, const std::shared_ptr<DataFile>& image) {
        // Check if the image has been copied to the node
        if (_copied_images.find(node) != _copied_images.end()) {
            if (_copied_images[node].find(image) != _copied_images[node].end()) {
                return true;
            }
        }
        
        return false;
    }

 }; // namespace wrench
 