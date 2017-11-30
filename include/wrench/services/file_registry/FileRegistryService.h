/**
 * Copyright (c) 2017. The WRENCH Team.
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
#include "FileRegistryServiceProperty.h"

namespace wrench {

    class WorkflowFile;

    class StorageService;
    
    /**
     * @brief A file registry service (a.k.a. replica catalog)
     */
    class FileRegistryService : public Service {

    public:


    private:

        std::map<std::string, std::string> default_property_values =
                {{FileRegistryServiceProperty::STOP_DAEMON_MESSAGE_PAYLOAD,          "1024"},
                 {FileRegistryServiceProperty::DAEMON_STOPPED_MESSAGE_PAYLOAD,       "1024"},
                 {FileRegistryServiceProperty::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD,  "1024"},
                 {FileRegistryServiceProperty::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD,   "1024"},
                 {FileRegistryServiceProperty::REMOVE_ENTRY_REQUEST_MESSAGE_PAYLOAD, "1024"},
                 {FileRegistryServiceProperty::REMOVE_ENTRY_ANSWER_MESSAGE_PAYLOAD,  "1024"},
                 {FileRegistryServiceProperty::ADD_ENTRY_REQUEST_MESSAGE_PAYLOAD,    "1024"},
                 {FileRegistryServiceProperty::ADD_ENTRY_ANSWER_MESSAGE_PAYLOAD,     "1024"},
                 {FileRegistryServiceProperty::LOOKUP_OVERHEAD,                      "0.0"},
                };

    public:


        // Public Constructor
        FileRegistryService(std::string hostname,
                            std::map<std::string, std::string> = {});

        /****************************/
        /** \cond DEVELOPER         */
        /****************************/

        std::set<StorageService *> lookupEntry(WorkflowFile *file);

        void addEntry(WorkflowFile *file, StorageService *storage_service);

        void removeEntry(WorkflowFile *file, StorageService *storage_service);

        /****************************/
        /** \endcond                */
        /****************************/


        /****************************/
        /** \cond INTERNAL          */
        /****************************/
        ~FileRegistryService();
        /****************************/
        /** \endcond                */
        /****************************/

    private:

        friend class Simulation;


        FileRegistryService(std::string hostname,
                            std::map<std::string, std::string> plist,
                            std::string suffix = "");


        void addEntryToDatabase(WorkflowFile *file, StorageService *storage_service);

        bool removeEntryFromDatabase(WorkflowFile *file, StorageService *storage_service);

        int main();

        bool processNextMessage();

        std::map<WorkflowFile *, std::set<StorageService *>> entries;
    };



};


#endif //WRENCH_FILEREGISTRYSERVICE_H
