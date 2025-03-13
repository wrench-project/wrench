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
 
 WRENCH_LOG_CATEGORY(wrench_core_serverless_service, "Log category for Serverless Compute Service");
 
 namespace wrench {

    ServerlessStateOfTheSystem::ServerlessStateOfTheSystem(std::vector<std::string> compute_hosts) {

        _compute_hosts = std::move(compute_hosts);
        for (const auto& compute_host : _compute_hosts) {
            _available_cores[compute_host] = S4U_Simulation::getHostNumCores(compute_host);
        }
    }
 
 }; // namespace wrench
 
     
 