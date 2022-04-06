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

    class DataFile;

    class StorageService;

    /**
     * @brief A file registry service (a.k.a. replica catalog) that holds a database
     *        of which files are available at which storage services. Specifically,
     *        the database holds a set of <file, storage service> entries. A WMS can add,
     *        lookup, and remove entries at will from this database.
     */
    class FileRegistryService : public Service {

    private:
        WRENCH_PROPERTY_COLLECTION_TYPE default_property_values = {
                {FileRegistryServiceProperty::LOOKUP_COMPUTE_COST, "0.0"},
                {FileRegistryServiceProperty::ADD_ENTRY_COMPUTE_COST, "0.0"},
                {FileRegistryServiceProperty::REMOVE_ENTRY_COMPUTE_COST, "0.0"},
        };

        WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE default_messagepayload_values = {
                {FileRegistryServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, 1024},
                {FileRegistryServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD, 1024},
                {FileRegistryServiceMessagePayload::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD, 1024},
                {FileRegistryServiceMessagePayload::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD, 1024},
                {FileRegistryServiceMessagePayload::REMOVE_ENTRY_REQUEST_MESSAGE_PAYLOAD, 1024},
                {FileRegistryServiceMessagePayload::REMOVE_ENTRY_ANSWER_MESSAGE_PAYLOAD, 1024},
                {FileRegistryServiceMessagePayload::ADD_ENTRY_REQUEST_MESSAGE_PAYLOAD, 1024},
                {FileRegistryServiceMessagePayload::ADD_ENTRY_ANSWER_MESSAGE_PAYLOAD, 1024},
        };

    public:
        // Public Constructor
        FileRegistryService(std::string hostname,
                            WRENCH_PROPERTY_COLLECTION_TYPE property_list = {},
                            WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list = {});

        /****************************/
        /** \cond DEVELOPER         */
        /****************************/

        std::set<std::shared_ptr<FileLocation>> lookupEntry(std::shared_ptr<DataFile> file);

        std::map<double, std::shared_ptr<FileLocation>> lookupEntry(
                std::shared_ptr<DataFile> file, std::string reference_host,
                std::shared_ptr<NetworkProximityService> network_proximity_service);

        void addEntry(const std::shared_ptr<DataFile>& file, std::shared_ptr<FileLocation> location);

        void removeEntry(const std::shared_ptr<DataFile>& file, const std::shared_ptr<FileLocation>& location);

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

        void addEntryToDatabase(const std::shared_ptr<DataFile>& file, const std::shared_ptr<FileLocation>& location);

        bool removeEntryFromDatabase(const std::shared_ptr<DataFile>& file, const std::shared_ptr<FileLocation>& location);

        int main() override;

        bool processNextMessage();

        std::map<std::shared_ptr<DataFile>, std::set<std::shared_ptr<FileLocation>>>
                entries;
    };

};// namespace wrench

#endif//WRENCH_FILEREGISTRYSERVICE_H
