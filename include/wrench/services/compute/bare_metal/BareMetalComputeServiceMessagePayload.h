/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_BAREMETALCOMPUTESERVICEMESSAGEPAYLOAD_H
#define WRENCH_BAREMETALCOMPUTESERVICEMESSAGEPAYLOAD_H

#include <map>

#include "wrench/services/compute/ComputeServiceMessagePayload.h"

namespace wrench {

    /**
    * @brief Configurable message payloads for a MultiHostMulticoreComputeService
    */
    class BareMetalComputeServiceMessagePayload : public ComputeServiceMessagePayload {

    public:
        /** @brief The number of bytes in the control message sent by the daemon to state that it does not have sufficient cores to (ever) run a submitted job **/
        DECLARE_MESSAGEPAYLOAD_NAME(NOT_ENOUGH_CORES_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent to the daemon to ask it for its per-core flop rate **/
        DECLARE_MESSAGEPAYLOAD_NAME(FLOP_RATE_REQUEST_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent by the daemon to state its per-core flop rate **/
        DECLARE_MESSAGEPAYLOAD_NAME(FLOP_RATE_ANSWER_MESSAGE_PAYLOAD);
    };

};// namespace wrench


#endif//WRENCH_BAREMETALCOMPUTESERVICEMESSAGEPAYLOAD_H
