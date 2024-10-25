/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_STORAGESERVICEPROPERTY_H
#define WRENCH_STORAGESERVICEPROPERTY_H

#include "wrench/services/ServiceProperty.h"


namespace wrench {

    /**
     * @brief Configurable properties for a StorageService
     */
    class StorageServiceProperty : public ServiceProperty {

    public:
        /** @brief Buffer size used when copying/communicating data:
         *  - "0": an ideal fluid model (very fast simulation)
         *  - "infinity": read/write and forward model (very fast, but likely less realistic simulation)
         *  - any integral value in between: an actual buffer size (the smaller the buffer size, the slower the simulation)
         *
         *  - Default value: "0" (fluid)
         *  - Example values: "0", "infinity", "42", "10000000", "42B", "56MB", "100KiB", etc.
         **/
        DECLARE_PROPERTY_NAME(BUFFER_SIZE);

        /** @brief The caching behavior. Possible values are:
         *   - "NONE" (default): no caching, i.e., if not enough space is available for a new file, then the file write/creation fails.
         *   - "FIFO": FIFO policy, i.e.,  if not enough space is available for a new file, the oldest
         *             files are deleted until enough space is available.
         *   - "LRU": Least Recently Used policy, i.e.,  if not enough space is available for a new file, the Least Recently Used
         *          files are deleted until enough space is available.
         **/
        DECLARE_PROPERTY_NAME(CACHING_BEHAVIOR);
    };

}// namespace wrench


#endif//WRENCH_STORAGESERVICEPROPERTY_H
