/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SERVICE_IS_SUSPENDED_H
#define WRENCH_SERVICE_IS_SUSPENDED_H

#include <set>
#include <string>

#include "wrench/services/Service.h"
#include "wrench/workflow/failure_causes/FailureCause.h"

namespace wrench {

    class Service;

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
     * @brief A "service is suspended" failure cause
     */
    class ServiceIsSuspended : public FailureCause {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        explicit ServiceIsSuspended(std::shared_ptr<Service> service);
        /***********************/
        /** \endcond           */
        /***********************/

        std::shared_ptr<Service> getService();
        std::string toString() override;

    private:
        std::shared_ptr<Service> service;
    };


    /***********************/
    /** \endcond           */
    /***********************/
};


#endif //WRENCH_SERVICE_IS_SUSPENDED_H
