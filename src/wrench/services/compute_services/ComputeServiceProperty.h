/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_COMPUTESERVICEPROPERTY_H
#define WRENCH_COMPUTESERVICEPROPERTY_H

#include <services/ServiceProperty.h>

namespace wrench {

    class ComputeServiceProperty : public ServiceProperty {
    public:
        /** The number of bytes in the control message sent by the daemon to state that it does not support the type of a submitted job **/
        DECLARE_PROPERTY_NAME(JOB_TYPE_NOT_SUPPORTED_MESSAGE_PAYLOAD);
        /** The number of bytes in the control message sent to the daemon to submit a standard job to it **/
        DECLARE_PROPERTY_NAME(SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD);
        /** The number of bytes in the control message sent by the daemon to acknowledge a standard job submission **/
        DECLARE_PROPERTY_NAME(SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD);
        /** The number of bytes in the control message sent by the daemon to state that it has completed a standard job **/
        DECLARE_PROPERTY_NAME(STANDARD_JOB_DONE_MESSAGE_PAYLOAD);
        /** The number of bytes in the control message sent by the daemon to state that a running standard job has failed **/
        DECLARE_PROPERTY_NAME(STANDARD_JOB_FAILED_MESSAGE_PAYLOAD);
        /** The number of bytes in the control message sent to the daemon to submit a pilot job to it **/
        DECLARE_PROPERTY_NAME(SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD);
        /** The number of bytes in the control message sent from the daemon to acknowledge a pilot job submission **/
        DECLARE_PROPERTY_NAME(SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD);
        /** The number of bytes in the control message sent by the daemon to state that a pilot job has started **/
        DECLARE_PROPERTY_NAME(PILOT_JOB_STARTED_MESSAGE_PAYLOAD);
        /** The number of bytes in the control message sent by the daemon to state that a pilot job has expired **/
        DECLARE_PROPERTY_NAME(PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD);
        /** The number of bytes in the control message sent by the daemon to state that a pilot job has failed **/
        DECLARE_PROPERTY_NAME(PILOT_JOB_FAILED_MESSAGE_PAYLOAD);
        /** The number of bytes in the control message sent to the daemon to ask it for its time-to-live **/
        DECLARE_PROPERTY_NAME(TTL_REQUEST_MESSAGE_PAYLOAD);
        /** The number of bytes in the control message sent by the daemon to state its time-to-live **/
        DECLARE_PROPERTY_NAME(TTL_ANSWER_MESSAGE_PAYLOAD);
        /** The number of bytes in the control message sent to the daemon to ask it for a file lookup **/
        DECLARE_PROPERTY_NAME(FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD);
        /** The number of bytes in the control message sent by the daemon to answer a file lookup request **/
        DECLARE_PROPERTY_NAME(FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD);

    };


};


#endif //WRENCH_COMPUTESERVICEPROPERTY_H
