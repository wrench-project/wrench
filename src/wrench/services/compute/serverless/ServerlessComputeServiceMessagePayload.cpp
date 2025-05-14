/**
 * Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/compute/serverless/ServerlessComputeServiceMessagePayload.h>

namespace wrench {


    SET_MESSAGEPAYLOAD_NAME(ServerlessComputeServiceMessagePayload, FUNCTION_REGISTER_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ServerlessComputeServiceMessagePayload, FUNCTION_REGISTER_ANSWER_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ServerlessComputeServiceMessagePayload, FUNCTION_INVOKE_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ServerlessComputeServiceMessagePayload, FUNCTION_INVOKE_ANSWER_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ServerlessComputeServiceMessagePayload, FUNCTION_COMPLETION_MESSAGE_PAYLOAD);

}// namespace wrench
