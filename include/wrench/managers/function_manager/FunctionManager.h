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

#include "wrench/services/Service.h"
#include "wrench/services/storage/storage_helpers/FileLocation.h"

namespace wrench {

    class Function;
    class ServerlessComputeService;
    class StorageService;

    /**
     * @brief A service to manage serverless function operations including creation and registration.
     */
    class FunctionManager : public Service {
    public:
        void stop() override;

        void kill();

        /**
         * @brief Creates a new function.
         * @param name The name of the function.
         * @param lambda The lambda function to execute.
         * @param image The file location of the associated container image.
         * @param code The file location of the source code.
         * @return A shared pointer to the created Function object.
         */
        static std::shared_ptr<Function> createFunction(
            const std::string& name,
            const std::function<std::string(const std::string&, const std::shared_ptr<StorageService>&)>& lambda,
            const std::shared_ptr<FileLocation>& image,
            const std::shared_ptr<FileLocation>& code);

        /**
         * @brief Registers a function with a serverless compute provider.
         * @param function The function to register.
         * @param compute_service The serverless compute service provider.
         * @param time_limit_in_seconds The time limit for function execution.
         * @param disk_space_limit_in_bytes The disk space limit for the function.
         * @param RAM_limit_in_bytes The RAM limit for the function.
         * @param ingress_in_bytes The maximum inbound data limit.
         * @param egress_in_bytes The maximum outbound data limit.
         */
        void registerFunction(
            const Function function,
            const std::shared_ptr<ServerlessComputeService>& compute_service,
            int time_limit_in_seconds,
            long disk_space_limit_in_bytes,
            long RAM_limit_in_bytes,
            long ingress_in_bytes,
            long egress_in_bytes);

        ~FunctionManager() override;

    protected:
        friend class ExecutionController;

        explicit FunctionManager(const std::string& hostname, S4U_CommPort* creator_commport);

    private:
        int main() override;

        S4U_CommPort* creator_commport;
    };

} // namespace wrench

#endif // WRENCH_FUNCTIONMANAGER_H
