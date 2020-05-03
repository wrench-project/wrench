/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_MRJOBEXECUTORMESSAGEPAYLOAD_H
#define WRENCH_MRJOBEXECUTORMESSAGEPAYLOAD_H

#include "wrench/services/compute/ComputeServiceMessagePayload.h"

namespace wrench {

    /**
     * @brief Configurable message payloads for a MRJobExecutor
     */
    class MRJobExecutorMessagePayload : public ComputeServiceMessagePayload {
    public:
        DECLARE_MESSAGEPAYLOAD_NAME(NOTIFY_EXECUTOR_STATUS_MESSAGE_PAYLOAD);
        DECLARE_MESSAGEPAYLOAD_NAME(STOP_DAEMON_MESSAGE_PAYLOAD);
        DECLARE_MESSAGEPAYLOAD_NAME(MAP_SIDE_HDFS_DATA_REQUEST_PAYLOAD);
        DECLARE_MESSAGEPAYLOAD_NAME(MAP_SIDE_HDFS_DATA_DELIVERY_PAYLOAD);
        DECLARE_MESSAGEPAYLOAD_NAME(MAP_SIDE_SHUFFLE_REQUEST_PAYLOAD);
    };
}

#endif //WRENCH_MRJOBEXECUTORMESSAGEPAYLOAD_H
