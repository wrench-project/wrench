//
// Created by jamcdonald on 1/17/23.
//

#ifndef WRENCH_STORAGESERVICEPROXYPROPERTY_H
#define WRENCH_STORAGESERVICEPROXYPROPERTY_H
#include "wrench/services/storage/StorageServiceProperty.h"


namespace wrench {

    /**
     * @brief Configurable properties for a StorageService
     */
    class StorageServiceProxyProperty : public StorageServiceProperty {

    public:
        /** @brief The overhead for handling just 1 message
         **/
        DECLARE_PROPERTY_NAME(MESSAGE_OVERHEAD);
        DECLARE_PROPERTY_NAME(UNCACHED_READ_METHOD);

    };

}// namespace wrench
#endif//WRENCH_STORAGESERVICEPROXYPROPERTY_H
