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

#include "wrench/services/ServiceMessage.h"
#include "wrench/failure_causes/FailureCause.h"
#include "wrench/services/file_registry/FileRegistryService.h"
#include "wrench/simulation/SimulationTimestampTypes.h"
#include "wrench/simulation/Simulation.h"
#include "wrench/simulation/SimulationOutput.h"

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Top-level class for messages received/sent by a StorageService
     */
    class StorageServiceMessage : public ServiceMessage {
    protected:
        StorageServiceMessage(double payload);
    };


    /**
     * @brief A message sent to a StorageService to enquire about its free space
     */
    class StorageServiceFreeSpaceRequestMessage : public StorageServiceMessage {
    public:
        StorageServiceFreeSpaceRequestMessage(simgrid::s4u::Mailbox *answer_mailbox, double payload);

        /** @brief Mailbox to which the answer message should be sent */
        simgrid::s4u::Mailbox *answer_mailbox;
    };

    /**
     * @brief A message sent by a StorageService in answer to a free space enquiry
     */
    class StorageServiceFreeSpaceAnswerMessage : public StorageServiceMessage {
    public:
        StorageServiceFreeSpaceAnswerMessage(std::map<std::string, double> free_space, double payload);

        /** @brief The amount of free space in bytes for each mount point, in a map */
        std::map<std::string, double> free_space;
    };

    /**
    * @brief A message sent to a StorageService to lookup a file
    */
    class StorageServiceFileLookupRequestMessage : public StorageServiceMessage {
    public:
        StorageServiceFileLookupRequestMessage(simgrid::s4u::Mailbox *answer_mailbox, std::shared_ptr<DataFile> file,
                                               std::shared_ptr<FileLocation> location, double payload);

        /** @brief Mailbox to which the answer message should be sent */
        simgrid::s4u::Mailbox *answer_mailbox;
        /** @brief The file to lookup */
        std::shared_ptr<DataFile> file;
        /** @brief The file location (hopefully) */
        std::shared_ptr<FileLocation> location;
    };

    /**
     * @brief A message sent by a StorageService in answer to a file lookup request
     */
    class StorageServiceFileLookupAnswerMessage : public StorageServiceMessage {
    public:
        StorageServiceFileLookupAnswerMessage(std::shared_ptr<DataFile> file, bool file_is_available, double payload);

        /** @brief The file that was looked up */
        std::shared_ptr<DataFile> file;

        /** @brief Whether the file was found */
        bool file_is_available;
    };

    /**
     * @brief A message sent to a StorageService to delete a file
     */
    class StorageServiceFileDeleteRequestMessage : public StorageServiceMessage {
    public:
        StorageServiceFileDeleteRequestMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                               std::shared_ptr<DataFile> file,
                                               std::shared_ptr<FileLocation> location,
                                               double payload);

        /** @brief Mailbox to which the answer message should be sent */
        simgrid::s4u::Mailbox *answer_mailbox;
        /** @brief The file to delete */
        std::shared_ptr<DataFile> file;
        /** @brief The location where the file will be deleted */
        std::shared_ptr<FileLocation> location;
    };

    /**
     * @brief A message sent  by a StorageService in answer to a file deletion request
     */
    class StorageServiceFileDeleteAnswerMessage : public StorageServiceMessage {
    public:
        StorageServiceFileDeleteAnswerMessage(std::shared_ptr<DataFile> file,
                                              std::shared_ptr<StorageService> storage_service,
                                              bool success,
                                              std::shared_ptr<FailureCause> failure_cause,
                                              double payload);

        /** @brief The file that was deleted (or not) */
        std::shared_ptr<DataFile> file;
        /** @brief The storage service on which the deletion happened (or not) */
        std::shared_ptr<StorageService> storage_service;
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
        StorageServiceFileCopyRequestMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                             std::shared_ptr<DataFile> file,
                                             std::shared_ptr<FileLocation> src,
                                             std::shared_ptr<FileLocation> dst,
                                             std::shared_ptr<FileRegistryService> file_registry_service,
                                             double payload);

        /** @brief Mailbox to which the answer message should be sent */
        simgrid::s4u::Mailbox *answer_mailbox;
        /** @brief The file to copy */
        std::shared_ptr<DataFile> file;
        /** @brief The source location */
        std::shared_ptr<FileLocation> src;
        /** @brief The destination location */
        std::shared_ptr<FileLocation> dst;
        /** @brief The file registry service to update, or none if nullptr */
        std::shared_ptr<FileRegistryService> file_registry_service;
    };

    /**
     * @brief A message sent by a StorageService in answer to a file copy request
     */
    class StorageServiceFileCopyAnswerMessage : public StorageServiceMessage {
    public:
        StorageServiceFileCopyAnswerMessage(std::shared_ptr<DataFile> file,
                                            std::shared_ptr<FileLocation> src,
                                            std::shared_ptr<FileLocation> dst,
                                            std::shared_ptr<FileRegistryService> file_registry_service,
                                            bool file_registry_service_updated,
                                            bool success, std::shared_ptr<FailureCause> cause,
                                            double payload);

        /** @brief The file was was copied, or not */
        std::shared_ptr<DataFile> file;
        /** @brief The source location */
        std::shared_ptr<FileLocation> src;
        /** @brief The destination location */
        std::shared_ptr<FileLocation> dst;
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
        StorageServiceFileWriteRequestMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                              std::shared_ptr<DataFile> file,
                                              std::shared_ptr<FileLocation> location,
                                              double buffer_size,
                                              double payload);

        /** @brief Mailbox to which the answer message should be sent */
        simgrid::s4u::Mailbox *answer_mailbox;
        /** @brief The file to write */
        std::shared_ptr<DataFile> file;
        /** @brief The location to write the file to */
        std::shared_ptr<FileLocation> location;
        /** @brief The buffer size to use */
        double buffer_size;
    };

    /**
     * @brief  A message sent by a StorageService in answer to a file write request
     */
    class StorageServiceFileWriteAnswerMessage : public StorageServiceMessage {
    public:
        StorageServiceFileWriteAnswerMessage(std::shared_ptr<DataFile> file,
                                             std::shared_ptr<FileLocation> location,
                                             bool success,
                                             std::shared_ptr<FailureCause> failure_cause,
                                             simgrid::s4u::Mailbox *data_write_mailbox_name,
                                             double payload);

        /** @brief The workflow file that should be written */
        std::shared_ptr<DataFile> file;
        /** @brief The location at which the file should be written */
        std::shared_ptr<FileLocation> location;
        /** @brief Whether the write operation request was accepted or not */
        bool success;
        /** @brief The mailbox on which to send the file */
        simgrid::s4u::Mailbox *data_write_mailbox;
        /** @brief The cause of the failure, if any, or nullptr */
        std::shared_ptr<FailureCause> failure_cause;
    };

    /**
     * @brief A message sent to a StorageService to read a file
     */
    class StorageServiceFileReadRequestMessage : public StorageServiceMessage {
    public:
        StorageServiceFileReadRequestMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                             simgrid::s4u::Mailbox *mailbox_to_receive_the_file_content,
                                             std::shared_ptr<DataFile> file,
                                             std::shared_ptr<FileLocation> location,
                                             double num_bytes_to_read,
                                             double payload);
        StorageServiceFileReadRequestMessage(StorageServiceFileReadRequestMessage &other);
        StorageServiceFileReadRequestMessage(StorageServiceFileReadRequestMessage *other);
        /** @brief The mailbox to which the answer message should be sent */
        simgrid::s4u::Mailbox *answer_mailbox;
        /** @brief The mailbox to which the file content should be sent */
        simgrid::s4u::Mailbox *mailbox_to_receive_the_file_content;
        /** @brief The file to read */
        std::shared_ptr<DataFile> file;
        /** @brief The location from which to  read the file */
        std::shared_ptr<FileLocation> location;
        /** @brief The number of bytes to read */
        double num_bytes_to_read;
    };

    /**
     * @brief A message sent by a StorageService in answer to a file read request
     */
    class StorageServiceFileReadAnswerMessage : public StorageServiceMessage {
    public:
        StorageServiceFileReadAnswerMessage(std::shared_ptr<DataFile> file,
                                            std::shared_ptr<FileLocation> location,
                                            bool success,
                                            std::shared_ptr<FailureCause> failure_cause,
                                            double buffer_size,
                                            double payload);

        /** @brief The file that was read */
        std::shared_ptr<DataFile> file;
        /** @brief The location of the file */
        std::shared_ptr<FileLocation> location;
        /** @brief Whether the read operation was successful or not */
        bool success;
        /** @brief The cause of the failure, or nullptr on success */
        std::shared_ptr<FailureCause> failure_cause;
        /** @brief The requested buffer size */
        double buffer_size;
    };

    /**
    * @brief A message sent/received by a StorageService that has a file size as a payload
    */
    class StorageServiceFileContentChunkMessage : public StorageServiceMessage {
    public:
        explicit StorageServiceFileContentChunkMessage(std::shared_ptr<DataFile> file,
                                                       double chunk_size, bool last_chunk);

        /** @brief The file */
        std::shared_ptr<DataFile> file;
        /** @brief Whether this is the last file chunk */
        bool last_chunk;
    };


    /**
    * @brief A message sent by a StorageService as an ack
    */
    class StorageServiceAckMessage : public StorageServiceMessage {
    public:
        StorageServiceAckMessage() : StorageServiceMessage(0) {}
    };


    /***********************/
    /** \endcond           */
    /***********************/

};// namespace wrench


#endif//WRENCH_STORAGESERVICEMESSAGE_H
