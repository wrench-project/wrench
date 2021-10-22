/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SERVICE_IS_DOWN_H
#define WRENCH_SERVICE_IS_DOWN_H

#include <set>
#include <string>

#include "FailureCause.h"
#include "wrench/services/Service.h"

namespace wrench {

    class Service;

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
     * @brief A "service is down" failure cause
     */
    class ServiceIsDown : public FailureCause {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        explicit ServiceIsDown(std::shared_ptr<Service> service);
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


#endif //WRENCH_SERVICE_IS_DOWN_H
