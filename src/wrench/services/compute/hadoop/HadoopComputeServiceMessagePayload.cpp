/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/compute/hadoop/HadoopComputeServiceMessagePayload.h"

namespace wrench {

    SET_MESSAGEPAYLOAD_NAME(HadoopComputeServiceMessagePayload, RUN_MR_JOB_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(HadoopComputeServiceMessagePayload, RUN_MR_JOB_ANSWER_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(HadoopComputeServiceMessagePayload, DAEMON_STOPPED_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(HadoopComputeServiceMessagePayload, NOTIFY_EXECUTOR_STATUS_MESSAGE_PAYLOAD);

}
