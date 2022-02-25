/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/ServiceMessagePayload.h>


namespace wrench {
    WRENCH_MESSAGEPAYLOAD_TYPE WRENCH_MESSAGEPAYLOAD_COUNT=0;
    SET_MESSAGEPAYLOAD_NAME(ServiceMessagePayload, STOP_DAEMON_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ServiceMessagePayload, DAEMON_STOPPED_MESSAGE_PAYLOAD);

};


