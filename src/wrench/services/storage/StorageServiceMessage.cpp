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
    * @param path: the path
    * @param payload: the message size in bytes
    *
    * @throw std::invalid_argument
    */
    StorageServiceFreeSpaceRequestMessage::StorageServiceFreeSpaceRequestMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                                                                 const std::string &path,
                                                                                 double payload)
        : StorageServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if (answer_mailbox == nullptr) {
            throw std::invalid_argument(
                    "StorageServiceFreeSpaceRequestMessage::StorageServiceFreeSpaceRequestMessage(): Invalid arguments");
        }
#endif
        this->answer_mailbox = answer_mailbox;
        if (path.empty()) {
            this->path = "/";
        } else {
            this->path = path;
        }
    }

    /**
     * @brief Constructor
     * @param free_space: the free space, in bytes, at each mount point, as a map
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    StorageServiceFreeSpaceAnswerMessage::StorageServiceFreeSpaceAnswerMessage(
            double free_space, double payload)
        : StorageServiceMessage(payload) {
        this->free_space = free_space;
    }

    /**
    * @brief Constructor
    * @param answer_mailbox: the mailbox to which to send the answer
    * @param location: the file location (hopefully)
    * @param payload: the message size in bytes
    *
    * @throw std::invalid_argument
    */
    StorageServiceFileLookupRequestMessage::StorageServiceFileLookupRequestMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                                                                   const std::shared_ptr<FileLocation> &location,
                                                                                   double payload)
        : StorageServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((answer_mailbox == nullptr) || (location == nullptr)) {
            throw std::invalid_argument(
                    "StorageServiceFileLookupRequestMessage::StorageServiceFileLookupRequestMessage(): Invalid nullptr arguments");
        }
#endif
        this->answer_mailbox = answer_mailbox;
        this->location = location;
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
     * @param location: the location
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    StorageServiceFileDeleteRequestMessage::StorageServiceFileDeleteRequestMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                                                                   const std::shared_ptr<FileLocation> &location,
                                                                                   double payload)
        : StorageServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((answer_mailbox == nullptr) || (location == nullptr)) {
            throw std::invalid_argument(
                    "StorageServiceFileDeleteRequestMessage::StorageServiceFileDeleteRequestMessage(): Invalid arguments");
        }
#endif
        this->answer_mailbox = answer_mailbox;
        this->location = location;
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
    * @param answer_mailbox: the mailbox to which to send the answer (if nullptr, no answer will be sent)
    * @param src: the source location
    * @param dst: the destination location
    * @param payload: the message size in bytes
    *
    * @throw std::invalid_argument
    */
    StorageServiceFileCopyRequestMessage::StorageServiceFileCopyRequestMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                                                               std::shared_ptr<FileLocation> src,
                                                                               std::shared_ptr<FileLocation> dst,
                                                                               double payload) : StorageServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((answer_mailbox == nullpr) || (src == nullptr) || (dst == nullptr)) {
            throw std::invalid_argument(
                    "StorageServiceFileCopyRequestMessage::StorageServiceFileCopyRequestMessage(): Invalid arguments");
        }
#endif
        this->answer_mailbox = answer_mailbox;
        this->src = std::move(src);
        this->dst = std::move(dst);
    }

    /**
     * @brief Constructor
     * @param src: the source location
     * @param dst: the destination location
     * @param success: true on success, false otherwise
     * @param failure_cause: the cause of a failure (nullptr if success==true)
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    StorageServiceFileCopyAnswerMessage::StorageServiceFileCopyAnswerMessage(std::shared_ptr<FileLocation> src,
                                                                             std::shared_ptr<FileLocation> dst,
                                                                             bool success,
                                                                             std::shared_ptr<FailureCause> failure_cause,
                                                                             double payload)
        : StorageServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((src == nullptr) || (dst == nullptr) ||
            (success && (failure_cause != nullptr)) ||
            (!success && (failure_cause == nullptr)) {
            throw std::invalid_argument(
                    "StorageServiceFileCopyAnswerMessage::StorageServiceFileCopyAnswerMessage(): Invalid arguments");
        }
#endif
        this->src = std::move(src);
        this->dst = std::move(dst);
        this->success = success;
        this->failure_cause = std::move(failure_cause);
    }

    /**
    * @brief Constructor
    * @param answer_mailbox: the mailbox to which to send the answer
    * @param requesting_host: the requesting host
    * @param location: the location
    * @param payload: the message size in bytes
    *
    * @throw std::invalid_argument
    */
    StorageServiceFileWriteRequestMessage::StorageServiceFileWriteRequestMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                                                                 simgrid::s4u::Host *requesting_host,
                                                                                 const std::shared_ptr<FileLocation> &location,
                                                                                 double payload)
        : StorageServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((!answer_mailbox) or (!requesting_host) or (!location)) {
            throw std::invalid_argument(
                    "StorageServiceFileWriteRequestMessage::StorageServiceFileWriteRequestMessage(): Invalid nullptr arguments");
        }
#endif
        this->answer_mailbox = answer_mailbox;
        this->requesting_host = requesting_host;
        this->location = location;
    }

    /**
     * @brief Constructor
     * @param location: the file's location
     * @param success: whether the write operation succeeded
     * @param failure_cause: the cause of the failure (nullptr if success)
     * @param data_write_mailboxes_and_bytes: mailboxes to which bytes need to be sent
     * @param buffer_size: the buffer size to use
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    StorageServiceFileWriteAnswerMessage::StorageServiceFileWriteAnswerMessage(std::shared_ptr<FileLocation> &location,
                                                                               bool success,
                                                                               std::shared_ptr<FailureCause> failure_cause,
                                                                               std::map<simgrid::s4u::Mailbox *, double> data_write_mailboxes_and_bytes,
                                                                               double buffer_size,
                                                                               double payload) : StorageServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((location == nullptr) ||
            (success && (data_write_mailbox == nullptr)) ||
            (success && data_write__mailboxes_and_bytes.empty()) ||
            (success && (failure_cause != nullptr)) || (!success && (failure_cause == nullptr))) {
            throw std::invalid_argument(
                    "StorageServiceFileWriteAnswerMessage::StorageServiceFileWriteAnswerMessage(): Invalid arguments");
        }
#endif
        this->location = location;
        this->success = success;
        this->failure_cause = std::move(failure_cause);
        this->data_write_mailboxes_and_bytes = data_write_mailboxes_and_bytes;
        this->buffer_size = buffer_size;
    }

    /**
   * @brief Constructor
   * @param answer_mailbox: the mailbox to which to send the answer
   * @param requesting_host: the requesting host
   * @param location: the location to read
   * @param num_bytes_to_read: the number of bytes to read
   * @param payload: the message size in bytes
   *
   * @throw std::invalid_argument
   */

    StorageServiceFileReadRequestMessage::StorageServiceFileReadRequestMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                                                               simgrid::s4u::Host *requesting_host,
                                                                               std::shared_ptr<FileLocation> location,
                                                                               double num_bytes_to_read,
                                                                               double payload) : StorageServiceMessage(payload) {

#ifdef WRENCH_INTERNAL_EXCEPTIONS

        if (!answer_mailbox || !requesting_host || !file || (num_bytes_to_read < 0.0)) ||
            (location == nullptr) || (num_bytes_to_read == -1)) {
                throw std::invalid_argument(
                        "StorageServiceFileReadRequestMessage::StorageServiceFileReadRequestMessage(): Invalid nullptr/0 arguments");
            }
#endif
        this->answer_mailbox = answer_mailbox;
        this->requesting_host = requesting_host;
        this->location = location;
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
    StorageServiceFileReadRequestMessage::StorageServiceFileReadRequestMessage(StorageServiceFileReadRequestMessage *other) : StorageServiceMessage(other->payload), answer_mailbox(other->answer_mailbox), location(other->location), num_bytes_to_read(other->num_bytes_to_read) {
    }

    /**
     * @brief Constructor
     * @param location: the location of the file to read
     * @param success: whether the read operation was successful
     * @param failure_cause: the cause of the failure (or nullptr on success)
     * @param mailbox_to_receive_the_file_content: the mailbox to which to send the file content (or nullptr if none)
     * @param buffer_size: the buffer size that will be used
     * @param number_of_sources: the number of sources that will send file chunks over
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    StorageServiceFileReadAnswerMessage::StorageServiceFileReadAnswerMessage(std::shared_ptr<FileLocation> location,
                                                                             bool success,
                                                                             std::shared_ptr<FailureCause> failure_cause,
                                                                             simgrid::s4u::Mailbox *mailbox_to_receive_the_file_content,
                                                                             double buffer_size,
                                                                             unsigned long number_of_sources,
                                                                             double payload) : StorageServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((location == nullptr) ||
            (success && (failure_cause != nullptr)) || (!success && (failure_cause == nullptr))) {
            throw std::invalid_argument(
                    "StorageServiceFileReadAnswerMessage::StorageServiceFileReadAnswerMessage(): Invalid arguments");
        }
#endif
        this->location = std::move(location);
        this->success = success;
        this->mailbox_to_receive_the_file_content = mailbox_to_receive_the_file_content;
        this->buffer_size = buffer_size;
        this->number_of_sources = number_of_sources;
        this->failure_cause = std::move(failure_cause);
    }

    /**
    * @brief Constructor
    * @param file: the workflow data file to which this chunk belongs
    * @param chunk_size: the chunk size
    * @param last_chunk: whether this is the last chunk in the file
    */
    StorageServiceFileContentChunkMessage::StorageServiceFileContentChunkMessage(
            std::shared_ptr<DataFile> file, double chunk_size, bool last_chunk) : StorageServiceMessage(chunk_size) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if (file == nullptr) {
            throw std::invalid_argument(
                    "StorageServiceFileContentChunkMessage::StorageServiceFileContentChunkMessage(): Invalid arguments");
        }
#endif
        this->file = std::move(file);
        this->last_chunk = last_chunk;
    }

}// namespace wrench
