/**
 * Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_FUNCTIONNOTFOUND_H
#define WRENCH_FUNCTIONNOTFOUND_H

#include <set>
#include <string>

#include "FailureCause.h"

namespace wrench {
    class RegisteredFunction;

    class Function;

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
     * @brief A "function was not found" failure cause
     */
    class FunctionNotFound : public FailureCause {
    public:

        /***********************/
        /** \cond INTERNAL     */
        /***********************/

        FunctionNotFound(std::shared_ptr<RegisteredFunction> registered_function);

        /***********************/
        /** \endcond           */
        /***********************/

        std::string toString() override;

    private:
        std::shared_ptr<RegisteredFunction> _registered_function;
    };

    /***********************/
    /** \endcond           */
    /***********************/

} // namespace wrench


#endif //WRENCH_FUNCTIONNOTFOUND_H
