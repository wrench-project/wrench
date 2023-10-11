/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef WRENCH_VIRTUALIZEDCLUSTERMESSAGEPAYLOAD_H
#define WRENCH_VIRTUALIZEDCLUSTERMESSAGEPAYLOAD_H

#include "wrench/services/compute/cloud/CloudComputeServiceMessagePayload.h"

namespace wrench {

    /**
     * @brief Configurable message payloads for a VirtualizedClusterComputeService
     */
    class VirtualizedClusterComputeServiceMessagePayload : public CloudComputeServiceMessagePayload {

    public:
        /** @brief The number of bytes in the control message sent to the service to request a VM migration. **/
        DECLARE_MESSAGEPAYLOAD_NAME(MIGRATE_VM_REQUEST_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent by the service in answer to a VM migration request. **/
        DECLARE_MESSAGEPAYLOAD_NAME(MIGRATE_VM_ANSWER_MESSAGE_PAYLOAD);
    };

}// namespace wrench

#endif//WRENCH_VIRTUALIZEDCLUSTERMESSAGEPAYLOAD_H
