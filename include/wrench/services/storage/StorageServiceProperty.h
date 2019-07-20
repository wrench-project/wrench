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

        /** @brief The maximum number of concurrent data connections supported by the service (default = "infinity") **/
        DECLARE_PROPERTY_NAME(MAX_NUM_CONCURRENT_DATA_CONNECTIONS);

        /** @brief The simulated local copy data rate in byte/sec (default = "infinity") **/
        DECLARE_PROPERTY_NAME(LOCAL_COPY_DATA_RATE);

    };

};


#endif //WRENCH_STORAGESERVICEPROPERTY_H
