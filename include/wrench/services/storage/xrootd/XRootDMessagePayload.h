//
// Created by jamcdonald on 3/28/22.
//

#ifndef WRENCH_XROOTDMESSAGEPAYLOAD_H
#define WRENCH_XROOTDMESSAGEPAYLOAD_H
/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include "wrench/services/ServiceMessagePayload.h"


namespace wrench {
    namespace XRootD{
        /**
         * @brief Configurable message payloads for a StorageService
         */
        class MessagePayload : public ServiceMessagePayload {

        public:
            /** @brief The number of bytes in the control message sent to the daemon to request its free space information **/

            /** @brief The number of bytes in the control message sent to the daemon to request a file lookup **/
            DECLARE_MESSAGEPAYLOAD_NAME(FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD);

            /** @brief The number of bytes in the control message sent to the daemon to request a file deletion **/
            DECLARE_MESSAGEPAYLOAD_NAME(FILE_DELETE_REQUEST_MESSAGE_PAYLOAD);

            /** @brief The number of bytes in the control message sent to the daemon to request a file read **/
            DECLARE_MESSAGEPAYLOAD_NAME(FILE_READ_REQUEST_MESSAGE_PAYLOAD);
            /** @brief The number of bytes in the control message sent by the daemon to answer a file read request **/
            DECLARE_MESSAGEPAYLOAD_NAME(FILE_SEARCH_ANSWER_MESSAGE_PAYLOAD);

            /** @brief The number of bytes in the control message sent by the daemon to continue a search request **/
            DECLARE_MESSAGEPAYLOAD_NAME(CONTINUE_SEARCH);

            /** @brief The number of bytes in the control message sent by the daemon to update the cache **/
            DECLARE_MESSAGEPAYLOAD_NAME(UPDATE_CACHE);

            /** @brief The number of bytes in each cache entry **/
            DECLARE_MESSAGEPAYLOAD_NAME(CACHE_ENTRY);


        };
    };
};// namespace wrench



#endif //WRENCH_XROOTDMESSAGEPAYLOAD_H
