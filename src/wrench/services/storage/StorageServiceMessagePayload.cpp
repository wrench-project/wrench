/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <wrench/services/storage/StorageServiceMessagePayload.h>


namespace wrench {

    SET_MESSAGEPAYLOAD_NAME(StorageServiceMessagePayload, FREE_SPACE_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(StorageServiceMessagePayload, FREE_SPACE_ANSWER_MESSAGE_PAYLOAD);

    SET_MESSAGEPAYLOAD_NAME(StorageServiceMessagePayload, FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(StorageServiceMessagePayload, FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD);

    SET_MESSAGEPAYLOAD_NAME(StorageServiceMessagePayload, FILE_DELETE_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(StorageServiceMessagePayload, FILE_DELETE_ANSWER_MESSAGE_PAYLOAD);

    SET_MESSAGEPAYLOAD_NAME(StorageServiceMessagePayload, FILE_COPY_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(StorageServiceMessagePayload, FILE_COPY_ANSWER_MESSAGE_PAYLOAD);

    SET_MESSAGEPAYLOAD_NAME(StorageServiceMessagePayload, FILE_WRITE_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(StorageServiceMessagePayload, FILE_WRITE_ANSWER_MESSAGE_PAYLOAD);

    SET_MESSAGEPAYLOAD_NAME(StorageServiceMessagePayload, FILE_READ_REQUEST_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(StorageServiceMessagePayload, FILE_READ_ANSWER_MESSAGE_PAYLOAD);

    SET_MESSAGEPAYLOAD_NAME(StorageServiceMessagePayload, FILE_NOT_FOUND_MESSAGE_PAYLOAD);

    SET_MESSAGEPAYLOAD_NAME(StorageServiceMessagePayload, NOT_ENOUGH_STORAGE_SPACE_MESSAGE_PAYLOAD);

};// namespace wrench
