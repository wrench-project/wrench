/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_CLOUDSERVICEPROPERTY_H
#define WRENCH_CLOUDSERVICEPROPERTY_H

#include "wrench/services/compute/ComputeServiceProperty.h"

namespace wrench {

    /**
     * @brief Properties for a CloudService
     */
    class CloudServiceProperty : public ComputeServiceProperty {

    public:
        DECLARE_PROPERTY_NAME(CREATE_VM_REQUEST_MESSAGE_PAYLOAD);
        DECLARE_PROPERTY_NAME(CREATE_VM_ANSWER_MESSAGE_PAYLOAD);
    };
}

#endif //WRENCH_CLOUDSERVICEPROPERTY_H
