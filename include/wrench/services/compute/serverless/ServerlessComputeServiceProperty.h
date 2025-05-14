/**
* Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_SERVERLESSCOMPUTESERVICEPROPERTY_H
#define WRENCH_SERVERLESSCOMPUTESERVICEPROPERTY_H

#include <map>

#include "wrench/services/compute/ComputeServiceProperty.h"

namespace wrench {

    /**
    * @brief Configurable properties for a bare_metal_standard_jobs
    */
    class ServerlessComputeServiceProperty : public ComputeServiceProperty {

    public:
        /** @brief The overhead to start a container after the image has been loaded into RAM (default value: "0", default unit: seconds):
         *         Examples: "5", "5s", "5000ms", etc.
         **/
        DECLARE_PROPERTY_NAME(CONTAINER_STARTUP_OVERHEAD);
    };

}// namespace wrench

#endif//WRENCH_SERVERLESSCOMPUTESERVICEPROPERTY_H
