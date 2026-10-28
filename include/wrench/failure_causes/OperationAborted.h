/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_OPERATION_ABORTED_H
#define WRENCH_OPERATION_ABORTED_H

#include <set>
#include <string>

#include "wrench/services/Service.h"
#include "FailureCause.h"

namespace wrench {

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
    * @brief An "operation aborted" failure cause
    */
    class OperationAborted : public FailureCause {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        explicit OperationAborted(std::string error_message);

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

#endif//WRENCH_OPERATION_ABORTED_H
