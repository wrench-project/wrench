/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <memory>
#include <string>
#include <boost/algorithm/string/split.hpp>
#include <utility>

#include "wrench/exceptions/ExecutionException.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/managers/function_manager/FunctionManager.h"
#include "wrench/services/compute/ComputeService.h"
#include "wrench/services/ServiceMessage.h"
#include "wrench/services/compute/ComputeServiceMessage.h"
#include "wrench/simgrid_S4U_util/S4U_CommPort.h"
#include "wrench/simulation/SimulationMessage.h"
#include "wrench/workflow/WorkflowTask.h"
#include "wrench/job/StandardJob.h"
#include "wrench/job/CompoundJob.h"
#include "wrench/job/PilotJob.h"
#include "wrench/services/helper_services/action_executor/ActionExecutor.h"
#include "wrench/services/helper_services/action_execution_service/ActionExecutionService.h"
#include "wrench/managers/job_manager/JobManagerMessage.h"

WRENCH_LOG_CATEGORY(wrench_core_function_manager, "Log category for Function Manager");

namespace wrench {


	   /**
     * @brief Main method of the daemon that implements the JobManager
     * @return 0 on success
     */
    int FunctionManager::main() {
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_YELLOW);

        WRENCH_INFO("New Function Manager starting (%s)", this->commport->get_cname());

        return 0;
    }

    /**
     * @brief Constructor
     *
     * @param hostname: the name of host on which the job manager will run
     * @param creator_commport: the commport of the manager's creator
     */
    FunctionManager::FunctionManager(const std::string& hostname, S4U_CommPort *creator_commport) : Service(hostname, "function_manager") {
        this->creator_commport = creator_commport;
    }

    void FunctionManager::stop() {
        // Implementation of stop logic, e.g., cleanup resources or notify shutdown
        this->Service::stop();
    }
    /**
     * @brief Destructor, which kills the daemon (and clears all the jobs)
     */
    FunctionManager::~FunctionManager() {
    	// Any necessary cleanup (if needed) goes here.
	}


    /**
     * 
     */
    std::shared_ptr<Function> FunctionManager::createFunction(const std::string& name,
            const std::function<std::string(const std::string&, const std::shared_ptr<StorageService>&)>& lambda,
            const std::shared_ptr<FileLocation>& image,
            const std::shared_ptr<FileLocation>& code) {
        // Create the notion of a function
        return std::make_shared<Function>(name, lambda, image, code);
    }

    /**
     * 
     */
      bool FunctionManager::registerFunction(
        const Function function,
        const std::shared_ptr<ServerlessComputeService>& compute_service,
        int time_limit_in_seconds,
        long disk_space_limit_in_bytes,
        long RAM_limit_in_bytes,
        long ingress_in_bytes,
        long egress_in_bytes)
    {

        // Logic to register the function with the serverless compute service
        return compute_service->registerFunction(function, time_limit_in_seconds, disk_space_limit_in_bytes, RAM_limit_in_bytes, ingress_in_bytes, egress_in_bytes);
        WRENCH_INFO("Function [%s] registered with compute service [%s]", function.getName().c_str(), compute_service->getName().c_str());
    }

//    /**
//     *
//     */
//    FunctionInvocation FunctionManager::invokeFunction(ServerlessComputeService, Function, FunctionInput) {
//        // Places a function invocation
//    }
//
//    /**
//     *
//     */
//    FunctionInvocation::is_running() {
//        // State finding method
//    }
//
//    /**
//     *
//     */
//    FunctionInvocation::is_done() {
//        // State finding method
//    }
//
//    /**
//     *
//     */
//    FunctionOutput FunctionInvocation::get_output() {
//        // State finding method
//    }
//
//    /**
//     *
//     */
//    FunctionInvocation::wait_one(one) {
//
//    }
//
//    /**
//     *
//     */
//    FunctionInvocation::wait_any(one) {
//
//    }
//
//    /**
//     *
//     */
//    FunctionInvocation::wait_all(list) {
//
//    }

}// namespace wrench
