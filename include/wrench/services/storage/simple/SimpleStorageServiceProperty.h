/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_SIMPLESTORAGESERVICEPROPERTY_H
#define WRENCH_SIMPLESTORAGESERVICEPROPERTY_H

#include "wrench/services/storage/StorageServiceProperty.h"

namespace wrench {

    /**
    * @brief Configurable properties for a SimpleStorageService
    */
    class SimpleStorageServiceProperty : public StorageServiceProperty {

    public:
        /** @brief The maximum number of concurrent data connections supported by the service (default = "infinity") **/
        DECLARE_PROPERTY_NAME(MAX_NUM_CONCURRENT_DATA_CONNECTIONS);
    };

};// namespace wrench


#endif//WRENCH_SIMPLESTORAGESERVICEPROPERTY_H
