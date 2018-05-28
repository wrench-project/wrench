/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_FILEREGISTRYPROPERTY_H
#define WRENCH_FILEREGISTRYPROPERTY_H

#include "wrench/services/ServiceProperty.h"

namespace wrench {

    /**
     * @brief Configurable properties for a FileRegistryService
     */
    class FileRegistryServiceProperty: public ServiceProperty {

    public:

        /** @brief The computational cost, in flops, of looking up entries for a file **/
        DECLARE_PROPERTY_NAME(LOOKUP_OVERHEAD);
        
    };

};


#endif //WRENCH_FILEREGISTRYPROPERTY_H
