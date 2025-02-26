/**
 * Copyright (c) 2025.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_FUNCTIONMANAGER_H
#define WRENCH_FUNCTIONMANAGER_H

#include <vector>
#include <map>
#include <string>
#include <memory>
#include <wrench/services/compute/serverless/Invocation.h>

#include "wrench/services/Service.h"
#include "wrench/services/storage/storage_helpers/FileLocation.h"
#include "wrench/managers/function_manager/FunctionInput.h"

namespace wrench {

    class Function;

    class RegisteredFunction;

    class ServerlessComputeService;

    class StorageService;

    class FailureCause;

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
     * @brief A service to manage serverless function operations including creation and registration.
     */
    class FunctionManager : public Service {
    public:
        void stop() override;

        void kill();

        static std::shared_ptr<Function> createFunction(const std::string& name,
                                                        const std::function<std::string(const std::shared_ptr<FunctionInput>&, 
                                                        const std::shared_ptr<StorageService>&)>& lambda,
                                                        const std::shared_ptr<FileLocation>& image,
                                                        const std::shared_ptr<FileLocation>& code);

        bool registerFunction(std::shared_ptr<Function> function,
                              const std::shared_ptr<ServerlessComputeService>& compute_service,
                              int time_limit_in_seconds,
                              long disk_space_limit_in_bytes,
                              long RAM_limit_in_bytes,
                              long ingress_in_bytes,
                              long egress_in_bytes);

        std::shared_ptr<Invocation> invokeFunction(std::shared_ptr<Function> function,
                                                    const std::shared_ptr<ServerlessComputeService>& sl_compute_service,
                                                    std::shared_ptr<FunctionInput> function_invocation_args);

        bool isDone(std::shared_ptr<Invocation> invocation);
        void wait_one(std::shared_ptr<Invocation> invocation);
        /***********************/
        /** \cond INTERNAL    */
        /***********************/

        ~FunctionManager() override;

    protected:
        friend class ExecutionController;

        explicit FunctionManager(const std::string& hostname, S4U_CommPort *creator_commport);

        /***********************/
        /** \endcond           */
        /***********************/

    private:
        int main() override;

        bool processNextMessage();

        void processFunctionInvocationComplete();

        void processFunctionInvocationFailure();

        void processWaitOne(std::shared_ptr<Invocation> invocation, S4U_CommPort* answer_commport);

        void processWaitAll(std::vector<std::shared_ptr<Invocation>> invocations, S4U_CommPort* answer_commport);

        void processInvocationsBeingWaitedFor();

        S4U_CommPort *creator_commport;

        // FunctionManager internal data structures
        std::set<std::shared_ptr<RegisteredFunction>> _registered_functions; // do we store these here or in the Serverless Compute Service?
        std::queue<std::shared_ptr<RegisteredFunction>> _functions_to_invoke;
        std::set<std::shared_ptr<Invocation>> _pending_invocations; // do we really need this?
        std::set<std::shared_ptr<Invocation>> _finished_invocations;
        std::vector<std::pair<std::shared_ptr<Invocation>, S4U_CommPort*>> _invocations_being_waited_for; // should it be S4U_CommPort or a pointer to one?
    };

    /***********************/
    /** \endcond            */
    /***********************/

} // namespace wrench

#endif // WRENCH_FUNCTIONMANAGER_H
