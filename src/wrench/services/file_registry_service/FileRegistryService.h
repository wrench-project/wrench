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


#include <simgrid_S4U_util/S4U_DaemonWithMailbox.h>
#include <services/storage_services/StorageService.h>
#include "FileRegistryServiceProperty.h"

namespace wrench {

    class FileRegistryService : public Service {

    public:
        

    private:

        std::map<std::string, std::string> default_property_values =
                {{FileRegistryServiceProperty::STOP_DAEMON_MESSAGE_PAYLOAD,    "1024"},
                 {FileRegistryServiceProperty::DAEMON_STOPPED_MESSAGE_PAYLOAD, "1024"},
                 {FileRegistryServiceProperty::REQUEST_MESSAGE_PAYLOAD,        "1024"},
                 {FileRegistryServiceProperty::ANSWER_MESSAGE_PAYLOAD,         "1024"},
                 {FileRegistryServiceProperty::REMOVE_ENTRY_PAYLOAD,           "1024"},
                 {FileRegistryServiceProperty::LOOKUP_OVERHEAD,                "0.0"},
                };

    public:

        // Public Constructor
        FileRegistryService(std::string hostname,
                            std::map<std::string, std::string> = {});

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        enum State {
            UP,
            DOWN,
        };

        /***********************/
        /** \endcond           */
        /***********************/


    private:

        friend class Simulation;

        void addEntry(WorkflowFile *file, StorageService *ss);
        void removeEntry(WorkflowFile *file, StorageService *ss);
        void removeAllEntries(WorkflowFile *file);

        FileRegistryService(std::string hostname,
                            std::map<std::string, std::string> plist,
                            std::string suffix = "");


        int main();
        bool processNextMessage();


        std::string hostname;
        FileRegistryService::State state;


        std::map<WorkflowFile *, std::set<StorageService *>> entries;
    };


};


#endif //WRENCH_FILEREGISTRYSERVICE_H
