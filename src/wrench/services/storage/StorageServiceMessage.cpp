/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include "StorageServiceMessage.h"
#include <wrench/workflow/WorkflowFile.h>

namespace wrench {

    /**
     * @brief Constructor
     * @param name: the message name
     * @param payload: the message size in bytes
     */
    StorageServiceMessage::StorageServiceMessage(std::string name, double payload) :
            ServiceMessage("StorageService::" + name, payload) {

    }

    /**
    * @brief Constructor
    * @param answer_mailbox: the mailbox to which to send the answer
    * @param payload: the message size in bytes
    *
    * @throw std::invalid_argument
    */
    StorageServiceFreeSpaceRequestMessage::StorageServiceFreeSpaceRequestMessage(std::string answer_mailbox,
                                                                                 double payload)
            : StorageServiceMessage("FREE_SPACE_REQUEST", payload) {
      if ((answer_mailbox == "")) {
        throw std::invalid_argument("StorageServiceFreeSpaceRequestMessage::StorageServiceFreeSpaceRequestMessage(): Invalid arguments");
      }
      this->answer_mailbox = answer_mailbox;
    }

    /**
     * @brief Constructor
     * @param free_space: the free space, in bytes
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    StorageServiceFreeSpaceAnswerMessage::StorageServiceFreeSpaceAnswerMessage(double free_space, double payload)
            : StorageServiceMessage(
            "FREE_SPACE_ANSWER", payload) {
      if ((free_space < 0.0)) {
        throw std::invalid_argument("StorageServiceFreeSpaceAnswerMessage::StorageServiceFreeSpaceAnswerMessage(): Invalid arguments");
      }
      this->free_space = free_space;
    }

    /**
    * @brief Constructor
    * @param answer_mailbox: the mailbox to which to send the answer
    * @param file: the file
    * @param dst_dir: the file directory to look up the file for
    * @param payload: the message size in bytes
    *
    * @throw std::invalid_argument
    */
    StorageServiceFileLookupRequestMessage::StorageServiceFileLookupRequestMessage(std::string answer_mailbox,
                                                                                   WorkflowFile *file,
                                                                                   std::string& dst_dir,
                                                                                   double payload)
            : StorageServiceMessage("FILE_LOOKUP_REQUEST",
                                    payload) {
      if ((file == nullptr) || (answer_mailbox == "")) {
        throw std::invalid_argument("StorageServiceFileLookupRequestMessage::StorageServiceFileLookupRequestMessage(): Invalid arguments");
      }
      this->answer_mailbox = answer_mailbox;
      this->file = file;
      this->dst_dir = dst_dir;
    }

    /**
     * @brief Constructor
     * @param file: the file
     * @param file_is_available: true if the file is available on the storage system
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    StorageServiceFileLookupAnswerMessage::StorageServiceFileLookupAnswerMessage(WorkflowFile *file,
                                                                                 bool file_is_available,
                                                                                 double payload)
            : StorageServiceMessage(
            "FILE_LOOKUP_ANSWER", payload) {

      if (file == nullptr) {
        throw std::invalid_argument("StorageServiceFileLookupAnswerMessage::StorageServiceFileLookupAnswerMessage(): Invalid arguments");
      }
      this->file = file;
      this->file_is_available = file_is_available;
    }

    /**
     * @brief Constructor
     * @param answer_mailbox: the mailbox to which to send the answer
     * @param file: the file
     * @param dst_dir: the file directory from where the file will be deleted
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    StorageServiceFileDeleteRequestMessage::StorageServiceFileDeleteRequestMessage(std::string answer_mailbox,
                                                                                   WorkflowFile *file,
                                                                                   std::string& dst_dir,
                                                                                   double payload)
            : StorageServiceMessage("FILE_DELETE_REQUEST",
                                    payload) {
      if ((answer_mailbox == "") || (file == nullptr)) {
        throw std::invalid_argument("StorageServiceFileDeleteRequestMessage::StorageServiceFileDeleteRequestMessage(): Invalid arguments");
      }
      this->file = file;
      this->answer_mailbox = answer_mailbox;
      this->dst_dir = dst_dir;
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
    StorageServiceFileDeleteAnswerMessage::StorageServiceFileDeleteAnswerMessage(WorkflowFile *file,
                                                                                 StorageService *storage_service,
                                                                                 bool success,
                                                                                 std::shared_ptr<FailureCause> failure_cause,
                                                                                 double payload)
            : StorageServiceMessage("FILE_DELETE_ANSWER", payload) {

      if ((file == nullptr) || (storage_service == nullptr) ||
              (success && (failure_cause != nullptr)) ||
              ((!success) && (failure_cause == nullptr))) {
        throw std::invalid_argument("StorageServiceFileDeleteAnswerMessage::StorageServiceFileDeleteAnswerMessage(): Invalid arguments");
      }
      this->file = file;
      this->storage_service = storage_service;
      this->success = success;
      this->failure_cause = std::move(failure_cause);
    }

    /**
    * @brief Constructor
    * @param answer_mailbox: the mailbox to which to send the answer
    * @param file: the file
    * @param src: the source storage service
    * @param src_dir: the file directory from where the file will be copied
    * @param dst_dir: the file directory where the file will be stored
    * @param file_registry_service: the file registry service to update (nullptr if none)
    * @param payload: the message size in bytes
    *
    * @throw std::invalid_argument
    */
    StorageServiceFileCopyRequestMessage::StorageServiceFileCopyRequestMessage(std::string answer_mailbox,
                                                                               WorkflowFile *file,
                                                                               StorageService *src,
                                                                               std::string& src_dir,
                                                                               std::string& dst_dir,
                                                                               FileRegistryService *file_registry_service,
                                                                               double payload) : StorageServiceMessage(
            "FILE_COPY_REQUEST", payload) {
      if ((answer_mailbox == "") || (file == nullptr) || (src == nullptr)) {
        throw std::invalid_argument("StorageServiceFileCopyRequestMessage::StorageServiceFileCopyRequestMessage(): Invalid arguments");
      }
      this->answer_mailbox = answer_mailbox;
      this->file = file;
      this->src = src;
      this->file_registry_service = file_registry_service;
      this->src_dir = src_dir;
      this->dst_dir = dst_dir;
    }

    /**
     * @brief Constructor
     * @param file: the file
     * @param storage_service: the storage service
     * @param file_registry_service: the file registry service to update (nullptr if none)
     * @param file_registry_service_updated: whether the file registry service was updated
     * @param success: true on success, false otherwise
     * @param failure_cause: the cause of a failure (nullptr if success==true)
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    StorageServiceFileCopyAnswerMessage::StorageServiceFileCopyAnswerMessage(WorkflowFile *file,
                                                                             StorageService *storage_service,
                                                                             FileRegistryService *file_registry_service,
                                                                             bool file_registry_service_updated,
                                                                             bool success,
                                                                             std::shared_ptr<FailureCause> failure_cause,
                                                                             double payload)
            : StorageServiceMessage("FILE_COPY_ANSWER", payload) {
      if ((file == nullptr) || (storage_service == nullptr) ||
              (success && (failure_cause != nullptr)) ||
              (!success && (failure_cause == nullptr)) ||
              ((file_registry_service == nullptr) and (file_registry_service_updated))) {
        throw std::invalid_argument("StorageServiceFileCopyAnswerMessage::StorageServiceFileCopyAnswerMessage(): Invalid arguments");
      }
      this->file = file;
      this->storage_service = storage_service;
      this->file_registry_service = file_registry_service;
      this->file_registry_service_updated = file_registry_service_updated;
      this->success = success;
      this->failure_cause = failure_cause;
      this->file_registry_service = file_registry_service;
    }

    /**
    * @brief Constructor
    * @param answer_mailbox: the mailbox to which to send the answer
    * @param file: the file
    * @param payload: the message size in bytes
    *
    * @throw std::invalid_argument
    */
    StorageServiceFileWriteRequestMessage::StorageServiceFileWriteRequestMessage(std::string answer_mailbox,
                                                                                 WorkflowFile *file,
                                                                                 std::string& dst_dir,
                                                                                 double payload)
            : StorageServiceMessage("FILE_WRITE_REQUEST",
                                    payload) {
      if ((answer_mailbox == "") || (file == nullptr)) {
        throw std::invalid_argument("StorageServiceFileWriteRequestMessage::StorageServiceFileWriteRequestMessage(): Invalid arguments");
      }
      this->payload += file->getSize();
      this->answer_mailbox = answer_mailbox;
      this->file = file;
      this->dst_dir = dst_dir;
    }

    /**
     * @brief Constructor
     * @param file: the file
     * @param storage_service: the storage service
     * @param success: whether the write operation succeeded
     * @param failure_cause: the cause of the failure (nullptr if success)
     * @param data_write_mailbox_name: the mailbox to which file content should be sent
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    StorageServiceFileWriteAnswerMessage::StorageServiceFileWriteAnswerMessage(WorkflowFile *file,
                                                                               StorageService *storage_service,
                                                                               bool success,
                                                                               std::shared_ptr<FailureCause> failure_cause,
                                                                               std::string data_write_mailbox_name,
                                                                               double payload) : StorageServiceMessage(
            "FILE_WRITE_ANSWER", payload) {
      if ((file == nullptr) || (storage_service == nullptr) || (data_write_mailbox_name == "") ||
              (success && (failure_cause != nullptr)) || (!success && (failure_cause == nullptr))) {
        throw std::invalid_argument("StorageServiceFileWriteAnswerMessage::StorageServiceFileWriteAnswerMessage(): Invalid arguments");
      }
      this->file = file;
      this->storage_service = storage_service;
      this->success = success;
      this->failure_cause = failure_cause;
      this->data_write_mailbox_name = data_write_mailbox_name;
    }

    /**
   * @brief Constructor
   * @param answer_mailbox: the mailbox to which to send the answer
   * @param mailbox_to_receive_the_file_content: the mailbox to which to send the file content
   * @param file: the file
   * @param payload: the message size in bytes
   *
   * @throw std::invalid_argument
   */
    StorageServiceFileReadRequestMessage::StorageServiceFileReadRequestMessage(std::string answer_mailbox,
                                                                               std::string mailbox_to_receive_the_file_content,
                                                                               WorkflowFile *file,
                                                                               std::string& src_dir,
                                                                               double payload) : StorageServiceMessage(
            "FILE_READ_REQUEST",
            payload) {
      if ((answer_mailbox == "") || (mailbox_to_receive_the_file_content == "") || (file == nullptr)) {
        throw std::invalid_argument("StorageServiceFileReadRequestMessage::StorageServiceFileReadRequestMessage(): Invalid arguments");
      }
      this->answer_mailbox = answer_mailbox;
      this->mailbox_to_receive_the_file_content = mailbox_to_receive_the_file_content;
      this->file = file;
      this->src_dir = src_dir;
    }

    /**
     * @brief Constructor
     * @param file: the file
     * @param storage_service: the storage service
     * @param success: whether the read operation was successful
     * @param failure_cause: the cause of the failure (or nullptr on success)
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    StorageServiceFileReadAnswerMessage::StorageServiceFileReadAnswerMessage(WorkflowFile *file,
                                                                             StorageService *storage_service,
                                                                             bool success,
                                                                             std::shared_ptr<FailureCause> failure_cause,
                                                                             double payload) : StorageServiceMessage(
            "FILE_READ_ANSWER",
            payload) {
      if ((file == nullptr) || (storage_service == nullptr) ||
              (success && (failure_cause != nullptr)) || (!success && (failure_cause == nullptr))) {
        throw std::invalid_argument("StorageServiceFileReadAnswerMessage::StorageServiceFileReadAnswerMessage(): Invalid arguments");
      }
      this->file = file;
      this->storage_service = storage_service;
      this->success = success;
      this->failure_cause = failure_cause;
    }

    /**
    * @brief Constructor
    * @param file: the workflow data file
    */
    StorageServiceFileContentMessage::StorageServiceFileContentMessage(WorkflowFile *file) : StorageServiceMessage(
            "FILE_CONTENT", 0) {
      if (file == nullptr) {
        throw std::invalid_argument("StorageServiceFileContentMessage::StorageServiceFileContentMessage(): Invalid arguments");
      }
      this->payload += file->getSize();
      this->file = file;
    }

};
