/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/file_registry/FileRegistryServiceMessagePayload.h>

namespace wrench {

    SET_MESSAGEPAYLOAD_NAME(FileRegistryServiceMessagePayload, FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(FileRegistryServiceMessagePayload, FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD);

    SET_MESSAGEPAYLOAD_NAME(FileRegistryServiceMessagePayload, REMOVE_ENTRY_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(FileRegistryServiceMessagePayload, REMOVE_ENTRY_ANSWER_MESSAGE_PAYLOAD);

    SET_MESSAGEPAYLOAD_NAME(FileRegistryServiceMessagePayload, ADD_ENTRY_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(FileRegistryServiceMessagePayload, ADD_ENTRY_ANSWER_MESSAGE_PAYLOAD);

};// namespace wrench
