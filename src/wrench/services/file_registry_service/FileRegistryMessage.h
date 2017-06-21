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

        std::string answer_mailbox;
        WorkflowFile *file;
    };

    /**
     * @brief FileRegistryFileLookupAnswerMessage class
     */
    class FileRegistryFileLookupAnswerMessage: public FileRegistryMessage {
    public:
        FileRegistryFileLookupAnswerMessage(WorkflowFile *file, std::set<StorageService*> locations, double payload);

        WorkflowFile *file;
        std::set<StorageService*> locations;
    };

    /**
     * @brief FileRegistryRemoveEntryRequestMessage class
     */
    class FileRegistryRemoveEntryRequestMessage: public FileRegistryMessage {
    public:
        FileRegistryRemoveEntryRequestMessage(std::string answer_mailbox, WorkflowFile *file, StorageService *storage_service, double payload);

        std::string answer_mailbox;
        WorkflowFile *file;
        StorageService *storage_service;
    };

    /**
     * @brief FileRegistryRemoveEntryAnswerMessage
     */
    class FileRegistryRemoveEntryAnswerMessage: public FileRegistryMessage {
    public:
        FileRegistryRemoveEntryAnswerMessage(bool success, double payload);

        bool success;
    };

    /**
     * @brief FileRegistryAddEntryRequestMessage class
     */
    class FileRegistryAddEntryRequestMessage: public FileRegistryMessage {
    public:
        FileRegistryAddEntryRequestMessage(std::string answer_mailbox, WorkflowFile *file, StorageService *storage_service, double payload);

        std::string answer_mailbox;
        WorkflowFile *file;
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
