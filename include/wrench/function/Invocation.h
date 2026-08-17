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
#include <string>

namespace wrench {
    class S4U_CommPort;
    class FailureCause;
    class RegisteredFunction;
    class FunctionInput;
    class FunctionOutput;
    class ServerlessComputeNode;
    class Container;


    /**
     * @class Invocation
     * @brief Represents an invocation of a registered function.
     */
    class Invocation {

    public:
        [[nodiscard]] bool isDispatched() const;
        [[nodiscard]] bool isDone() const;
        [[nodiscard]] bool hasSucceeded() const;
        [[nodiscard]] std::shared_ptr<RegisteredFunction> getRegisteredFunction() const;
        [[nodiscard]] std::shared_ptr<FailureCause> getFailureCause() const;
        [[nodiscard]] std::shared_ptr<FunctionOutput> getOutput() const;
        [[nodiscard]] double getSubmitDate() const;
        [[nodiscard]] double getDispatchDate() const;
        [[nodiscard]] double getFunctionStartDate() const;
        [[nodiscard]] double getFunctionEndDate() const;
        [[nodiscard]] unsigned long long getId() const;
        [[nodiscard]] std::string getComputeNode() const;

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

        Invocation(const std::shared_ptr<RegisteredFunction> &registered_function,
                   const std::shared_ptr<FunctionInput> &function_input,
                   S4U_CommPort* notify_commport);
    private:
        friend class FunctionManager;
        friend class ServerlessComputeService;

        const std::shared_ptr<RegisteredFunction> _registered_function; // the registered function to be invoked
        std::shared_ptr<FunctionInput> _function_input; // the input for the function
        bool _done; // whether the invocation is done
        bool _dispatched; // whether the invocation has been dispatched
        bool _success; // whether the invocation was successful
        std::shared_ptr<FailureCause> _failure_cause; // the cause of failure
        std::shared_ptr<FunctionOutput> _function_output; // the output of the function invocation
        S4U_CommPort* _notify_commport; // the communication port for notifications

        double _submit_date = -1.0;
        double _dispatch_date = -1.0;
        double _function_start_date = -1.0;
        double _function_end_date = -1.0;

        unsigned long long _id = 0;

        std::shared_ptr<ServerlessComputeNode> _compute_node;
        std::shared_ptr<Container> _container;

        /***********************/
        /** \endcond          **/
        /***********************/

    };
}

#endif //INVOCATION_H
