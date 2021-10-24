/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include "wrench/services/helper_services/standard_job_executor/StandardJobExecutorMessagePayload.h"

namespace wrench {

    SET_MESSAGEPAYLOAD_NAME(StandardJobExecutorMessagePayload, STANDARD_JOB_DONE_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(StandardJobExecutorMessagePayload, STANDARD_JOB_FAILED_MESSAGE_PAYLOAD);
};
