/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "FileRegistryServiceProperty.h"

namespace wrench {

    SET_PROPERTY_NAME(FileRegistryServiceProperty, FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD);
    SET_PROPERTY_NAME(FileRegistryServiceProperty, FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD);

    SET_PROPERTY_NAME(FileRegistryServiceProperty, REMOVE_ENTRY_REQUEST_PAYLOAD);
    SET_PROPERTY_NAME(FileRegistryServiceProperty, REMOVE_ENTRY_ANSWER_PAYLOAD);

    SET_PROPERTY_NAME(FileRegistryServiceProperty, ADD_ENTRY_REQUEST_PAYLOAD);
    SET_PROPERTY_NAME(FileRegistryServiceProperty, ADD_ENTRY_ANSWER_PAYLOAD);

    SET_PROPERTY_NAME(FileRegistryServiceProperty, LOOKUP_OVERHEAD);
};

