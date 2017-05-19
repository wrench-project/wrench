/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */



#include "StorageServiceProperty.h"


namespace wrench {

    SET_PROPERTY_NAME(StorageServiceProperty, FREE_SPACE_REQUEST_MESSAGE_PAYLOAD);
    SET_PROPERTY_NAME(StorageServiceProperty, FREE_SPACE_ANSWER_MESSAGE_PAYLOAD);
    
    SET_PROPERTY_NAME(StorageServiceProperty, FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD);
    SET_PROPERTY_NAME(StorageServiceProperty, FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD);

    SET_PROPERTY_NAME(StorageServiceProperty, FILE_DELETE_REQUEST_MESSAGE_PAYLOAD);
    SET_PROPERTY_NAME(StorageServiceProperty, FILE_DELETE_ANSWER_MESSAGE_PAYLOAD);

    SET_PROPERTY_NAME(StorageServiceProperty, FILE_COPY_REQUEST_MESSAGE_PAYLOAD);
    SET_PROPERTY_NAME(StorageServiceProperty, FILE_COPY_ANSWER_MESSAGE_PAYLOAD);

    SET_PROPERTY_NAME(StorageServiceProperty, FILE_UPLOAD_REQUEST_MESSAGE_PAYLOAD);
    SET_PROPERTY_NAME(StorageServiceProperty, FILE_UPLOAD_ANSWER_MESSAGE_PAYLOAD);

    SET_PROPERTY_NAME(StorageServiceProperty, FILE_DOWNLOAD_REQUEST_MESSAGE_PAYLOAD);
    SET_PROPERTY_NAME(StorageServiceProperty, FILE_DOWNLOAD_ANSWER_MESSAGE_PAYLOAD);

    SET_PROPERTY_NAME(StorageServiceProperty, FILE_NOT_FOUND_MESSAGE_PAYLOAD);

    SET_PROPERTY_NAME(StorageServiceProperty, NOT_ENOUGH_STORAGE_SPACE_MESSAGE_PAYLOAD);

};

