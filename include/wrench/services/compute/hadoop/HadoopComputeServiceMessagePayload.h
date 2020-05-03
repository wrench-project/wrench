/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_HADOOPCOMPUTESERVICEMESSAGEPAYLOAD_H
#define WRENCH_HADOOPCOMPUTESERVICEMESSAGEPAYLOAD_H

#include "wrench/services/compute/ComputeServiceMessagePayload.h"

namespace wrench {

    /**
     * @brief Configurable message payloads for a HadoopComputeService
     */
    class HadoopComputeServiceMessagePayload : public ComputeServiceMessagePayload {
    public:
        DECLARE_MESSAGEPAYLOAD_NAME(RUN_MR_JOB_REQUEST_MESSAGE_PAYLOAD);
        DECLARE_MESSAGEPAYLOAD_NAME(RUN_MR_JOB_ANSWER_MESSAGE_PAYLOAD);
        DECLARE_MESSAGEPAYLOAD_NAME(DAEMON_STOPPED_MESSAGE_PAYLOAD);
        DECLARE_MESSAGEPAYLOAD_NAME(NOTIFY_EXECUTOR_STATUS_MESSAGE_PAYLOAD);
    };
}

#endif //WRENCH_HADOOPCOMPUTESERVICEMESSAGEPAYLOAD_H
