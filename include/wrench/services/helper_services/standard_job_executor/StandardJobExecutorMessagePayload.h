/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_MULTINODEMULTICORESTANDARDJOBEXECUTORMESSAGEPAYLOAD_H
#define WRENCH_MULTINODEMULTICORESTANDARDJOBEXECUTORMESSAGEPAYLOAD_H


#include "wrench/services/ServiceMessagePayload.h"

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Configurable message payloads for a StandardJobExecutor
     */
    class StandardJobExecutorMessagePayload {

    public:

        /** @brief The number of bytes in the control message sent by the executor to state that it has completed a job **/
        DECLARE_MESSAGEPAYLOAD_NAME(STANDARD_JOB_DONE_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent by the executor to state that a job has failed **/
        DECLARE_MESSAGEPAYLOAD_NAME(STANDARD_JOB_FAILED_MESSAGE_PAYLOAD);

    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_MULTINODEMULTICORESTANDARDJOBEXECUTORMESSAGEPAYLOAD_H
