/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_STORAGESERVICEMESSAGE_H
#define WRENCH_STORAGESERVICEMESSAGE_H


#include <services/ServiceMessage.h>

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    class StorageServiceMessage : public ServiceMessage {
    protected:
        StorageServiceMessage(std::string name, double payload);
    };




    /**
     * @brief "FREE_SPACE_REQUEST" SimulationMessage class
     */
    class StorageServiceFreeSpaceRequestMessage : public StorageServiceMessage {
    public:
        StorageServiceFreeSpaceRequestMessage(std::string answer_mailbox, double payload);

        std::string answer_mailbox;
    };

    /**
     * @brief "FREE_SPACE_ANSWER" SimulationMessage class
     */
    class StorageServiceFreeSpaceAnswerMessage : public StorageServiceMessage {
    public:
        StorageServiceFreeSpaceAnswerMessage(double free_space, double payload);

        double free_space;
    };

    /**
    * @brief "FILE_LOOKUP_REQUEST" SimulationMessage class
    */
    class StorageServiceFileLookupRequestMessage : public StorageServiceMessage {
    public:
        StorageServiceFileLookupRequestMessage(std::string answer_mailbox, WorkflowFile *file, double payload);

        std::string answer_mailbox;
        WorkflowFile *file;
    };

    /**
     * @brief "FILE_LOOKUP_ANSWER" SimulationMessage class
     */
    class StorageServiceFileLookupAnswerMessage : public StorageServiceMessage {
    public:
        StorageServiceFileLookupAnswerMessage(WorkflowFile *file, bool file_is_availalbe, double payload);

        WorkflowFile *file;
        bool file_is_available;
    };

    /**
     * @brief "FILE_DELETE_REQUEST" SimulationMessage class
     */
    class StorageServiceFileDeleteRequestMessage : public StorageServiceMessage {
    public:
        StorageServiceFileDeleteRequestMessage(std::string answer_mailbox,
                                 WorkflowFile *file,
                                 double payload);

        std::string answer_mailbox;
        WorkflowFile *file;
    };

    /**
     * @brief "FILE_DELETE_ANSWER" SimulationMessage class
     */
    class StorageServiceFileDeleteAnswerMessage : public StorageServiceMessage {
    public:
        StorageServiceFileDeleteAnswerMessage(WorkflowFile *file,
                                StorageService *storage_service,
                                bool success,
                                WorkflowExecutionFailureCause *failure_cause,
                                double payload);

        WorkflowFile *file;
        StorageService *storage_service;
        bool success;
        WorkflowExecutionFailureCause *failure_cause;
    };

    /**
    * @brief "FILE_COPY_REQUEST" SimulationMessage class
    */
    class StorageServiceFileCopyRequestMessage : public StorageServiceMessage {
    public:
        StorageServiceFileCopyRequestMessage(std::string answer_mailbox, WorkflowFile *file, StorageService *src, double payload);

        std::string answer_mailbox;
        WorkflowFile *file;
        StorageService *src;
    };

    /**
     * @brief "FILE_COPY_ANSWER" SimulationMessage class
     */
    class StorageServiceFileCopyAnswerMessage : public StorageServiceMessage {
    public:
        StorageServiceFileCopyAnswerMessage(WorkflowFile *file, StorageService *storage_service,
                              bool success, WorkflowExecutionFailureCause *cause, double payload);

        WorkflowFile *file;
        StorageService *storage_service;
        bool success;
        WorkflowExecutionFailureCause *failure_cause;
    };

    /**
    * @brief "FILE_WRITE_REQUEST" SimulationMessage class
    */
    class StorageServiceFileWriteRequestMessage : public StorageServiceMessage {
    public:
        StorageServiceFileWriteRequestMessage(std::string answer_mailbox, WorkflowFile *file, double payload);

        std::string answer_mailbox;
        WorkflowFile *file;
    };

    /**
     * @brief "FILE_WRITE_ANSWER" SimulationMessage class
     */
    class StorageServiceFileWriteAnswerMessage : public StorageServiceMessage {
    public:
        StorageServiceFileWriteAnswerMessage(WorkflowFile *file,
                                StorageService *storage_service,
                                bool success,
                                WorkflowExecutionFailureCause *failure_cause,
                                std::string data_write_mailbox_name,
                                double payload);

        WorkflowFile *file;
        StorageService *storage_service;
        bool success;
        WorkflowExecutionFailureCause *failure_cause;
        std::string data_write_mailbox_name;
    };

    /**
     * @brief "FILE_READ_REQUEST" SimulationMessage class
     */
    class StorageServiceFileReadRequestMessage : public StorageServiceMessage {
    public:
        StorageServiceFileReadRequestMessage(std::string answer_mailbox, WorkflowFile *file, double payload);

        std::string answer_mailbox;
        WorkflowFile *file;
    };

    /**
     * @brief "FILE_READ_ANSWER" SimulationMessage class
     */
    class StorageServiceFileReadAnswerMessage : public StorageServiceMessage {
    public:
        StorageServiceFileReadAnswerMessage(WorkflowFile *file,
                                  StorageService *storage_service,
                                  bool success,
                                  WorkflowExecutionFailureCause *failure_cause,
                                  double payload);

        WorkflowFile *file;
        StorageService *storage_service;
        bool success;
        WorkflowExecutionFailureCause *failure_cause;
    };

    /**
    * @brief "FILE_CONTENT" SimulationMessage class
    */
    class StorageServiceFileContentMessage : public StorageServiceMessage {
    public:
        StorageServiceFileContentMessage(WorkflowFile *file);

        WorkflowFile *file;
    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_STORAGESERVICEMESSAGE_H
