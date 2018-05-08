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
     * @brief Configurable properties for a CloudService
     */
    class CloudServiceProperty : public ComputeServiceProperty {

    public:
        /** @brief The number of bytes in the control message sent to the service to request a get execution hosts. **/
        DECLARE_PROPERTY_NAME(GET_EXECUTION_HOSTS_REQUEST_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent by the service in answer to a get execution hosts request. **/
        DECLARE_PROPERTY_NAME(GET_EXECUTION_HOSTS_ANSWER_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent to the service to request a VM creation. **/
        DECLARE_PROPERTY_NAME(CREATE_VM_REQUEST_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent by the service in answer to a VM creation request. **/
        DECLARE_PROPERTY_NAME(CREATE_VM_ANSWER_MESSAGE_PAYLOAD);
    };
}

#endif //WRENCH_CLOUDSERVICEPROPERTY_H
