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


#include <memory>

#include <wrench/services/ServiceMessage.h>
#include <wrench/workflow/execution_events/FailureCause.h>


namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Top-level StorageServiceMessage class
     */
    class StorageServiceMessage : public ServiceMessage {
    protected:
        StorageServiceMessage(std::string name, double payload);
    };


    /**
     * @brief StorageServiceFreeSpaceRequestMessage class
     */
    class StorageServiceFreeSpaceRequestMessage : public StorageServiceMessage {
    public:
        StorageServiceFreeSpaceRequestMessage(std::string answer_mailbox, double payload);
        /** @brief Mailbox to which the answer message should be sent */
        std::string answer_mailbox;
    };

    /**
     * @brief StorageServiceFreeSpaceAnswerMessage class
     */
    class StorageServiceFreeSpaceAnswerMessage : public StorageServiceMessage {
    public:
        StorageServiceFreeSpaceAnswerMessage(double free_space, double payload);

        /** @brief The amount of free space in bytes */
        double free_space;
    };

    /**
    * @brief StorageServiceFileLookupRequestMessage class
    */
    class StorageServiceFileLookupRequestMessage : public StorageServiceMessage {
    public:
        StorageServiceFileLookupRequestMessage(std::string answer_mailbox, WorkflowFile *file, double payload);

        /** @brief Mailbox to which the answer message should be sent */
        std::string answer_mailbox;
        /** @brief The file to lookup */
        WorkflowFile *file;
    };

    /**
     * @brief StorageServiceFileLookupAnswerMessage class
     */
    class StorageServiceFileLookupAnswerMessage : public StorageServiceMessage {
    public:
        StorageServiceFileLookupAnswerMessage(WorkflowFile *file, bool file_is_available, double payload);

        /** @brief The file that was looked up */
        WorkflowFile *file;
        /** @brief Whether the file was found */
        bool file_is_available;
    };

    /**
     * @brief StorageServiceFileDeleteRequestMessage class
     */
    class StorageServiceFileDeleteRequestMessage : public StorageServiceMessage {
    public:
        StorageServiceFileDeleteRequestMessage(std::string answer_mailbox,
                                               WorkflowFile *file,
                                               double payload);

        /** @brief Mailbox to which the answer message should be sent */
        std::string answer_mailbox;
        /** @brief The file to delete */
        WorkflowFile *file;
    };

    /**
     * @brief StorageServiceFileDeleteAnswerMessage class
     */
    class StorageServiceFileDeleteAnswerMessage : public StorageServiceMessage {
    public:
        StorageServiceFileDeleteAnswerMessage(WorkflowFile *file,
                                              StorageService *storage_service,
                                              bool success,
                                              std::shared_ptr<FailureCause> failure_cause,
                                              double payload);

        /** @brief The file that was deleted (or not) */
        WorkflowFile *file;
        /** @brief The storage service on which the deletion happened (or not) */
        StorageService *storage_service;
        /** @brief Whether the deletion was successful */
        bool success;
        /** @brief The cause of the failure, or nullptr if success */
        std::shared_ptr<FailureCause> failure_cause;
    };

    /**
    * @brief StorageServiceFileCopyRequestMessage class
    */
    class StorageServiceFileCopyRequestMessage : public StorageServiceMessage {
    public:
        StorageServiceFileCopyRequestMessage(std::string answer_mailbox, WorkflowFile *file, StorageService *src,
                                             double payload);

        /** @brief Mailbox to which the answer message should be sent */
        std::string answer_mailbox;
        /** @brief The file to copy */
        WorkflowFile *file;
        /** @brief The storage service from which to copy the file */
        StorageService *src;
    };

    /**
     * @brief StorageServiceFileCopyAnswerMessage class
     */
    class StorageServiceFileCopyAnswerMessage : public StorageServiceMessage {
    public:
        StorageServiceFileCopyAnswerMessage(WorkflowFile *file, StorageService *storage_service,
                                            bool success, std::shared_ptr<FailureCause> cause, double payload);

        /** @brief The file was was copied, or not */
        WorkflowFile *file;
        /** @brief The storage service that performed the copy */
        StorageService *storage_service;
        /** @brief Whether the copy was successful */
        bool success;
        /** @brief The cause of the failure, or nullptr if success */
        std::shared_ptr<FailureCause> failure_cause;
    };

    /**
    * @brief StorageServiceFileWriteRequestMessage class
    */
    class StorageServiceFileWriteRequestMessage : public StorageServiceMessage {
    public:
        StorageServiceFileWriteRequestMessage(std::string answer_mailbox, WorkflowFile *file, double payload);

        /** @brief Mailbox to which the answer message should be sent */
        std::string answer_mailbox;
        /** @brief The file to write */
        WorkflowFile *file;
    };

    /**
     * @brief StorageServiceFileWriteAnswerMessage class
     */
    class StorageServiceFileWriteAnswerMessage : public StorageServiceMessage {
    public:
        StorageServiceFileWriteAnswerMessage(WorkflowFile *file,
                                             StorageService *storage_service,
                                             bool success,
                                             std::shared_ptr<FailureCause> failure_cause,
                                             std::string data_write_mailbox_name,
                                             double payload);

        /** @brief The workflow file that should be written */
        WorkflowFile *file;
        /** @brief The storage service on which the file should be written */
        StorageService *storage_service;
        /** @brief Whether the write operation request was accepted or not */
        bool success;
        /** @brief The mailbox on which to send the file */
        std::string data_write_mailbox_name;
        /** @brief The cause of the failure, if any, or nullptr */
        std::shared_ptr<FailureCause> failure_cause;
    };

    /**
     * @brief StorageServiceFileReadRequestMessage class
     */
    class StorageServiceFileReadRequestMessage : public StorageServiceMessage {
    public:
        StorageServiceFileReadRequestMessage(std::string answer_mailbox,
                                             std::string mailbox_to_receive_the_file_content,
                                             WorkflowFile *file, double payload);

        /** @brief The mailbox to which the answer message should be sent */
        std::string answer_mailbox;
        /** @brief The mailbox to which the file content should be sent */
        std::string mailbox_to_receive_the_file_content;
        /** @brief The file to read */
        WorkflowFile *file;
    };

    /**
     * @brief StorageServiceFileReadAnswerMessage class
     */
    class StorageServiceFileReadAnswerMessage : public StorageServiceMessage {
    public:
        StorageServiceFileReadAnswerMessage(WorkflowFile *file,
                                            StorageService *storage_service,
                                            bool success,
                                            std::shared_ptr<FailureCause> failure_cause,
                                            double payload);

        /** @brief The file that was read */
        WorkflowFile *file;
        /** @brief The storage service on which the file was read */
        StorageService *storage_service;
        /** @brief Whether the read operation was successful or not */
        bool success;
        /** @brief The cause of the failure, or nullptr on success */
        std::shared_ptr<FailureCause> failure_cause;
    };

    /**
    * @brief StorageServiceFileContentMessage class
    */
    class StorageServiceFileContentMessage : public StorageServiceMessage {
    public:
        StorageServiceFileContentMessage(WorkflowFile *file);

        /** @brief The file */
        WorkflowFile *file;
    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_STORAGESERVICEMESSAGE_H
