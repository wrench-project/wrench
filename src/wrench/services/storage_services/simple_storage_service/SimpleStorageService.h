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


#include <services/storage_services/StorageService.h>
#include <simgrid_S4U_util/S4U_DaemonWithMailbox.h>

namespace wrench {

    class SimpleStorageService : public StorageService {

    public:

        enum Property {
            /** The number of bytes in the control message
             * sent to the daemon to terminate it (default: 1024) **/
                    STOP_DAEMON_MESSAGE_PAYLOAD,
            /** The number of bytes in the control message
            * sent by the daemon to confirm it has terminate (default: 1024) **/
                    DAEMON_STOPPED_MESSAGE_PAYLOAD
        };

    private:

        std::map<SimpleStorageService::Property, std::string> default_property_values =
                {{SimpleStorageService::Property::STOP_DAEMON_MESSAGE_PAYLOAD,    "1024"},
                 {SimpleStorageService::Property::DAEMON_STOPPED_MESSAGE_PAYLOAD, "1024"}
                };


    public:

        // Public Constructor
        SimpleStorageService(std::string hostname,
                             double capacity,
                             std::map<SimpleStorageService::Property, std::string> = {});

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        // Stopping the service
        void stop();

        void copyFile(WorkflowFile *file, StorageService *src);

        void downloadFile(WorkflowFile *file);

        void uploadFile(WorkflowFile *file);

        void deleteFile(WorkflowFile *file);


        /***********************/
        /** \endcond           */
        /***********************/

    private:

        friend class Simulation;

        // Low-level Constructor
        SimpleStorageService(std::string hostname,
                             double capacity,
                             std::map<SimpleStorageService::Property, std::string>,
                             std::string suffix);

        std::map<SimpleStorageService::Property, std::string> property_list;

        std::string hostname;
        double capacity;


        int main();

        bool processNextMessage();

    };


};


#endif //WRENCH_SIMPLESTORAGESERVICE_H
