/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/compute/hadoop/hadoop_subsystem/MRJobExecutorMessagePayload.h"

namespace wrench {

    SET_MESSAGEPAYLOAD_NAME(MRJobExecutorMessagePayload, NOTIFY_EXECUTOR_STATUS_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(MRJobExecutorMessagePayload, STOP_DAEMON_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(MRJobExecutorMessagePayload, MAP_SIDE_SHUFFLE_REQUEST_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(MRJobExecutorMessagePayload, MAP_SIDE_HDFS_DATA_DELIVERY_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(MRJobExecutorMessagePayload, MAP_SIDE_HDFS_DATA_REQUEST_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(MRJobExecutorMessagePayload, MAP_OUTPUT_MATERIALIZED_BYTES_PAYLOAD);
}
