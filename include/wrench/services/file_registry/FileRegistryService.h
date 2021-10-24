/**
 * Copyright (c) 2017-2020. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_FILEREGISTRYSERVICE_H
#define WRENCH_FILEREGISTRYSERVICE_H

#include <set>

#include "wrench/services/Service.h"
#include "wrench/services/network_proximity/NetworkProximityService.h"
#include "wrench/services/storage/StorageService.h"
#include "wrench/services/storage/storage_helpers/FileLocation.h"

#include "FileRegistryServiceProperty.h"
#include "FileRegistryServiceMessagePayload.h"

namespace wrench {

    class WorkflowFile;

    class StorageService;

    /**
     * @brief A file registry service (a.k.a. replica catalog) that holds a database
     *        of which files are available at which storage services. Specifically,
     *        the database holds a set of <file, storage service> entries. A WMS can add,
     *        lookup, and remove entries at will from this database.
     */
    class FileRegistryService : public Service {

    private:
        std::map <std::string, std::string> default_property_values = {
                {FileRegistryServiceProperty::LOOKUP_COMPUTE_COST,       "0.0"},
                {FileRegistryServiceProperty::ADD_ENTRY_COMPUTE_COST,    "0.0"},
                {FileRegistryServiceProperty::REMOVE_ENTRY_COMPUTE_COST, "0.0"},
        };

        std::map<std::string, double> default_messagepayload_values = {
                {FileRegistryServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD,          1024},
                {FileRegistryServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD,       1024},
                {FileRegistryServiceMessagePayload::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD,  1024},
                {FileRegistryServiceMessagePayload::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD,   1024},
                {FileRegistryServiceMessagePayload::REMOVE_ENTRY_REQUEST_MESSAGE_PAYLOAD, 1024},
                {FileRegistryServiceMessagePayload::REMOVE_ENTRY_ANSWER_MESSAGE_PAYLOAD,  1024},
                {FileRegistryServiceMessagePayload::ADD_ENTRY_REQUEST_MESSAGE_PAYLOAD,    1024},
                {FileRegistryServiceMessagePayload::ADD_ENTRY_ANSWER_MESSAGE_PAYLOAD,     1024},
        };

    public:
        // Public Constructor
        explicit FileRegistryService(std::string hostname,
                                     std::map <std::string, std::string> property_list = {},
                                     std::map<std::string, double> messagepayload_list = {}
        );

        /****************************/
        /** \cond DEVELOPER         */
        /****************************/

        std::set<std::shared_ptr<FileLocation>> lookupEntry(WorkflowFile *file);

        std::map<double, std::shared_ptr<FileLocation>> lookupEntry(
                WorkflowFile *file, std::string reference_host,
                std::shared_ptr <NetworkProximityService> network_proximity_service);

        void addEntry(WorkflowFile *file, std::shared_ptr <FileLocation> location);

        void removeEntry(WorkflowFile *file, std::shared_ptr <FileLocation> location);

        /****************************/
        /** \endcond                */
        /****************************/


        /****************************/
        /** \cond INTERNAL          */
        /****************************/

        ~FileRegistryService() override;
        /****************************/
        /** \endcond                */
        /****************************/

    private:
        friend class Simulation;

        void addEntryToDatabase(WorkflowFile *file, std::shared_ptr <FileLocation> location);

        bool removeEntryFromDatabase(WorkflowFile *file, std::shared_ptr <FileLocation> location);

        int main() override;

        bool processNextMessage();

        std::map<WorkflowFile *, std::set < std::shared_ptr < FileLocation>>>
        entries;
    };

};

#endif //WRENCH_FILEREGISTRYSERVICE_H
