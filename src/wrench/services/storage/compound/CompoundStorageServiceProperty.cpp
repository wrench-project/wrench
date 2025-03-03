/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/storage/compound/CompoundStorageServiceProperty.h>

namespace wrench {

    SET_PROPERTY_NAME(CompoundStorageServiceProperty, MAX_ALLOCATION_CHUNK_SIZE);

    SET_PROPERTY_NAME(CompoundStorageServiceProperty, INTERNAL_STRIPING);

} // namespace wrench