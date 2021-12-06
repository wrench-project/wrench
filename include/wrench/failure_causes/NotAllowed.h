/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_NOT_ALLOWED_H
#define WRENCH_NOT_ALLOWED_H

#include <set>
#include <string>

#include "wrench/services/Service.h"
#include "FailureCause.h"

namespace wrench {

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
    * @brief A "operation not allowed" failure cause
    */
    class NotAllowed : public FailureCause {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        NotAllowed(std::shared_ptr<Service> service, std::string &error_message);
        /***********************/
        /** \endcond           */
        /***********************/

        std::shared_ptr<Service> getService();
        std::string toString() override;

    private:
        std::shared_ptr<Service> service;
        std::string error_message;
    };

    /***********************/
    /** \endcond           */
    /***********************/
};


#endif //WRENCH_NOT_ALLOWED_H
