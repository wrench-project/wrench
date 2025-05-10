/**
 * Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef INVOCATION_H
#define INVOCATION_H

#include <memory>
#include <wrench/managers/function_manager/Function.h>
#include <wrench/managers/function_manager/RegisteredFunction.h>
#include <wrench/managers/function_manager/FunctionOutput.h>
#include <wrench/failure_causes/FailureCause.h>

namespace wrench {
   
    /**
     * @class Invocation
     * @brief Represents an invocation of a registered function.
     */
    class Invocation {

    public:
        /**
         * @brief Constructor for Invocation.
         * @param registered_function The registered function to be invoked.
         * @param function_input The input for the function.
         * @param notify_commport The communication port for notifications.
         */
        Invocation(const std::shared_ptr<RegisteredFunction> &registered_function,
                   const std::shared_ptr<FunctionInput> &function_input,
                   S4U_CommPort* notify_commport);

        [[nodiscard]] bool isDone() const;
        [[nodiscard]] bool hasSucceeded() const;
        [[nodiscard]] std::shared_ptr<RegisteredFunction> getRegisteredFunction() const;
        [[nodiscard]] std::shared_ptr<FailureCause> getFailureCause() const;
        [[nodiscard]] std::shared_ptr<FunctionOutput> getOutput() const;
        [[nodiscard]] double getSubmitDate() const;
        [[nodiscard]] double getStartDate() const;
        [[nodiscard]] double getFinishDate() const;

    private:
        friend class FunctionManager;
        friend class ServerlessComputeService;

        const std::shared_ptr<RegisteredFunction> _registered_function; // the registered function to be invoked
        std::shared_ptr<FunctionInput> _function_input; // the input for the function
        bool _done; // whether the invocation is done
        bool _success; // whether the invocation was successful
        std::shared_ptr<FailureCause> _failure_cause; // the cause of failure
        std::shared_ptr<FunctionOutput> _function_output; // the output of the function invocation
        S4U_CommPort* _notify_commport; // the communication port for notifications

        // TODO: All the members below are really implementation details of the service
        // TODO: and could be instead storage in some server-side "ConcreteInvocation" object
        // TODO: that merely holds a pointer to a client-side "Invocation" object
        std::shared_ptr<FileLocation> _tmp_file;
        std::shared_ptr<StorageService> _tmp_storage_service;

        double _submit_date = -1.0;
        double _start_date = -1.0;
        double _finish_date = -1.0;

    };
}

#endif //INVOCATION_H
