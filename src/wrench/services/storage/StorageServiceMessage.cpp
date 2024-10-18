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
#include <wrench/services/compute/cloud/CloudComputeService.h>
#include <wrench/services/storage/StorageService.h>

#include <utility>

namespace wrench {

    /**
     * @brief Constructor
     * @param payload: the message size in bytes
     */
    StorageServiceMessage::StorageServiceMessage(sg_size_t payload) : ServiceMessage(payload) {
    }

    /**
    * @brief Constructor
    * @param answer_commport: the commport to which to send the answer
    * @param path: the path
    * @param payload: the message size in bytes
    *
    */
    StorageServiceFreeSpaceRequestMessage::StorageServiceFreeSpaceRequestMessage(S4U_CommPort *answer_commport,
                                                                                 const std::string &path,
                                                                                 sg_size_t payload)
        : StorageServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if (answer_commport == nullptr) {
            throw std::invalid_argument(
                    "StorageServiceFreeSpaceRequestMessage::StorageServiceFreeSpaceRequestMessage(): Invalid arguments");
        }
#endif
        this->answer_commport = answer_commport;
//        if (path.empty()) {
//            this->path = "/";
//        } else {
            this->path = path;
//        }
    }

    /**
     * @brief Constructor
     * @param free_space: the free space, in bytes, at each mount point, as a map
     * @param payload: the message size in bytes
     *
     */
    StorageServiceFreeSpaceAnswerMessage::StorageServiceFreeSpaceAnswerMessage(
            sg_size_t free_space, sg_size_t payload)
        : StorageServiceMessage(payload) {
        this->free_space = free_space;
    }

    /**
    * @brief Constructor
    * @param answer_commport: the commport to which to send the answer
    * @param location: the file location (hopefully)
    * @param payload: the message size in bytes
    */
    StorageServiceFileLookupRequestMessage::StorageServiceFileLookupRequestMessage(S4U_CommPort *answer_commport,
                                                                                   const std::shared_ptr<FileLocation> &location,
                                                                                   sg_size_t payload)
        : StorageServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((answer_commport == nullptr) || (location == nullptr)) {
            throw std::invalid_argument(
                    "StorageServiceFileLookupRequestMessage::StorageServiceFileLookupRequestMessage(): Invalid nullptr arguments");
        }
#endif
        this->answer_commport = answer_commport;
        this->location = location;
    }

    /**
     * @brief Constructor
     * @param file: the file
     * @param file_is_available: true if the file is available on the storage system
     * @param payload: the message size in bytes
     *
     */
    StorageServiceFileLookupAnswerMessage::StorageServiceFileLookupAnswerMessage(std::shared_ptr<DataFile> file,
                                                                                 bool file_is_available,
                                                                                 sg_size_t payload)
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
     * @param answer_commport: the commport to which to send the answer
     * @param location: the location
     * @param payload: the message size in bytes
     *
     */
    StorageServiceFileDeleteRequestMessage::StorageServiceFileDeleteRequestMessage(S4U_CommPort *answer_commport,
                                                                                   const std::shared_ptr<FileLocation> &location,
                                                                                   sg_size_t payload)
        : StorageServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((answer_commport == nullptr) || (location == nullptr)) {
            throw std::invalid_argument(
                    "StorageServiceFileDeleteRequestMessage::StorageServiceFileDeleteRequestMessage(): Invalid arguments");
        }
#endif
        this->answer_commport = answer_commport;
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
     */
    StorageServiceFileDeleteAnswerMessage::StorageServiceFileDeleteAnswerMessage(std::shared_ptr<DataFile> file,
                                                                                 std::shared_ptr<StorageService> storage_service,
                                                                                 bool success,
                                                                                 std::shared_ptr<FailureCause> failure_cause,
                                                                                 sg_size_t payload)
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
    * @param answer_commport: the commport to which to send the answer (if nullptr, no answer will be sent)
    * @param src: the source location
    * @param dst: the destination location
    * @param payload: the message size in bytes
    *
    */
    StorageServiceFileCopyRequestMessage::StorageServiceFileCopyRequestMessage(S4U_CommPort *answer_commport,
                                                                               std::shared_ptr<FileLocation> src,
                                                                               std::shared_ptr<FileLocation> dst,
                                                                               sg_size_t payload) : StorageServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((answer_commport == nullpr) || (src == nullptr) || (dst == nullptr)) {
            throw std::invalid_argument(
                    "StorageServiceFileCopyRequestMessage::StorageServiceFileCopyRequestMessage(): Invalid arguments");
        }
#endif
        this->answer_commport = answer_commport;
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
     */
    StorageServiceFileCopyAnswerMessage::StorageServiceFileCopyAnswerMessage(std::shared_ptr<FileLocation> src,
                                                                             std::shared_ptr<FileLocation> dst,
                                                                             bool success,
                                                                             std::shared_ptr<FailureCause> failure_cause,
                                                                             sg_size_t payload)
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
    * @param answer_commport: the commport to which to send the answer
    * @param requesting_host: the requesting host
    * @param location: the location
    * @param num_bytes_to_write: the number of bytes to write to the file
    * @param payload: the message size in bytes
    *
    */
    StorageServiceFileWriteRequestMessage::StorageServiceFileWriteRequestMessage(S4U_CommPort *answer_commport,
                                                                                 simgrid::s4u::Host *requesting_host,
                                                                                 const std::shared_ptr<FileLocation> &location,
                                                                                 sg_size_t num_bytes_to_write,
                                                                                 sg_size_t payload)
        : StorageServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((!answer_commport) or (!requesting_host) or (!location)) {
            throw std::invalid_argument(
                    "StorageServiceFileWriteRequestMessage::StorageServiceFileWriteRequestMessage(): Invalid nullptr arguments");
        }
#endif
        this->answer_commport = answer_commport;
        this->requesting_host = requesting_host;
        this->location = location;
        this->num_bytes_to_write = num_bytes_to_write;
    }

    /**
     * @brief Constructor
     * @param location: the file's location
     * @param success: whether the write operation succeeded
     * @param failure_cause: the cause of the failure (nullptr if success)
     * @param data_write_commports_and_bytes: commports to which bytes need to be sent
     * @param buffer_size: the buffer size to use
     * @param payload: the message size in bytes
     *
     */
    StorageServiceFileWriteAnswerMessage::StorageServiceFileWriteAnswerMessage(std::shared_ptr<FileLocation> &location,
                                                                               bool success,
                                                                               std::shared_ptr<FailureCause> failure_cause,
                                                                               std::map<S4U_CommPort *, sg_size_t> data_write_commports_and_bytes,
                                                                               sg_size_t buffer_size,
                                                                               sg_size_t payload) : StorageServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((location == nullptr) ||
            (success && (data_write_commport == nullptr)) ||
            (success && data_write__commports_and_bytes.empty()) ||
            (success && (failure_cause != nullptr)) || (!success && (failure_cause == nullptr))) {
            throw std::invalid_argument(
                    "StorageServiceFileWriteAnswerMessage::StorageServiceFileWriteAnswerMessage(): Invalid arguments");
        }
#endif
        this->location = location;
        this->success = success;
        this->failure_cause = std::move(failure_cause);
        this->data_write_commport_and_bytes = std::move(data_write_commports_and_bytes);
        this->buffer_size = buffer_size;
    }

    /**
   * @brief Constructor
   * @param answer_commport: the commport to which to send the answer
   * @param requesting_host: the requesting host
   * @param location: the location to read
   * @param num_bytes_to_read: the number of bytes to read
   * @param payload: the message size in bytes
   *
   */

    StorageServiceFileReadRequestMessage::StorageServiceFileReadRequestMessage(S4U_CommPort *answer_commport,
                                                                               simgrid::s4u::Host *requesting_host,
                                                                               std::shared_ptr<FileLocation> location,
                                                                               sg_size_t num_bytes_to_read,
                                                                               sg_size_t payload) : StorageServiceMessage(payload) {

#ifdef WRENCH_INTERNAL_EXCEPTIONS

        if (!answer_commport || !requesting_host || !file || (num_bytes_to_read < 0.0)) ||
            (location == nullptr) || (num_bytes_to_read == -1)) {
                throw std::invalid_argument(
                        "StorageServiceFileReadRequestMessage::StorageServiceFileReadRequestMessage(): Invalid nullptr/0 arguments");
            }
#endif
        this->answer_commport = answer_commport;
        this->requesting_host = requesting_host;
        this->location = std::move(location);
        this->num_bytes_to_read = num_bytes_to_read;
    }

    /**
    * @brief Constructor
    * @param other: packet to copy
    *
    */
    StorageServiceFileReadRequestMessage::StorageServiceFileReadRequestMessage(StorageServiceFileReadRequestMessage &other) : StorageServiceFileReadRequestMessage(&other) {
    }

    /**
    * @brief Constructor
    * @param other: packet to copy
    *
    */
    StorageServiceFileReadRequestMessage::StorageServiceFileReadRequestMessage(StorageServiceFileReadRequestMessage *other) : StorageServiceMessage(other->payload), answer_commport(other->answer_commport), location(other->location), num_bytes_to_read(other->num_bytes_to_read) {
    }

    /**
     * @brief Constructor
     * @param location: the location of the file to read
     * @param success: whether the read operation was successful
     * @param failure_cause: the cause of the failure (or nullptr on success)
     * @param commport_to_receive_the_file_content: the commport to which to send the file content (or nullptr if none)
     * @param buffer_size: the buffer size that will be used
     * @param number_of_sources: the number of sources that will send file chunks over
     * @param payload: the message size in bytes
     *
     */
    StorageServiceFileReadAnswerMessage::StorageServiceFileReadAnswerMessage(std::shared_ptr<FileLocation> location,
                                                                             bool success,
                                                                             std::shared_ptr<FailureCause> failure_cause,
                                                                             S4U_CommPort *commport_to_receive_the_file_content,
                                                                             sg_size_t buffer_size,
                                                                             unsigned long number_of_sources,
                                                                             sg_size_t payload) : StorageServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((location == nullptr) ||
            (success && (failure_cause != nullptr)) || (!success && (failure_cause == nullptr))) {
            throw std::invalid_argument(
                    "StorageServiceFileReadAnswerMessage::StorageServiceFileReadAnswerMessage(): Invalid arguments");
        }
#endif
        this->location = std::move(location);
        this->success = success;
        this->commport_to_receive_the_file_content = commport_to_receive_the_file_content;
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
            std::shared_ptr<DataFile> file, sg_size_t chunk_size, bool last_chunk) : StorageServiceMessage(chunk_size) {
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
