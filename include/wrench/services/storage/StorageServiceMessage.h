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
#include <utility>

#include "wrench/services/ServiceMessage.h"
#include "wrench/failure_causes/FailureCause.h"
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
        explicit StorageServiceMessage(sg_size_t payload);
    };


    /**
     * @brief A message sent to a StorageService to enquire about its free space
     */
    class StorageServiceFreeSpaceRequestMessage : public StorageServiceMessage {
    public:
        StorageServiceFreeSpaceRequestMessage(S4U_CommPort *answer_commport, const std::string &path, sg_size_t payload);

        /** @brief CommPort to which the answer message should be sent */
        S4U_CommPort *answer_commport;
        /** @brief The path */
        std::string path;
    };

    /**
     * @brief A message sent by a StorageService in answer to a free space enquiry
     */
    class StorageServiceFreeSpaceAnswerMessage : public StorageServiceMessage {
    public:
        StorageServiceFreeSpaceAnswerMessage(sg_size_t free_space, sg_size_t payload);

        /** @brief The amount of free space in bytes */
        sg_size_t free_space;
    };

    /**
    * @brief A message sent to a StorageService to lookup a file
    */
    class StorageServiceFileLookupRequestMessage : public StorageServiceMessage {
    public:
        StorageServiceFileLookupRequestMessage(S4U_CommPort *answer_commport,
                                               const std::shared_ptr<FileLocation> &location,
                                               sg_size_t payload);

        /** @brief CommPort to which the answer message should be sent */
        S4U_CommPort *answer_commport;
        /** @brief The location to lookup */
        std::shared_ptr<FileLocation> location;
    };

    /**
     * @brief A message sent by a StorageService in answer to a file lookup request
     */
    class StorageServiceFileLookupAnswerMessage : public StorageServiceMessage {
    public:
        StorageServiceFileLookupAnswerMessage(std::shared_ptr<DataFile> file, bool file_is_available, sg_size_t payload);

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
        StorageServiceFileDeleteRequestMessage(S4U_CommPort *answer_commport,
                                               const std::shared_ptr<FileLocation> &location,
                                               sg_size_t payload);

        /** @brief CommPort to which the answer message should be sent */
        S4U_CommPort *answer_commport;
        /** @brief The location to delete  */
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
                                              sg_size_t payload);

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
        StorageServiceFileCopyRequestMessage(S4U_CommPort *answer_commport,
                                             std::shared_ptr<FileLocation> src,
                                             std::shared_ptr<FileLocation> dst,
                                             sg_size_t payload);

        /** @brief CommPort to which the answer message should be sent */
        S4U_CommPort *answer_commport;
        /** @brief The source location */
        std::shared_ptr<FileLocation> src;
        /** @brief The destination location */
        std::shared_ptr<FileLocation> dst;
    };

    /**
     * @brief A message sent by a StorageService in answer to a file copy request
     */
    class StorageServiceFileCopyAnswerMessage : public StorageServiceMessage {
    public:
        StorageServiceFileCopyAnswerMessage(std::shared_ptr<FileLocation> src,
                                            std::shared_ptr<FileLocation> dst,
                                            bool success, std::shared_ptr<FailureCause> cause,
                                            sg_size_t payload);

        /** @brief The source location */
        std::shared_ptr<FileLocation> src;
        /** @brief The destination location */
        std::shared_ptr<FileLocation> dst;
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
        StorageServiceFileWriteRequestMessage(S4U_CommPort *answer_commport,
                                              simgrid::s4u::Host *requesting_host,
                                              const std::shared_ptr<FileLocation> &location,
                                              sg_size_t num_bytes_to_write,
                                              sg_size_t payload);

        /** @brief CommPort to which the answer message should be sent */
        S4U_CommPort *answer_commport;
        /** @brief The requesting host */
        simgrid::s4u::Host *requesting_host;
        /** @brief The file to write */
        std::shared_ptr<FileLocation> location;
        /** @brief The number of of bytes to write to the file */
        sg_size_t num_bytes_to_write;
    };

    /**
     * @brief  A message sent by a StorageService in answer to a file write request
     */
    class StorageServiceFileWriteAnswerMessage : public StorageServiceMessage {
    public:
        StorageServiceFileWriteAnswerMessage(std::shared_ptr<FileLocation> &location,
                                             bool success,
                                             std::shared_ptr<FailureCause> failure_cause,
                                             std::map<S4U_CommPort *, sg_size_t> data_write_commport_and_bytes,
                                             sg_size_t buffer_size,
                                             sg_size_t payload);

        /** @brief The file location hould be written */
        std::shared_ptr<FileLocation> location;
        /** @brief Whether the write operation request was accepted or not */
        bool success;
        /** @brief The cause of the failure, if any, or nullptr */
        std::shared_ptr<FailureCause> failure_cause;
        /** @brief The set of destination commports and the number of bytes to send to each */
        std::map<S4U_CommPort *, sg_size_t> data_write_commport_and_bytes;
        /** @brief The buffer size to use */
        sg_size_t buffer_size;
    };

    /**
     * @brief A message sent to a StorageService to read a file
     */
    class StorageServiceFileReadRequestMessage : public StorageServiceMessage {
    public:
        StorageServiceFileReadRequestMessage(S4U_CommPort *answer_commport,
                                             simgrid::s4u::Host *requesting_host,
                                             std::shared_ptr<FileLocation> location,
                                             sg_size_t num_bytes_to_read,
                                             sg_size_t payload);
        StorageServiceFileReadRequestMessage(StorageServiceFileReadRequestMessage &other);
        explicit StorageServiceFileReadRequestMessage(StorageServiceFileReadRequestMessage *other);
        /** @brief The commport_name to which the answer message should be sent */
        S4U_CommPort *answer_commport;
        /** @brief The requesting host */
        simgrid::s4u::Host *requesting_host = nullptr;
        /** @brief The file to read */
        std::shared_ptr<FileLocation> location;
        /** @brief The number of bytes to read */
        sg_size_t num_bytes_to_read;
    };

    /**
     * @brief A message sent by a StorageService in answer to a file read request
     */
    class StorageServiceFileReadAnswerMessage : public StorageServiceMessage {
    public:
        StorageServiceFileReadAnswerMessage(std::shared_ptr<FileLocation> location,
                                            bool success,
                                            std::shared_ptr<FailureCause> failure_cause,
                                            S4U_CommPort *commport_to_receive_the_file_content,
                                            sg_size_t buffer_size,
                                            unsigned long number_of_sources,
                                            sg_size_t payload);

        /** @brief The location of the file */
        std::shared_ptr<FileLocation> location;
        /** @brief Whether the read operation was successful or not */
        bool success;
        /** @brief The cause of the failure, or nullptr on success */
        std::shared_ptr<FailureCause> failure_cause;
        /** @brief The commport_name to which the file content should be sent (or nullptr) */
        S4U_CommPort *commport_to_receive_the_file_content;
        /** @brief The requested buffer size */
        sg_size_t buffer_size;
        /** @brief The number of sources that will send data */
        unsigned long number_of_sources;
    };

    /**
    * @brief A message sent/received by a StorageService that has a file size as a payload
    */
    class StorageServiceFileContentChunkMessage : public StorageServiceMessage {
    public:
        explicit StorageServiceFileContentChunkMessage(std::shared_ptr<DataFile> file,
                                                       sg_size_t chunk_size, bool last_chunk);

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
        /**
	 * @brief Constructor
	 *
	 * @param location: the file location
	 **/
        explicit StorageServiceAckMessage(std::shared_ptr<FileLocation> location) : StorageServiceMessage(S4U_CommPort::default_control_message_size), location(std::move(location)) {}

        /** @brief The location */
        std::shared_ptr<FileLocation> location;
    };


    /***********************/
    /** \endcond           */
    /***********************/

}// namespace wrench


#endif//WRENCH_STORAGESERVICEMESSAGE_H
