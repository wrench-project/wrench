/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include "wrench/services/compute/virtualized_cluster/VirtualizedClusterServiceMessagePayload.h"


namespace wrench {

    SET_MESSAGEPAYLOAD_NAME(VirtualizedClusterServiceMessagePayload, GET_EXECUTION_HOSTS_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(VirtualizedClusterServiceMessagePayload, GET_EXECUTION_HOSTS_ANSWER_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(VirtualizedClusterServiceMessagePayload, CREATE_VM_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(VirtualizedClusterServiceMessagePayload, CREATE_VM_ANSWER_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(VirtualizedClusterServiceMessagePayload, MIGRATE_VM_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(VirtualizedClusterServiceMessagePayload, MIGRATE_VM_ANSWER_MESSAGE_PAYLOAD);
}
