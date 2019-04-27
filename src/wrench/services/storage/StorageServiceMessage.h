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
#include <wrench/services/file_registry/FileRegistryService.h>
#include <wrench/simulation/SimulationTimestampTypes.h>
#include <wrench/simulation/Simulation.h>
#include <wrench/simulation/SimulationOutput.h>

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Top-level class for messages received/sent by a StorageService
     */
    class StorageServiceMessage : public ServiceMessage {
    protected:
        StorageServiceMessage(std::string name, double payload);
    };


    /**
     * @brief A message sent to a StorageService to enquire about its free space
     */
    class StorageServiceFreeSpaceRequestMessage : public StorageServiceMessage {
    public:
        StorageServiceFreeSpaceRequestMessage(std::string answer_mailbox, double payload);
        /** @brief Mailbox to which the answer message should be sent */
        std::string answer_mailbox;
    };

    /**
     * @brief A message sent by a StorageService in answer to a free space enquiry
     */
    class StorageServiceFreeSpaceAnswerMessage : public StorageServiceMessage {
    public:
        StorageServiceFreeSpaceAnswerMessage(double free_space, double payload);

        /** @brief The amount of free space in bytes */
        double free_space;
    };

    /**
    * @brief A message sent to a StorageService to lookup a file
    */
    class StorageServiceFileLookupRequestMessage : public StorageServiceMessage {
    public:
        StorageServiceFileLookupRequestMessage(std::string answer_mailbox, WorkflowFile *file, std::string& dst_partition, double payload);

        /** @brief Mailbox to which the answer message should be sent */
        std::string answer_mailbox;
        /** @brief The file to lookup */
        WorkflowFile *file;
        /** @brief The file partition where to lookup the file for */
        std::string dst_partition;
    };

    /**
     * @brief A message sent by a StorageService in answer to a file lookup request
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
     * @brief A message sent to a StorageService to delete a file
     */
    class StorageServiceFileDeleteRequestMessage : public StorageServiceMessage {
    public:
        StorageServiceFileDeleteRequestMessage(std::string answer_mailbox,
                                               WorkflowFile *file,
                                               std::string& dst_partition,
                                               double payload);

        /** @brief Mailbox to which the answer message should be sent */
        std::string answer_mailbox;
        /** @brief The file to delete */
        WorkflowFile *file;
        /** @brief The file partition from where the file will be deleted */
        std::string dst_partition;
    };

    /**
     * @brief A message sent  by a StorageService in answer to a file deletion request
     */
    class StorageServiceFileDeleteAnswerMessage : public StorageServiceMessage {
    public:
        StorageServiceFileDeleteAnswerMessage(WorkflowFile *file,
                                              std::shared_ptr<StorageService>  storage_service,
                                              bool success,
                                              std::shared_ptr<FailureCause> failure_cause,
                                              double payload);

        /** @brief The file that was deleted (or not) */
        WorkflowFile *file;
        /** @brief The storage service on which the deletion happened (or not) */
        std::shared_ptr<StorageService>  storage_service;
        /** @brief Whether the deletion was successful */
        bool success;
        /** @brief The cause of the failure, or nullptr if success */
        std::shared_ptr<FailureCause> failure_cause;
    };

    /**
    * @brief A message sent to a StorageService to copy a file from another StorageService
    */
    class StorageServiceFileCopyRequestMessage : public StorageServiceMessage {
    public:
        StorageServiceFileCopyRequestMessage(std::string answer_mailbox, WorkflowFile *file, std::shared_ptr<StorageService> src,
                                             std::string& src_partition, std::shared_ptr<StorageService > dst, std::string& dst_partition,
                                             std::shared_ptr<FileRegistryService> file_registry_service,
                                             SimulationTimestampFileCopyStart *start_timestamp,
                                             double payload);

        /** @brief Mailbox to which the answer message should be sent */
        std::string answer_mailbox;
        /** @brief The file to copy */
        WorkflowFile *file;
        /** @brief The storage service that the file will be copied from */
        std::shared_ptr<StorageService> src;
        /** @brief The file partition from where the file will be copied from */
        std::string src_partition;
        /** @brief The StorageService where the file will be copied to */
        std::shared_ptr<StorageService> dst;
        /** @brief The file partition inside the storage service where the file will be copied to */
        std::string dst_partition;
        /** @brief The file registry service to update, or none if nullptr */
        std::shared_ptr<FileRegistryService> file_registry_service;
        /** @brief The SimulationTimestampFileCopyStart associated with this file copy request */
        SimulationTimestampFileCopyStart *start_timestamp;
    };

    /**
     * @brief A message sent by a StorageService in answer to a file copy request
     */
    class StorageServiceFileCopyAnswerMessage : public StorageServiceMessage {
    public:
        StorageServiceFileCopyAnswerMessage(WorkflowFile *file,
                                            std::shared_ptr<StorageService> storage_service,
                                            std::string dst_partition,
                                            std::shared_ptr<FileRegistryService> file_registry_service,
                                            bool file_registry_service_updated,
                                            bool success, std::shared_ptr<FailureCause> cause,
                                            double payload);

        /** @brief The file was was copied, or not */
        WorkflowFile *file;
        /** @brief The storage service that performed the copy (i.e., which stored the file) */
        std::shared_ptr<StorageService> storage_service;
        /** @brief The destination partition */
        std::string dst_partition;
        /** @brief The file registry service that the user had requested be updated, or nullptr if none */
        std::shared_ptr<FileRegistryService> file_registry_service;
        /** @brief Whether a file registry service has been updated or not */
        bool file_registry_service_updated;
        /** @brief Whether the copy was successful */
        bool success;
        /** @brief The cause of the failure, or nullptr if success */
        std::shared_ptr<FailureCause> failure_cause;

    };

    /**
    * @brief A message sent to a StorageService to write a file
    */
    class StorageServiceFileWriteRequestMessage : public StorageServiceMessage {
    public:
        StorageServiceFileWriteRequestMessage(std::string answer_mailbox, WorkflowFile *file, std::string& dst_partition,
                double payload);

        /** @brief Mailbox to which the answer message should be sent */
        std::string answer_mailbox;
        /** @brief The file to write */
        WorkflowFile *file;
        /** @brief The file partition to write the file to */
        std::string dst_partition;
    };

    /**
     * @brief  A message sent by a StorageService in answer to a file write request
     */
    class StorageServiceFileWriteAnswerMessage : public StorageServiceMessage {
    public:
        StorageServiceFileWriteAnswerMessage(WorkflowFile *file,
                                             std::shared_ptr<StorageService>  storage_service,
                                             bool success,
                                             std::shared_ptr<FailureCause> failure_cause,
                                             std::string data_write_mailbox_name,
                                             double payload);

        /** @brief The workflow file that should be written */
        WorkflowFile *file;
        /** @brief The storage service on which the file should be written */
        std::shared_ptr<StorageService>  storage_service;
        /** @brief Whether the write operation request was accepted or not */
        bool success;
        /** @brief The mailbox on which to send the file */
        std::string data_write_mailbox_name;
        /** @brief The cause of the failure, if any, or nullptr */
        std::shared_ptr<FailureCause> failure_cause;
    };

    /**
     * @brief A message sent to a StorageService to read a file
     */
    class StorageServiceFileReadRequestMessage : public StorageServiceMessage {
    public:
        StorageServiceFileReadRequestMessage(std::string answer_mailbox,
                                             std::string mailbox_to_receive_the_file_content,
                                             WorkflowFile *file, std::string& src_partition,
                                             double payload);

        /** @brief The mailbox to which the answer message should be sent */
        std::string answer_mailbox;
        /** @brief The mailbox to which the file content should be sent */
        std::string mailbox_to_receive_the_file_content;
        /** @brief The file to read */
        WorkflowFile *file;
        /** @brief The source partition from which to read the file */
        std::string src_partition;
    };

    /**
     * @brief A message sent by a StorageService in answer to a file read request
     */
    class StorageServiceFileReadAnswerMessage : public StorageServiceMessage {
    public:
        StorageServiceFileReadAnswerMessage(WorkflowFile *file,
                                            std::shared_ptr<StorageService>  storage_service,
                                            bool success,
                                            std::shared_ptr<FailureCause> failure_cause,
                                            double payload);

        /** @brief The file that was read */
        WorkflowFile *file;
        /** @brief The storage service on which the file was read */
        std::shared_ptr<StorageService>  storage_service;
        /** @brief Whether the read operation was successful or not */
        bool success;
        /** @brief The cause of the failure, or nullptr on success */
        std::shared_ptr<FailureCause> failure_cause;
    };

    /**
    * @brief A message sent/received by a StorageService that has a file size as a payload
    */
    class StorageServiceFileContentMessage : public StorageServiceMessage {
    public:
        explicit StorageServiceFileContentMessage(WorkflowFile *file);

        /** @brief The file */
        WorkflowFile *file;
    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_STORAGESERVICEMESSAGE_H
