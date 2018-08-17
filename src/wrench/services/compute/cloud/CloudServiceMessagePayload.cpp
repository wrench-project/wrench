/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/compute/cloud/CloudServiceMessagePayload.h"


namespace wrench {

    SET_MESSAGEPAYLOAD_NAME(CloudServiceMessagePayload, GET_EXECUTION_HOSTS_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(CloudServiceMessagePayload, GET_EXECUTION_HOSTS_ANSWER_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(CloudServiceMessagePayload, CREATE_VM_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(CloudServiceMessagePayload, CREATE_VM_ANSWER_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(CloudServiceMessagePayload, SHUTDOWN_VM_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(CloudServiceMessagePayload, SHUTDOWN_VM_ANSWER_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(CloudServiceMessagePayload, START_VM_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(CloudServiceMessagePayload, START_VM_ANSWER_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(CloudServiceMessagePayload, SUSPEND_VM_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(CloudServiceMessagePayload, SUSPEND_VM_ANSWER_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(CloudServiceMessagePayload, RESUME_VM_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(CloudServiceMessagePayload, RESUME_VM_ANSWER_MESSAGE_PAYLOAD);

}
