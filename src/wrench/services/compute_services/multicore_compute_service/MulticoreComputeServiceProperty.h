/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_MULTICORECOMPUTESERVICEPROPERTY_H
#define WRENCH_MULTICORECOMPUTESERVICEPROPERTY_H

#include <map>

#include <services/compute_services/ComputeServiceProperty.h>

namespace wrench {

   /**
    * @brief Properties for a MulticoreComputeService
    */
    class MulticoreComputeServiceProperty : public ComputeServiceProperty {

    public:

        /** @brief The number of bytes in the control message sent by the daemon to state that it does not have sufficient cores to run a submitted job **/
        DECLARE_PROPERTY_NAME(NOT_ENOUGH_CORES_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent to the daemon to ask it for its number of cores **/
        DECLARE_PROPERTY_NAME(NUM_CORES_REQUEST_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent by the daemon to state how many cores it has **/
        DECLARE_PROPERTY_NAME(NUM_CORES_ANSWER_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent to the daemon to ask it for its number of idle cores **/
        DECLARE_PROPERTY_NAME(NUM_IDLE_CORES_REQUEST_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent by the daemon to state how many idle cores it has **/
        DECLARE_PROPERTY_NAME(NUM_IDLE_CORES_ANSWER_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent to the daemon to ask it for its per-core flop rate **/
        DECLARE_PROPERTY_NAME(FLOP_RATE_REQUEST_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent by the daemon to state its per-core flop rate **/
        DECLARE_PROPERTY_NAME(FLOP_RATE_ANSWER_MESSAGE_PAYLOAD);
        /** @brief The overhead to start a task execution, in seconds **/
        DECLARE_PROPERTY_NAME(TASK_STARTUP_OVERHEAD);

    };

};


#endif //WRENCH_MULTICORECOMPUTESERVICEPROPERTY_H
