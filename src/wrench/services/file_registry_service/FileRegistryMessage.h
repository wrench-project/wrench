/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_FILEREGISTRYMESSAGE_H
#define WRENCH_FILEREGISTRYMESSAGE_H


#include <services/ServiceMessage.h>

namespace wrench {

//    FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD, "1024"},
//{FileRegistryServiceProperty::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD,  "1024"},
//{FileRegistryServiceProperty::REMOVE_ENTRY_REQUEST_PAYLOAD,        "1024"},
//{FileRegistryServiceProperty::REMOVE_ENTRY_ANSWER_PAYLOAD,         "1024"},
//{FileRegistryServiceProperty::ADD_ENTRY_REQUEST_PAYLOAD,           "1024"},
//{FileRegistryServiceProperty::ADD_ENTRY_ANSWER_PAYLOAD,

class FileRegistryMessage : public ServiceMessage {
    protected:
        FileRegistryMessage(std::string name, double payload);

    };

};


#endif //WRENCH_FILEREGISTRYMESSAGE_H
