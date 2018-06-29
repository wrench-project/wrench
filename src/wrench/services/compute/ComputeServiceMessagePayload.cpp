/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/compute/ComputeServiceMessagePayload.h"

namespace wrench {

    SET_MESSAGEPAYLOAD_NAME(ComputeServiceMessagePayload, JOB_TYPE_NOT_SUPPORTED_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ComputeServiceMessagePayload, SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ComputeServiceMessagePayload, SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ComputeServiceMessagePayload, STANDARD_JOB_DONE_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ComputeServiceMessagePayload, STANDARD_JOB_FAILED_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ComputeServiceMessagePayload, TERMINATE_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ComputeServiceMessagePayload, TERMINATE_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ComputeServiceMessagePayload, SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ComputeServiceMessagePayload, SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ComputeServiceMessagePayload, PILOT_JOB_STARTED_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ComputeServiceMessagePayload, PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ComputeServiceMessagePayload, PILOT_JOB_FAILED_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ComputeServiceMessagePayload, TTL_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ComputeServiceMessagePayload, TTL_ANSWER_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ComputeServiceMessagePayload, TERMINATE_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ComputeServiceMessagePayload, TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ComputeServiceMessagePayload, RESOURCE_DESCRIPTION_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ComputeServiceMessagePayload, RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD);

};
