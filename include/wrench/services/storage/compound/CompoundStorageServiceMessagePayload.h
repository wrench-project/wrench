/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_COMPOUNDSTORAGESERVICEMESSAGEPAYLOAD_H
#define WRENCH_COMPOUNDSTORAGESERVICEMESSAGEPAYLOAD_H

#include "wrench/services/ServiceMessagePayload.h"
#include "wrench/services/storage/StorageServiceMessagePayload.h"

namespace wrench {

    /**
     * @brief Configurable message payloads for a CompoundStorageService
     */
    class CompoundStorageServiceMessagePayload : public StorageServiceMessagePayload {

    public:
        /** @brief The number of bytes in the control message sent by the daemon to answer a storage selection request **/
        DECLARE_MESSAGEPAYLOAD_NAME(STORAGE_SELECTION_PAYLOAD);
    };

};// namespace wrench

#endif// WRENCH_COMPOUNDSTORAGESERVICEMESSAGEPAYLOAD_H
