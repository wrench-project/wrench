/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_BAREMETALCOMPUTESERVICEPROPERTY_H
#define WRENCH_BAREMETALCOMPUTESERVICEPROPERTY_H

#include <map>

#include "wrench/services/compute/ComputeServiceProperty.h"

namespace wrench {

   /**
    * @brief Configurable properties for a bare_metal_standard_jobs
    */
    class BareMetalComputeServiceProperty : public ComputeServiceProperty {

    public:

        /** @brief The overhead to start a thread, in seconds **/
        DECLARE_PROPERTY_NAME(TASK_STARTUP_OVERHEAD);
        /** @brief If true, fail action after an executor crash, otherwise re-ready it and try again **/
        DECLARE_PROPERTY_NAME(FAIL_ACTION_AFTER_ACTION_EXECUTOR_CRASH);
        /** @brief If true, service will terminate whenever all resources are down **/
       DECLARE_PROPERTY_NAME(TERMINATE_WHENEVER_ALL_RESOURCES_ARE_DOWN);

    };

};

#endif //WRENCH_BAREMETALCOMPUTESERVICEPROPERTY_H
