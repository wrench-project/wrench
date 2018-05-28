/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/compute/multihost_multicore/MultihostMulticoreComputeServiceMessagePayload.h"

namespace wrench {

    SET_MESSAGEPAYLOAD_NAME(MultihostMulticoreComputeServiceMessagePayload, NOT_ENOUGH_CORES_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(MultihostMulticoreComputeServiceMessagePayload, FLOP_RATE_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(MultihostMulticoreComputeServiceMessagePayload, FLOP_RATE_ANSWER_MESSAGE_PAYLOAD);

};

