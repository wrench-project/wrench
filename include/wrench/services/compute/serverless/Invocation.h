#ifndef INVOCATION_H
#define INVOCATION_H

#include <memory>

#include <wrench/managers/function_manager/Function.h>
#include <wrench/managers/function_manager/RegisteredFunction.h>
#include <wrench/simgrid_S4U_util/S4U_CommPort.h>

namespace wrench
{
    class Invocation {
    public:
        Invocation(std::shared_ptr<wrench::RegisteredFunction> registered_function,
                   std::shared_ptr<wrench::FunctionInput> function_input,
                   wrench::S4U_CommPort* notify_commport) : _registered_function(registered_function),
                                                                            _function_input(function_input),
                                                                            _notify_commport(notify_commport)
        {
        }

        std::shared_ptr<wrench::RegisteredFunction> _registered_function;
        std::shared_ptr<wrench::FunctionInput> _function_input;
        // std::shared_ptr<wrench::FunctionOutput> _function_output;
        wrench::S4U_CommPort* _notify_commport;
    };
}

#endif //INVOCATION_H
