/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_FUNCTIONALITY_NOT_AVAILABLE_H
#define WRENCH_FUNCTIONALITY_NOT_AVAILABLE_H

#include <set>
#include <string>

#include "FailureCause.h"
#include "wrench/services/storage/StorageService.h"
#include "wrench/services/compute/ComputeService.h"

namespace wrench {

    class Service;

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/


    /**
     * @brief A "requested functionality is not available on that service" failure cause
     */
    class FunctionalityNotAvailable : public FailureCause {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        FunctionalityNotAvailable(std::shared_ptr<Service>  service, std::string functionality_name);
        /***********************/
        /** \endcond           */
        /***********************/

        std::shared_ptr<Service>  getService();
        std::string getFunctionalityName();
        std::string toString() override;

    private:
        std::shared_ptr<Service>  service;
        std::string functionality_name;
    };


    /***********************/
    /** \endcond           */
    /***********************/
};


#endif //WRENCH_FUNCTIONALITY_NOT_AVAILABLE_H
