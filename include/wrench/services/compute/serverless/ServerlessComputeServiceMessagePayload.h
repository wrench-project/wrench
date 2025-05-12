/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_SERVERLESSCOMPUTESERVICEMESSAGEPAYLOAD_H
#define WRENCH_SERVERLESSCOMPUTESERVICEMESSAGEPAYLOAD_H

#include "wrench/services/compute/ComputeServiceMessagePayload.h"

namespace wrench {

    /**
     * @brief Configurable message payloads for a ServerlessComputeService
     */
    class ServerlessComputeServiceMessagePayload : public ComputeServiceMessagePayload {
    public:

        /** @brief The number of bytes in a control message sent to the Serverless Compute Service to register a function */
        DECLARE_MESSAGEPAYLOAD_NAME(FUNCTION_REGISTER_REQUEST_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in a control message sent by the Serverless Compute Service to answer a function registration request */
        DECLARE_MESSAGEPAYLOAD_NAME(FUNCTION_REGISTER_ANSWER_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in a control message sent by the Serverless Compute Service to invoke a function */
        DECLARE_MESSAGEPAYLOAD_NAME(FUNCTION_INVOKE_REQUEST_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in a control message sent by the Serverless Compute Service to answer a function invocation request */
        DECLARE_MESSAGEPAYLOAD_NAME(FUNCTION_INVOKE_ANSWER_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in a control message sent by the Serverless Compute Service to notify of a function completion */
        DECLARE_MESSAGEPAYLOAD_NAME(FUNCTION_COMPLETION_MESSAGE_PAYLOAD);
    };
}// namespace wrench

#endif//WRENCH_SERVERLESSCOMPUTESERVICEMESSAGEPAYLOAD_H
