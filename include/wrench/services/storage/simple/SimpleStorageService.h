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


#include <services/storage/simple/NetworkConnection.h>
#include <services/storage/simple/NetworkConnectionManager.h>
#include "wrench/services/storage/StorageService.h"
#include "SimpleStorageServiceProperty.h"
#include "wrench/simgrid_S4U_util/S4U_PendingCommunication.h"
#include "IncomingFile.h"

namespace wrench {

    class SimulationMessage;

    class S4U_PendingCommunication;

    class IncomingFile;

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
                             unsigned long num_concurrent_connections,
                             std::map<std::string, std::string> = {});

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

        ~SimpleStorageService();

        /***********************/
        /** \endcond          **/
        /***********************/

    private:



        friend class Simulation;

        // Low-level Constructor
        SimpleStorageService(std::string hostname,
                             double capacity,
                             std::map<std::string, std::string>,
                             std::string suffix);

        int main() override;

        bool processControlMessage(std::unique_ptr<NetworkConnection> connection);

        bool processDataConnection(std::unique_ptr<NetworkConnection> connection);
        bool processIncomingDataConnection(std::unique_ptr<NetworkConnection> connection);
        bool processOutgoingDataConnection(std::unique_ptr<NetworkConnection> connection);

        unsigned long getNewUniqueNumber();

        bool processFileWriteRequest(WorkflowFile *file, std::string answer_mailbox);

        bool processFileReadRequest(WorkflowFile *file, std::string answer_mailbox,
                                    std::string mailbox_to_receive_the_file_content);

        bool processFileCopyRequest(WorkflowFile *file, StorageService *src, std::string answer_mailbox);

        unsigned long num_concurrent_connections;

        std::vector<std::unique_ptr<S4U_PendingCommunication>> pending_incoming_communications;
        std::map<S4U_PendingCommunication *, std::unique_ptr<IncomingFile>> incoming_files;

        std::vector<std::unique_ptr<S4U_PendingCommunication>> pending_outgoing_communications;

        std::unique_ptr<NetworkConnectionManager> network_connection_manager;


    };

};


#endif //WRENCH_SIMPLESTORAGESERVICE_H
