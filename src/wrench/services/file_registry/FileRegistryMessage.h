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


#include <wrench/services/ServiceMessage.h>
#include <iostream>
#include <wrench/services/network_proximity/NetworkProximityService.h>

namespace wrench {

    /***********************/
    /** \cond INTERNAL    **/
    /***********************/

    /**
     * @brief Top-level FileRegistryMessage class
     */
    class FileRegistryMessage : public ServiceMessage {
    protected:
        FileRegistryMessage(std::string name, double payload);

    };

    /**
     * @brief FileRegistryFileLookupRequestMessage class
     */
    class FileRegistryFileLookupRequestMessage : public FileRegistryMessage {
    public:
        FileRegistryFileLookupRequestMessage(std::string answer_mailbox, WorkflowFile *file, double payload);

        /** @brief The mailbox to which the answer message should be sent */
        std::string answer_mailbox;
        /** @brief The file to lookup */
        WorkflowFile *file;
    };

    /**
     * @brief FileRegistryFileLookupAnswerMessage class
     */
    class FileRegistryFileLookupAnswerMessage : public FileRegistryMessage {
    public:
        FileRegistryFileLookupAnswerMessage(WorkflowFile *file, std::set<StorageService *> locations, double payload);

        /** @brief The file that was looked up */
        WorkflowFile *file;
        /** @brief The (possibly empty) set of storage services where the file was found */
        std::set<StorageService *> locations;
    };

    /**
     * @brief FileRegistryFileLookupByProximityRequestMessage class
     */
    class FileRegistryFileLookupByProximityRequestMessage : public FileRegistryMessage {
    public:
        FileRegistryFileLookupByProximityRequestMessage(std::string answer_mailbox, WorkflowFile *file,
                                                        std::string reference_host,
                NetworkProximityService *network_proximity_service, double payload);

        /** @brief The mailbox to which the answer message should be sent */
        std::string answer_mailbox;
        /** @brief The file to lookup */
        WorkflowFile *file;
        /**
         * @brief The host from which network proximity will be measured from.
         * If 'host_to_measure_from' is host 'A', and the workflow file resides at hosts 'B'
         * and 'C', then the proximity will be computed between hosts 'A' and 'B', and hosts
         * 'A' and 'C' so that the locations of the workflow file may be sorted with respect to
         * their current network proximity value
         */
        std::string reference_host;

        /**
         * @brief The network proximity service to be used
         */
        NetworkProximityService *network_proximity_service;
    };

    /**
     * @brief FileRegistryFileLookupByProximityAnswerMessage class
     */
    class FileRegistryFileLookupByProximityAnswerMessage : public FileRegistryMessage {
    public:
        FileRegistryFileLookupByProximityAnswerMessage(WorkflowFile *file,
                                                       std::string reference_host,
                                                       std::map<double, StorageService *> locations,
                                                       double payload);

        /** @brief The file to lookup */
        WorkflowFile *file;
        /**
         * @brief The host from which network proximity will be measured from.
         * If 'host_to_measure_from' is host 'A', and the workflow file resides at hosts 'B'
         * and 'C', then the proximity will be computed between hosts 'A' and 'B', and hosts
         * 'A' and 'C' so that the locations of the workflow file may be sorted with respect to
         * their current network proximity value
         */
        std::string reference_host;
        /**
         * @brief A map of all locations where the file resides sorted with respect to their distance from
         * the host 'host_to_measure_from'
         */
        std::map<double, StorageService *> locations;
    };

    /**
     * @brief FileRegistryRemoveEntryRequestMessage class
     */
    class FileRegistryRemoveEntryRequestMessage : public FileRegistryMessage {
    public:
        FileRegistryRemoveEntryRequestMessage(std::string answer_mailbox, WorkflowFile *file,
                                              StorageService *storage_service, double payload);

        /** @brief The mailbox to which the answer message should be sent */
        std::string answer_mailbox;
        /** @brief The file whose entry to remove */
        WorkflowFile *file;
        /** @brief The storage service */
        StorageService *storage_service;
    };

    /**
     * @brief FileRegistryRemoveEntryAnswerMessage
     */
    class FileRegistryRemoveEntryAnswerMessage : public FileRegistryMessage {
    public:
        FileRegistryRemoveEntryAnswerMessage(bool success, double payload);

        /** @brief Whether the remove entry operation was successful or not */
        bool success;
    };

    /**
     * @brief FileRegistryAddEntryRequestMessage class
     */
    class FileRegistryAddEntryRequestMessage : public FileRegistryMessage {
    public:
        FileRegistryAddEntryRequestMessage(std::string answer_mailbox, WorkflowFile *file,
                                           StorageService *storage_service, double payload);

        /** @brief The mailbox to which the answer message should be sent */
        std::string answer_mailbox;
        /** @brief The file for which to add an entry */
        WorkflowFile *file;
        /** @brief The storage service */
        StorageService *storage_service;
    };

    /**
     * @brief FileRegistryAddEntryAnswerMessage class
     */
    class FileRegistryAddEntryAnswerMessage : public FileRegistryMessage {
    public:
        FileRegistryAddEntryAnswerMessage(double payload);
    };

    /***********************/
    /** \endcond          **/
    /***********************/
};


#endif //WRENCH_FILEREGISTRYMESSAGE_H
