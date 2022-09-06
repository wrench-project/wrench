/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include "wrench/services/storage/StorageServiceMessage.h"
#include <wrench/data_file/DataFile.h>
#include <wrench/simulation/SimulationTimestampTypes.h>
#include <wrench/simulation/Simulation.h>
#include <wrench/simulation/SimulationOutput.h>
#include <wrench/services/compute/cloud/CloudComputeService.h>
#include <wrench/services/compute/virtualized_cluster/VirtualizedClusterComputeService.h>
#include <wrench/services/storage/StorageService.h>

#include <utility>

namespace wrench {

    /**
     * @brief Constructor
     * @param payload: the message size in bytes
     */
    StorageServiceMessage::StorageServiceMessage(double payload) : ServiceMessage(payload) {
    }

    /**
    * @brief Constructor
    * @param answer_mailbox: the mailbox to which to send the answer
    * @param payload: the message size in bytes
    *
    * @throw std::invalid_argument
    */
    StorageServiceFreeSpaceRequestMessage::StorageServiceFreeSpaceRequestMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                                                                 double payload)
        : StorageServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if (answer_mailbox == nullptr) {
            throw std::invalid_argument(
                    "StorageServiceFreeSpaceRequestMessage::StorageServiceFreeSpaceRequestMessage(): Invalid arguments");
        }
#endif
        this->answer_mailbox = answer_mailbox;
    }

    /**
     * @brief Constructor
     * @param free_space: the free space, in bytes, at each mount point, as a map
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    StorageServiceFreeSpaceAnswerMessage::StorageServiceFreeSpaceAnswerMessage(
            std::map<std::string, double> free_space, double payload)
        : StorageServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS

        for (auto const &f: free_space) {
            if (f.second < 0) {
                throw std::invalid_argument(
                        "StorageServiceFreeSpaceAnswerMessage::StorageServiceFreeSpaceAnswerMessage(): Invalid arguments");
            }
        }
#endif
        this->free_space = std::move(free_space);
    }

    /**
    * @brief Constructor
    * @param answer_mailbox: the mailbox to which to send the answer
    * @param file: the file
    * @param location: the file location (hopefully)
    * @param payload: the message size in bytes
    *
    * @throw std::invalid_argument
    */
    StorageServiceFileLookupRequestMessage::StorageServiceFileLookupRequestMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                                                                   std::shared_ptr<DataFile> file,
                                                                                   std::shared_ptr<FileLocation> location,
                                                                                   double payload)
        : StorageServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((file == nullptr) || (location == nullptr) || (answer_mailbox == nullptr)) {
            throw std::invalid_argument(
                    "StorageServiceFileLookupRequestMessage::StorageServiceFileLookupRequestMessage(): Invalid arguments");
        }
#endif
        this->answer_mailbox = answer_mailbox;
        this->file = std::move(file);
        this->location = std::move(location);
    }

    /**
     * @brief Constructor
     * @param file: the file
     * @param file_is_available: true if the file is available on the storage system
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    StorageServiceFileLookupAnswerMessage::StorageServiceFileLookupAnswerMessage(std::shared_ptr<DataFile> file,
                                                                                 bool file_is_available,
                                                                                 double payload)
        : StorageServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if (file == nullptr) {
            throw std::invalid_argument(
                    "StorageServiceFileLookupAnswerMessage::StorageServiceFileLookupAnswerMessage(): Invalid arguments");
        }
#endif
        this->file = std::move(file);
        this->file_is_available = file_is_available;
    }

    /**
     * @brief Constructor
     * @param answer_mailbox: the mailbox to which to send the answer
     * @param file: the file
     * @param location: the file location
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    StorageServiceFileDeleteRequestMessage::StorageServiceFileDeleteRequestMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                                                                   std::shared_ptr<DataFile> file,
                                                                                   std::shared_ptr<FileLocation> location,
                                                                                   double payload)
        : StorageServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((answer_mailbox == nullptr) || (file == nullptr) || (location == nullptr)) {
            throw std::invalid_argument(
                    "StorageServiceFileDeleteRequestMessage::StorageServiceFileDeleteRequestMessage(): Invalid arguments");
        }
#endif
        this->answer_mailbox = answer_mailbox;
        this->file = std::move(file);
        this->location = std::move(location);
    }

    /**
     * @brief Constructor
     * @param file: the file to delete
     * @param storage_service: the storage service on which to delete it
     * @param success: whether the deletion was successful
     * @param failure_cause: the cause of a failure (nullptr means "no failure")
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    StorageServiceFileDeleteAnswerMessage::StorageServiceFileDeleteAnswerMessage(std::shared_ptr<DataFile> file,
                                                                                 std::shared_ptr<StorageService> storage_service,
                                                                                 bool success,
                                                                                 std::shared_ptr<FailureCause> failure_cause,
                                                                                 double payload)
        : StorageServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((file == nullptr) || (storage_service == nullptr) ||
            (success && (failure_cause != nullptr)) ||
            ((!success) && (failure_cause == nullptr))) {
            throw std::invalid_argument(
                    "StorageServiceFileDeleteAnswerMessage::StorageServiceFileDeleteAnswerMessage(): Invalid arguments");
        }
#endif
        this->file = std::move(file);
        this->storage_service = std::move(storage_service);
        this->success = success;
        this->failure_cause = std::move(failure_cause);
    }

    /**
    * @brief Constructor
    * @param answer_mailbox: the mailbox to which to send the answer
    * @param file: the file
    * @param src: the source location
    * @param dst: the destination location
    * @param file_registry_service: the file registry service to update (nullptr if none)
    * @param payload: the message size in bytes
    *
    * @throw std::invalid_argument
    */
    StorageServiceFileCopyRequestMessage::StorageServiceFileCopyRequestMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                                                               std::shared_ptr<DataFile> file,
                                                                               std::shared_ptr<FileLocation> src,
                                                                               std::shared_ptr<FileLocation> dst,
                                                                               std::shared_ptr<FileRegistryService> file_registry_service,
                                                                               double payload) : StorageServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((answer_mailbox == nullptr) || (file == nullptr) || (src == nullptr) || (dst == nullptr)) {
            throw std::invalid_argument(
                    "StorageServiceFileCopyRequestMessage::StorageServiceFileCopyRequestMessage(): Invalid arguments");
        }
#endif
        this->answer_mailbox = answer_mailbox;
        this->file = std::move(file);
        this->src = std::move(src);
        this->dst = std::move(dst);
        this->file_registry_service = std::move(file_registry_service);
    }

    /**
     * @brief Constructor
     * @param file: the file
     * @param src: the source location
     * @param dst: the destination location
     * @param file_registry_service: the file registry service to update (nullptr if none)
     * @param file_registry_service_updated: whether the file registry service was updated
     * @param success: true on success, false otherwise
     * @param failure_cause: the cause of a failure (nullptr if success==true)
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    StorageServiceFileCopyAnswerMessage::StorageServiceFileCopyAnswerMessage(std::shared_ptr<DataFile> file,
                                                                             std::shared_ptr<FileLocation> src,
                                                                             std::shared_ptr<FileLocation> dst,
                                                                             std::shared_ptr<FileRegistryService> file_registry_service,
                                                                             bool file_registry_service_updated,
                                                                             bool success,
                                                                             std::shared_ptr<FailureCause> failure_cause,
                                                                             double payload)
        : StorageServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((file == nullptr) || (src == nullptr) || (dst == nullptr) ||
            (success && (failure_cause != nullptr)) ||
            (!success && (failure_cause == nullptr)) ||
            ((file_registry_service == nullptr) and (file_registry_service_updated))) {
            throw std::invalid_argument(
                    "StorageServiceFileCopyAnswerMessage::StorageServiceFileCopyAnswerMessage(): Invalid arguments");
        }
#endif
        this->file = std::move(file);
        this->src = std::move(src);
        this->dst = std::move(dst);
        this->file_registry_service = file_registry_service;
        this->file_registry_service_updated = file_registry_service_updated;
        this->success = success;
        this->failure_cause = std::move(failure_cause);
        this->file_registry_service = file_registry_service;
    }

    /**
    * @brief Constructor
    * @param answer_mailbox: the mailbox to which to send the answer
    * @param file: the file
    * @param location: the file location
    * @param buffer_size: the buffer size
    * @param payload: the message size in bytes
    *
    * @throw std::invalid_argument
    */
    StorageServiceFileWriteRequestMessage::StorageServiceFileWriteRequestMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                                                                 std::shared_ptr<DataFile> file,
                                                                                 std::shared_ptr<FileLocation> location,
                                                                                 unsigned long buffer_size,
                                                                                 double payload)
        : StorageServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((answer_mailbox == nullptr) || (file == nullptr) || (location == nullptr)) {
            throw std::invalid_argument(
                    "StorageServiceFileWriteRequestMessage::StorageServiceFileWriteRequestMessage(): Invalid arguments");
        }
#endif
        this->answer_mailbox = answer_mailbox;
        this->file = std::move(file);
        this->location = std::move(location);
        this->buffer_size = buffer_size;
    }

    /**
     * @brief Constructor
     * @param file: the file
     * @param location: the file's location
     * @param success: whether the write operation succeeded
     * @param failure_cause: the cause of the failure (nullptr if success)
     * @param data_write_mailbox: the mailbox to which file content should be sent
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    StorageServiceFileWriteAnswerMessage::StorageServiceFileWriteAnswerMessage(std::shared_ptr<DataFile> file,
                                                                               std::shared_ptr<FileLocation> location,
                                                                               bool success,
                                                                               std::shared_ptr<FailureCause> failure_cause,
                                                                               simgrid::s4u::Mailbox *data_write_mailbox,
                                                                               double payload) : StorageServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((file == nullptr) || (location == nullptr) ||
            (success && (data_write_mailbox == nullptr)) ||
            (!success && (data_write_mailbox != nullptr)) ||
            (success && (failure_cause != nullptr)) || (!success && (failure_cause == nullptr))) {
            throw std::invalid_argument(
                    "StorageServiceFileWriteAnswerMessage::StorageServiceFileWriteAnswerMessage(): Invalid arguments");
        }
#endif
        this->file = std::move(file);
        this->location = std::move(location);
        this->success = success;
        this->failure_cause = std::move(failure_cause);
        this->data_write_mailbox = data_write_mailbox;
    }

    /**
   * @brief Constructor
   * @param answer_mailbox: the mailbox to which to send the answer
   * @param mailbox_to_receive_the_file_content: the mailbox to which to send the file content
   * @param file: the file
   * @param location: the location where the file is stored
   * @param num_bytes_to_read: the number of bytes to read
   * @param payload: the message size in bytes
   *
   * @throw std::invalid_argument
   */
    StorageServiceFileReadRequestMessage::StorageServiceFileReadRequestMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                                                               simgrid::s4u::Mailbox *mailbox_to_receive_the_file_content,
                                                                               std::shared_ptr<DataFile> file,
                                                                               std::shared_ptr<FileLocation> location,
                                                                               double num_bytes_to_read,
                                                                               double payload) : StorageServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS

        if ((answer_mailbox == nullptr) || (mailbox_to_receive_the_file_content == nullptr) ||
            (file == nullptr) || (location == nullptr) || (num_bytes_to_read == -1)) {
            throw std::invalid_argument(
                    "StorageServiceFileReadRequestMessage::StorageServiceFileReadRequestMessage(): Invalid arguments");
        }
#endif
        this->answer_mailbox = answer_mailbox;
        this->mailbox_to_receive_the_file_content = mailbox_to_receive_the_file_content;
        this->file = std::move(file);
        this->location = std::move(location);
        this->num_bytes_to_read = num_bytes_to_read;
    }
    /**
    * @brief Constructor
    * @param other: packet to copy
    *
    * @throw std::invalid_argument
    */
    StorageServiceFileReadRequestMessage::StorageServiceFileReadRequestMessage(StorageServiceFileReadRequestMessage &other) : StorageServiceFileReadRequestMessage(&other) {
    }
    /**
    * @brief Constructor
    * @param other: packet to copy
    *
    * @throw std::invalid_argument
    */
    StorageServiceFileReadRequestMessage::StorageServiceFileReadRequestMessage(StorageServiceFileReadRequestMessage *other) : StorageServiceMessage(other->payload), answer_mailbox(other->answer_mailbox), mailbox_to_receive_the_file_content(other->mailbox_to_receive_the_file_content), file(other->file), location(other->location), num_bytes_to_read(other->num_bytes_to_read) {
    }
    /**
     * @brief Constructor
     * @param file: the file
     * @param location: the location of the file trrex
     * @param success: whether the read operation was successful
     * @param failure_cause: the cause of the failure (or nullptr on success)
   * @param buffer_size: the buffer size that will be used
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    StorageServiceFileReadAnswerMessage::StorageServiceFileReadAnswerMessage(std::shared_ptr<DataFile> file,
                                                                             std::shared_ptr<FileLocation> location,
                                                                             bool success,
                                                                             std::shared_ptr<FailureCause> failure_cause,
                                                                             unsigned long buffer_size,
                                                                             double payload) : StorageServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((file == nullptr) || (location == nullptr) ||
            (success && (failure_cause != nullptr)) || (!success && (failure_cause == nullptr))) {
            throw std::invalid_argument(
                    "StorageServiceFileReadAnswerMessage::StorageServiceFileReadAnswerMessage(): Invalid arguments");
        }
#endif
        this->file = std::move(file);
        this->location = std::move(location);
        this->success = success;
        this->buffer_size = buffer_size;

        this->failure_cause = std::move(failure_cause);
    }

    /**
    * @brief Constructor
    * @param file: the workflow data file to which this chunk belongs
    * @param chunk_size: the chunk size
    * @param last_chunk: whether this is the last chunk in the file
    */
    StorageServiceFileContentChunkMessage::StorageServiceFileContentChunkMessage(
            std::shared_ptr<DataFile> file, unsigned long chunk_size, bool last_chunk) : StorageServiceMessage((double) chunk_size) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if (file == nullptr) {
            throw std::invalid_argument(
                    "StorageServiceFileContentChunkMessage::StorageServiceFileContentChunkMessage(): Invalid arguments");
        }
#endif
        this->file = std::move(file);
        this->last_chunk = last_chunk;
    }

};// namespace wrench
