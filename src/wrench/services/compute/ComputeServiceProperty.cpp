/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/compute/ComputeServiceProperty.h"

namespace wrench {

    SET_PROPERTY_NAME(ComputeServiceProperty, SCRATCH_STORAGE_SIZE);
    SET_PROPERTY_NAME(ComputeServiceProperty, SUPPORTS_STANDARD_JOBS);
    SET_PROPERTY_NAME(ComputeServiceProperty, SUPPORTS_PILOT_JOBS);
    SET_PROPERTY_NAME(ComputeServiceProperty, JOB_TYPE_NOT_SUPPORTED_MESSAGE_PAYLOAD);
    SET_PROPERTY_NAME(ComputeServiceProperty, SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD);
    SET_PROPERTY_NAME(ComputeServiceProperty, SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD);
    SET_PROPERTY_NAME(ComputeServiceProperty, STANDARD_JOB_DONE_MESSAGE_PAYLOAD);
    SET_PROPERTY_NAME(ComputeServiceProperty, STANDARD_JOB_FAILED_MESSAGE_PAYLOAD);
    SET_PROPERTY_NAME(ComputeServiceProperty, TERMINATE_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD);
    SET_PROPERTY_NAME(ComputeServiceProperty, TERMINATE_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD);
    SET_PROPERTY_NAME(ComputeServiceProperty, SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD);
    SET_PROPERTY_NAME(ComputeServiceProperty, SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD);
    SET_PROPERTY_NAME(ComputeServiceProperty, PILOT_JOB_STARTED_MESSAGE_PAYLOAD);
    SET_PROPERTY_NAME(ComputeServiceProperty, PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD);
    SET_PROPERTY_NAME(ComputeServiceProperty, PILOT_JOB_FAILED_MESSAGE_PAYLOAD);
    SET_PROPERTY_NAME(ComputeServiceProperty, TTL_REQUEST_MESSAGE_PAYLOAD);
    SET_PROPERTY_NAME(ComputeServiceProperty, TTL_ANSWER_MESSAGE_PAYLOAD);
    SET_PROPERTY_NAME(ComputeServiceProperty, TERMINATE_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD);
    SET_PROPERTY_NAME(ComputeServiceProperty, TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD);
    SET_PROPERTY_NAME(ComputeServiceProperty, RESOURCE_DESCRIPTION_REQUEST_MESSAGE_PAYLOAD);
    SET_PROPERTY_NAME(ComputeServiceProperty, RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD);

};
