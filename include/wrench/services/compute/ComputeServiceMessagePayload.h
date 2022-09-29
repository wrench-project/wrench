/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_COMPUTESERVICEMESSAGEPAYLOAD_H
#define WRENCH_COMPUTESERVICEMESSAGEPAYLOAD_H

#include "wrench/services/ServiceMessagePayload.h"

namespace wrench {

    /**
     * @brief Configurable message payloads for a ComputeService
     */
    class ComputeServiceMessagePayload : public ServiceMessagePayload {
    public:
        /** @brief The number of bytes in the control message sent by the daemon to state that it does not support the type of a submitted job **/
        DECLARE_MESSAGEPAYLOAD_NAME(JOB_TYPE_NOT_SUPPORTED_MESSAGE_PAYLOAD);

        /** @brief The number of bytes in the control message sent to the daemon to submit a standard job **/
        DECLARE_MESSAGEPAYLOAD_NAME(SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent by the daemon to acknowledge a standard job submission **/
        DECLARE_MESSAGEPAYLOAD_NAME(SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent by the daemon to state that it has completed a standard job **/
        DECLARE_MESSAGEPAYLOAD_NAME(STANDARD_JOB_DONE_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent by the daemon to state that a running standard job has failed **/
        DECLARE_MESSAGEPAYLOAD_NAME(STANDARD_JOB_FAILED_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent to the daemon to terminate a standard job **/
        DECLARE_MESSAGEPAYLOAD_NAME(TERMINATE_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent by the daemon to acknowledge a standard job termination **/
        DECLARE_MESSAGEPAYLOAD_NAME(TERMINATE_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent to the daemon to submit a pilot job **/

        /** @brief The number of bytes in the control message sent to the daemon to submit a compound job **/
        DECLARE_MESSAGEPAYLOAD_NAME(SUBMIT_COMPOUND_JOB_REQUEST_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent by the daemon to acknowledge a compound job submission **/
        DECLARE_MESSAGEPAYLOAD_NAME(SUBMIT_COMPOUND_JOB_ANSWER_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent by the daemon to state that it has completed a compound job **/
        DECLARE_MESSAGEPAYLOAD_NAME(COMPOUND_JOB_DONE_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent by the daemon to state that a running compound job has failed **/
        DECLARE_MESSAGEPAYLOAD_NAME(COMPOUND_JOB_FAILED_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent to the daemon to terminate a compound job **/
        DECLARE_MESSAGEPAYLOAD_NAME(TERMINATE_COMPOUND_JOB_REQUEST_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent by the daemon to acknowledge a compound job termination **/
        DECLARE_MESSAGEPAYLOAD_NAME(TERMINATE_COMPOUND_JOB_ANSWER_MESSAGE_PAYLOAD);

        /** @brief The number of bytes in the control message sent to the daemon to submit a pilot job **/
        DECLARE_MESSAGEPAYLOAD_NAME(SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent from the daemon to acknowledge a pilot job submission **/
        DECLARE_MESSAGEPAYLOAD_NAME(SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent by the daemon to state that a pilot job has started **/
        DECLARE_MESSAGEPAYLOAD_NAME(PILOT_JOB_STARTED_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent by the daemon to state that a pilot job has expired **/
        DECLARE_MESSAGEPAYLOAD_NAME(PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent by the daemon to state that a pilot job has failed **/
        DECLARE_MESSAGEPAYLOAD_NAME(PILOT_JOB_FAILED_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent to the daemon to terminate a pilot job **/
        DECLARE_MESSAGEPAYLOAD_NAME(TERMINATE_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent by the daemon to acknowledge a pilot job termination **/
        DECLARE_MESSAGEPAYLOAD_NAME(TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent to the daemon to ask it for information on its resources **/

        /** @brief The number of bytes in the control message sent to the daemon to request information on its resources **/
        DECLARE_MESSAGEPAYLOAD_NAME(RESOURCE_DESCRIPTION_REQUEST_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent by the daemon to state information on its resources **/
        DECLARE_MESSAGEPAYLOAD_NAME(RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent to the daemon to ask is one host has some resources available **/
        DECLARE_MESSAGEPAYLOAD_NAME(IS_THERE_AT_LEAST_ONE_HOST_WITH_AVAILABLE_RESOURCES_ANSWER_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message by the the daemon to state whether one host has some resources available **/
        DECLARE_MESSAGEPAYLOAD_NAME(IS_THERE_AT_LEAST_ONE_HOST_WITH_AVAILABLE_RESOURCES_REQUEST_MESSAGE_PAYLOAD);
    };
};// namespace wrench

#endif//WRENCH_COMPUTESERVICEMESSAGEPAYLOAD_H
