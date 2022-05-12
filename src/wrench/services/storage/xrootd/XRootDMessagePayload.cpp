/**
* Copyright (c) 2017. The WRENCH Team.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#include "wrench/services/storage/xrootd/XRootDMessagePayload.h"
namespace wrench {
    namespace XRootD{


        SET_MESSAGEPAYLOAD_NAME(MessagePayload,FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD);

        SET_MESSAGEPAYLOAD_NAME(MessagePayload,FILE_DELETE_REQUEST_MESSAGE_PAYLOAD);

        SET_MESSAGEPAYLOAD_NAME(MessagePayload,FILE_READ_REQUEST_MESSAGE_PAYLOAD);
        SET_MESSAGEPAYLOAD_NAME(MessagePayload,FILE_SEARCH_ANSWER_MESSAGE_PAYLOAD);

        SET_MESSAGEPAYLOAD_NAME(MessagePayload,CONTINUE_SEARCH);

        SET_MESSAGEPAYLOAD_NAME(MessagePayload,UPDATE_CACHE);

        SET_MESSAGEPAYLOAD_NAME(MessagePayload,CACHE_ENTRY);



    };
};// namespace wrench
