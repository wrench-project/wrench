/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_INVALID_OPERATION_H
#define WRENCH_INVALID_OPERATION_H

#include <set>
#include <string>

#include "wrench/services/Service.h"
#include "FailureCause.h"

namespace wrench {

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
    * @brief A "invalid operation" failure cause
    */
    class InvalidOperation : public FailureCause {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        explicit InvalidOperation(std::string error_message);

        /***********************/
        /** \endcond           */
        /***********************/

        std::string toString() override;

    private:
        std::string error_message;
    };

    /***********************/
    /** \endcond           */
    /***********************/
}// namespace wrench

#endif//WRENCH_INVALID_OPERATION_H
