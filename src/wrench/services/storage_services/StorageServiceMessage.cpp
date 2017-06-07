/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include "StorageServiceMessage.h"
#include <workflow/WorkflowFile.h>

namespace wrench {

    StorageServiceMessage::StorageServiceMessage(std::string name, double payload) :
            ServiceMessage("StorageService::" + name, payload) {

    }

    /**
    * @brief Constructor
    * @param answer_mailbox: the mailbox to which to send the answer
    * @param payload: message size in bytes
    *
    * @throw std::invalid_argument
    */
    StorageServiceFreeSpaceRequestMessage::StorageServiceFreeSpaceRequestMessage(std::string answer_mailbox,
                                                                                 double payload)
            : StorageServiceMessage("FREE_SPACE_REQUEST", payload) {
      if ((answer_mailbox == "")) {
        throw std::invalid_argument("Invalid constructor arguments");
      }
      this->answer_mailbox = answer_mailbox;
    }

    /**
     * @brief Constructor
     * @param free_space: the free space, in bytes
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    StorageServiceFreeSpaceAnswerMessage::StorageServiceFreeSpaceAnswerMessage(double free_space, double payload)
            : StorageServiceMessage(
            "FREE_SPACE_ANSWER", payload) {
      if ((free_space < 0.0)) {
        throw std::invalid_argument("Invalid constructor arguments");
      }
      this->free_space = free_space;
    }

    /**
    * @brief Constructor
    * @param answer_mailbox: the mailbox to which to send the answer
    * @param payload: message size in bytes
    *
    * @throw std::invalid_argument
    */
    StorageServiceFileLookupRequestMessage::StorageServiceFileLookupRequestMessage(std::string answer_mailbox,
                                                                                   WorkflowFile *file,
                                                                                   double payload)
            : StorageServiceMessage("FILE_LOOKUP_REQUEST",
                                    payload) {
      if ((file == nullptr) || (answer_mailbox == "")) {
        throw std::invalid_argument("Invalid constructor arguments");
      }
      this->answer_mailbox = answer_mailbox;
      this->file = file;
    }

    /**
     * @brief Constructor
     * @param file_is_available: true if the file is available on the storage system
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    StorageServiceFileLookupAnswerMessage::StorageServiceFileLookupAnswerMessage(WorkflowFile *file,
                                                                                 bool file_is_available,
                                                                                 double payload)
            : StorageServiceMessage(
            "FILE_LOOKUP_ANSWER", payload) {

      this->file = file;
      this->file_is_available = file_is_available;
    }

    /**
     * @brief Constructor
     * @param answer_mailbox: the mailbox to which to send the answer
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    StorageServiceFileDeleteRequestMessage::StorageServiceFileDeleteRequestMessage(std::string answer_mailbox,
                                                                                   WorkflowFile *file,
                                                                                   double payload)
            : StorageServiceMessage("FILE_DELETE_REQUEST",
                                    payload) {
      if ((answer_mailbox == "")) {
        throw std::invalid_argument("Invalid constructor arguments");
      }
      this->file = file;
      this->answer_mailbox = answer_mailbox;
    }

    /**
     * @brief Constructor
     * @param failure_cause: the cause of a failure (nullptr means "no failure")
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    StorageServiceFileDeleteAnswerMessage::StorageServiceFileDeleteAnswerMessage(WorkflowFile *file,
                                                                                 StorageService *storage_service,
                                                                                 bool success,
                                                                                 WorkflowExecutionFailureCause *failure_cause,
                                                                                 double payload)
            : StorageServiceMessage("FILE_DELETE_ANSWER", payload) {

      this->file = file;
      this->storage_service = storage_service;
      this->success = success;
      this->failure_cause = failure_cause;
    }

    /**
    * @brief Constructor
    * @param answer_mailbox: the mailbox to which to send the answer
    * @param file: the file
    * @param src: the source storage service
    * @param payload: message size in bytes
    *
    * @throw std::invalid_argument
    */
    StorageServiceFileCopyRequestMessage::StorageServiceFileCopyRequestMessage(std::string answer_mailbox,
                                                                               WorkflowFile *file,
                                                                               StorageService *src,
                                                                               double payload) : StorageServiceMessage(
            "FILE_COPY_REQUEST", payload) {
      if ((answer_mailbox == "") || (file == nullptr) || (src == nullptr)) {
        throw std::invalid_argument("Invalid constructor arguments");
      }
      this->answer_mailbox = answer_mailbox;
      this->file = file;
      this->src = src;
    }

    /**
     * @brief Constructor
     * @param file: the file
     * @param storage_service: the storage service
     * @param success: true on success, false otherwise
     * @param failure_cause: the cause of a failure (nullptr if success==true)
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    StorageServiceFileCopyAnswerMessage::StorageServiceFileCopyAnswerMessage(WorkflowFile *file,
                                                                             StorageService *storage_service,
                                                                             bool success,
                                                                             WorkflowExecutionFailureCause *failure_cause,
                                                                             double payload)
            : StorageServiceMessage("FILE_COPY_ANSWER", payload) {
      if ((file == nullptr) || (storage_service == nullptr)) {
        throw std::invalid_argument("Invalid constructor arguments");
      }
      this->file = file;
      this->storage_service = storage_service;
      this->success = success;
      this->failure_cause = failure_cause;
    }

    /**
    * @brief Constructor
    * @param answer_mailbox: the mailbox to which to send the answer
    * @param file: the file
    * @param payload: message size in bytes
    *
    * @throw std::invalid_argument
    */
    StorageServiceFileWriteRequestMessage::StorageServiceFileWriteRequestMessage(std::string answer_mailbox,
                                                                                 WorkflowFile *file,
                                                                                 double payload)
            : StorageServiceMessage("FILE_WRITE_REQUEST",
                                    payload + file->getSize()) {
      if ((answer_mailbox == "")) {
        throw std::invalid_argument("Invalid constructor arguments");
      }
      this->answer_mailbox = answer_mailbox;
      this->file = file;
    }

    /**
     * @brief Constructor
     * @param file: the file
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    StorageServiceFileWriteAnswerMessage::StorageServiceFileWriteAnswerMessage(WorkflowFile *file,
                                                                               StorageService *storage_service,
                                                                               bool success,
                                                                               WorkflowExecutionFailureCause *failure_cause,
                                                                               std::string data_write_mailbox_name,
                                                                               double payload) : StorageServiceMessage(
            "FILE_WRITE_ANSWER", payload) {
      if ((file == nullptr) || (storage_service == nullptr)) {
        throw std::invalid_argument("Invalid constructor arguments");
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
   * @param file: the file
   * @param payload: message size in bytes
   *
   * @throw std::invalid_argument
   */
    StorageServiceFileReadRequestMessage::StorageServiceFileReadRequestMessage(std::string answer_mailbox,
                                                                               WorkflowFile *file,
                                                                               double payload) : StorageServiceMessage(
            "FILE_READ_REQUEST",
            payload +
            file->getSize()) {
      if ((answer_mailbox == "") || (file == nullptr)) {
        throw std::invalid_argument("Invalid constructor arguments");
      }
      this->answer_mailbox = answer_mailbox;
      this->file = file;
    }

    /**
     * @brief Constructor
     * @param file: the file
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    StorageServiceFileReadAnswerMessage::StorageServiceFileReadAnswerMessage(WorkflowFile *file,
                                                                             StorageService *storage_service,
                                                                             bool success,
                                                                             WorkflowExecutionFailureCause *failure_cause,
                                                                             double payload) : StorageServiceMessage(
            "FILE_READ_ANSWER",
            payload) {
      if ((file == nullptr) || (storage_service == nullptr)) {
        throw std::invalid_argument("Invalid constructor arguments");
      }
      this->file = file;
      this->storage_service = storage_service;
      this->success = success;
      this->failure_cause = failure_cause;
    }

    /**
    * @brief Constructor
    * @param file: the workflow data file
    * @param payload: message size in bytes
    */
    StorageServiceFileContentMessage::StorageServiceFileContentMessage(WorkflowFile *file) : StorageServiceMessage(
            "FILE_CONTENT", file->getSize()) {
      if (file == nullptr) {
        throw std::invalid_argument("Invalid constructor arguments");
      }
      this->file = file;
    }

};
