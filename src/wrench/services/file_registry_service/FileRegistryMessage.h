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
    class FileRegistryFileLookupRequestMessage: public FileRegistryMessage {
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
    class FileRegistryFileLookupAnswerMessage: public FileRegistryMessage {
    public:
        FileRegistryFileLookupAnswerMessage(WorkflowFile *file, std::set<StorageService*> locations, double payload);

        /** @brief The file that was looked up */
        WorkflowFile *file;
        /** @brief The (possibly empty) set of storage services where the file was found */
        std::set<StorageService*> locations;
    };

    /**
     * @brief FileRegistryRemoveEntryRequestMessage class
     */
    class FileRegistryRemoveEntryRequestMessage: public FileRegistryMessage {
    public:
        FileRegistryRemoveEntryRequestMessage(std::string answer_mailbox, WorkflowFile *file, StorageService *storage_service, double payload);

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
    class FileRegistryRemoveEntryAnswerMessage: public FileRegistryMessage {
    public:
        FileRegistryRemoveEntryAnswerMessage(bool success, double payload);

        /** @brief Whether the remove entry operation was successful or not */
        bool success;
    };

    /**
     * @brief FileRegistryAddEntryRequestMessage class
     */
    class FileRegistryAddEntryRequestMessage: public FileRegistryMessage {
    public:
        FileRegistryAddEntryRequestMessage(std::string answer_mailbox, WorkflowFile *file, StorageService *storage_service, double payload);

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
    class FileRegistryAddEntryAnswerMessage: public FileRegistryMessage {
    public:
        FileRegistryAddEntryAnswerMessage(double payload);
    };

    /***********************/
    /** \endcond          **/
    /***********************/
};


#endif //WRENCH_FILEREGISTRYMESSAGE_H
