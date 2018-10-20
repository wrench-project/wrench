/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_MULTIHOSTMULTICORECOMPUTESERVICEPROPERTY_H
#define WRENCH_MULTIHOSTMULTICORECOMPUTESERVICEPROPERTY_H

#include <map>

#include "wrench/services/compute/ComputeServiceProperty.h"

namespace wrench {

   /**
    * @brief Configurable properties for a MultiHostMulticoreComputeService
    */
    class MultihostMulticoreComputeServiceProperty : public ComputeServiceProperty {

    public:

        /** @brief The overhead to start a thread, in seconds **/
        DECLARE_PROPERTY_NAME(THREAD_STARTUP_OVERHEAD);

    };

};


#endif //WRENCH_MULTIHOSTMULTICORECOMPUTESERVICEPROPERTY_H
