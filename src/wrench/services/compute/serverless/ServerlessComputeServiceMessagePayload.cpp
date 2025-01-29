/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/compute/serverless/ServerlessComputeServiceMessagePayload.h>

namespace wrench {


    SET_MESSAGEPAYLOAD_NAME(ServerlessComputeServiceMessagePayload, FUNCTION_REGISTER_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ServerlessComputeServiceMessagePayload, JOB_TYPE_NOT_SUPPORTED_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ServerlessComputeServiceMessagePayload, SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ServerlessComputeServiceMessagePayload, SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ServerlessComputeServiceMessagePayload, STANDARD_JOB_DONE_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ServerlessComputeServiceMessagePayload, STANDARD_JOB_FAILED_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ServerlessComputeServiceMessagePayload, TERMINATE_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ServerlessComputeServiceMessagePayload, TERMINATE_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ServerlessComputeServiceMessagePayload, SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ServerlessComputeServiceMessagePayload, SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ServerlessComputeServiceMessagePayload, PILOT_JOB_STARTED_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ServerlessComputeServiceMessagePayload, PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ServerlessComputeServiceMessagePayload, PILOT_JOB_FAILED_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ServerlessComputeServiceMessagePayload, TERMINATE_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ServerlessComputeServiceMessagePayload, TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ServerlessComputeServiceMessagePayload, RESOURCE_DESCRIPTION_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ServerlessComputeServiceMessagePayload, RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ServerlessComputeServiceMessagePayload, IS_THERE_AT_LEAST_ONE_HOST_WITH_AVAILABLE_RESOURCES_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ServerlessComputeServiceMessagePayload, IS_THERE_AT_LEAST_ONE_HOST_WITH_AVAILABLE_RESOURCES_ANSWER_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ServerlessComputeServiceMessagePayload, COMPOUND_JOB_DONE_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ServerlessComputeServiceMessagePayload, COMPOUND_JOB_FAILED_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ServerlessComputeServiceMessagePayload, SUBMIT_COMPOUND_JOB_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ServerlessComputeServiceMessagePayload, SUBMIT_COMPOUND_JOB_ANSWER_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ServerlessComputeServiceMessagePayload, TERMINATE_COMPOUND_JOB_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ServerlessComputeServiceMessagePayload, TERMINATE_COMPOUND_JOB_ANSWER_MESSAGE_PAYLOAD);

}// namespace wrench
