/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SIMPLESTORAGESERVICE_H
#define WRENCH_SIMPLESTORAGESERVICE_H


#include "services/storage_services/StorageService.h"
#include "services/storage_services/simple_storage_service/SimpleStorageServiceProperty.h"

namespace wrench {

    /**
     * @brief A simple StorageService implementation
     */
    class SimpleStorageService : public StorageService {

    private:
        std::map<std::string, std::string> default_property_values =
                {{SimpleStorageServiceProperty::STOP_DAEMON_MESSAGE_PAYLOAD,         "1024"},
                 {SimpleStorageServiceProperty::DAEMON_STOPPED_MESSAGE_PAYLOAD,      "1024"},
                 {SimpleStorageServiceProperty::FREE_SPACE_REQUEST_MESSAGE_PAYLOAD,  "1024"},
                 {SimpleStorageServiceProperty::FREE_SPACE_ANSWER_MESSAGE_PAYLOAD,   "1024"},
                 {SimpleStorageServiceProperty::FILE_DELETE_REQUEST_MESSAGE_PAYLOAD, "1024"},
                 {SimpleStorageServiceProperty::FILE_DELETE_ANSWER_MESSAGE_PAYLOAD,  "1024"},
                 {SimpleStorageServiceProperty::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD, "1024"},
                 {SimpleStorageServiceProperty::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD,  "1024"},
                 {SimpleStorageServiceProperty::FILE_COPY_REQUEST_MESSAGE_PAYLOAD,   "1024"},
                 {SimpleStorageServiceProperty::FILE_COPY_ANSWER_MESSAGE_PAYLOAD,    "1024"},
                 {SimpleStorageServiceProperty::FILE_WRITE_REQUEST_MESSAGE_PAYLOAD,  "1024"},
                 {SimpleStorageServiceProperty::FILE_WRITE_ANSWER_MESSAGE_PAYLOAD,   "1024"},
                 {SimpleStorageServiceProperty::FILE_READ_REQUEST_MESSAGE_PAYLOAD,   "1024"},
                 {SimpleStorageServiceProperty::FILE_READ_ANSWER_MESSAGE_PAYLOAD,    "1024"},
                };

    public:

        // Public Constructor
        SimpleStorageService(std::string hostname,
                             double capacity,
                             std::map<std::string, std::string> = {});


        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        /***********************/
        /** \endcond           */
        /***********************/

    private:

        friend class Simulation;

        // Low-level Constructor
        SimpleStorageService(std::string hostname,
                             double capacity,
                             std::map<std::string, std::string>,
                             std::string suffix);

        double capacity;

        int main();

        bool processNextMessage();

        std::string data_write_mailbox_name;

        unsigned long getNewUniqueNumber();

        };

};


#endif //WRENCH_SIMPLESTORAGESERVICE_H
