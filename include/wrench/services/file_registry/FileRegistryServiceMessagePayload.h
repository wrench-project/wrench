/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_FILEREGISTRYMESSAGEPAYLOAD_H
#define WRENCH_FILEREGISTRYMESSAGEPAYLOAD_H

#include "wrench/services/ServiceMessagePayload.h"

namespace wrench {

    /**
     * @brief Configurable message payload for a FileRegistryService
     */
    class FileRegistryServiceMessagePayload : public ServiceMessagePayload {

    public:
        /** @brief The number of bytes in a request control message sent to the daemon to request a list of file locations **/
        DECLARE_MESSAGEPAYLOAD_NAME(FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD);
        /** @brief The number of bytes per file location returned in an answer sent by the daemon to answer a file location request **/
        DECLARE_MESSAGEPAYLOAD_NAME(FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD);

        /** @brief The number of bytes in the control message sent to the daemon to cause it to add an entry **/
        DECLARE_MESSAGEPAYLOAD_NAME(ADD_ENTRY_REQUEST_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent by the daemon to answer an entry addition request **/
        DECLARE_MESSAGEPAYLOAD_NAME(ADD_ENTRY_ANSWER_MESSAGE_PAYLOAD);

        /** @brief The number of bytes in the control message sent to the daemon to cause it to remove an entry **/
        DECLARE_MESSAGEPAYLOAD_NAME(REMOVE_ENTRY_REQUEST_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent by the daemon to answer an entry removal request **/
        DECLARE_MESSAGEPAYLOAD_NAME(REMOVE_ENTRY_ANSWER_MESSAGE_PAYLOAD);
    };

};// namespace wrench


#endif//WRENCH_FILEREGISTRYMESSAGEPAYLOAD_H
