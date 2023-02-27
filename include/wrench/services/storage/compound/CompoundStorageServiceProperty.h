/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_COMPOUNDSTORAGESERVICEPROPERTY_H
#define WRENCH_COMPOUNDSTORAGESERVICEPROPERTY_H

#include "wrench/services/storage/StorageServiceProperty.h"

namespace wrench {

    /**
    * @brief Configurable properties for a CompoundStorageService
    */
    class CompoundStorageServiceProperty : public StorageServiceProperty {
    public:
        /** @brief Property that defines how the underlying storage is selected:
         *         So far the only option is to have an external process that 
         *         update actions in job (property value: "external"), with the
         *         CompoundStorageService being passive. A future option would be
         *         to have the CSS take the decision upon receiving an IO request.
         */
        DECLARE_PROPERTY_NAME(STORAGE_SELECTION_METHOD);
    };

};// namespace wrench


#endif//WRENCH_COMPOUNDSTORAGESERVICEPROPERTY_H