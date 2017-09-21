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
     * @brief Properties for a FileRegistryService
     */
    class FileRegistryServiceProperty: public ServiceProperty {

    public:
        /** @brief The number of bytes in a request control message sent to the daemon to request a list of file locations **/
        DECLARE_PROPERTY_NAME(FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD);
        /** @brief The number of bytes per file location returned in an answer sent by the daemon to answer a file location request **/
        DECLARE_PROPERTY_NAME(FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD);

        /** @brief The number of bytes in the control message sent to the daemon to cause it to add an entry **/
        DECLARE_PROPERTY_NAME(ADD_ENTRY_REQUEST_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent by the daemon to answer an entry addition request **/
        DECLARE_PROPERTY_NAME(ADD_ENTRY_ANSWER_MESSAGE_PAYLOAD);

        /** @brief The number of bytes in the control message sent to the daemon to cause it to remove an entry **/
        DECLARE_PROPERTY_NAME(REMOVE_ENTRY_REQUEST_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent by the daemon to answer an entry removal request **/
        DECLARE_PROPERTY_NAME(REMOVE_ENTRY_ANSWER_MESSAGE_PAYLOAD);

        /** @brief The overhead, in seconds, of looking up entries for a file **/
        DECLARE_PROPERTY_NAME(LOOKUP_OVERHEAD);
        
    };

};


#endif //WRENCH_FILEREGISTRYPROPERTY_H
