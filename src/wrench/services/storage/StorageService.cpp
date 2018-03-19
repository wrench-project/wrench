/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/exceptions/WorkflowExecutionException.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/services/storage/StorageService.h"
#include "services/storage/StorageServiceMessage.h"
#include "wrench/services/storage/StorageServiceProperty.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/simulation/Simulation.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(storage_service, "Log category for Storage Service");


namespace wrench {

    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host on which the service runs
     * @param service_name: the name of the storage service
     * @param mailbox_name_prefix:
     * @param capacity:
     */
    StorageService::StorageService(std::string hostname, std::string service_name, std::string mailbox_name_prefix,
                                   double capacity) :
            Service(hostname, service_name, mailbox_name_prefix) {

      if (capacity < 0) {
        throw std::invalid_argument("SimpleStorageService::SimpleStorageService(): Invalid argument");
      }

      this->simulation = nullptr; // will be filled in via Simulation::add()
      this->state = StorageService::UP;
      this->capacity = capacity;
    }

    /**
     * @brief Internal method to add a file to the storage in a StorageService BEFORE the simulation start
     *
     * @param file: a workflow file
     *
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    void StorageService::stageFile(WorkflowFile *file) {

      if (file == nullptr) {
        throw std::invalid_argument("StorageService::stageFile(): Invalid arguments");
      }

      if (!simgrid::s4u::this_actor::isMaestro()) {
        throw std::runtime_error("StorageService::stageFile(): Can only be called before the simulation starts");
      }

      if (file->getSize() > (this->capacity - this->occupied_space)) {
        WRENCH_WARN("File exceeds free space capacity on storage service (file size: %lf, storage free space: %lf)",
                    file->getSize(), (this->capacity - this->occupied_space));
        throw std::runtime_error("StorageService::stageFile(): File exceeds free space capacity on storage service");
      }
      this->stored_files.insert(file);
      this->occupied_space += file->getSize();
      WRENCH_INFO("Stored file %s (storage usage: %.10lf%%)", file->getId().c_str(),
                  100.0 * this->occupied_space / this->capacity);
    }


    /**
     * @brief Internal method to delete a file from the storage  in a StorageService
     *
     * @param file: a workflow file
     *
     * @throw std::runtime_error
     */
    void StorageService::removeFileFromStorage(WorkflowFile *file) {

      if (file == nullptr) {
        throw std::invalid_argument("StorageService::removeFileFromStorage(): Invalid arguments");
      }

      if (this->stored_files.find(file) == this->stored_files.end()) {
        throw std::runtime_error(
                "StorageService::removeFileFromStorage(): Attempting to remove a file that is not on the storage service");
      }
      this->stored_files.erase(file);
      this->occupied_space -= file->getSize();
      WRENCH_INFO("Deleted file %s (storage usage: %.2lf%%)", file->getId().c_str(),
                  100.0 * this->occupied_space / this->capacity);
    }

    /**
     * @brief Stop the compute service - should be called by the stop()
     *        method of derived classes, if any
     */
    void StorageService::stop() {

      // Call the super class's method
      Service::stop();
    }

    /***************************************************************/
    /****** EVERYTHING BELOW ARE INTERACTIONS WITH THE DAEMON ******/
    /***************************************************************/

    /**
     * @brief Synchronously asks the storage service for its capacity
     * @return the free space in bytes
     *
     * @throw WorkflowExecutionException
     *
     * @throw std::runtime_error
     *
     */
    double StorageService::howMuchFreeSpace() {
      if (this->state == DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      // Send a message to the daemon
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("how_much_free_space");
      try {
        S4U_Mailbox::putMessage(this->mailbox_name, new StorageServiceFreeSpaceRequestMessage(
                answer_mailbox,
                this->getPropertyValueAsDouble(StorageServiceProperty::FREE_SPACE_REQUEST_MESSAGE_PAYLOAD)));
      } catch (FailureCause &cause) {
        throw WorkflowExecutionException(&cause);
      }

      // Wait for a reply
      std::unique_ptr<SimulationMessage> message = nullptr;
      try {
        message = S4U_Mailbox::getMessage(answer_mailbox);
      } catch (FailureCause &cause) {
        throw WorkflowExecutionException(&cause);
      }

      if (auto msg = dynamic_cast<StorageServiceFreeSpaceAnswerMessage *>(message.get())) {
        return msg->free_space;
      } else {
        throw std::runtime_error("StorageService::howMuchFreeSpace(): Unexpected [" + msg->getName() + "] message");
      }
    }

    /**
     * @brief Synchronously asks the storage service whether it holds a file
     *
     * @param file: the file
     *
     * @return true or false
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     * @throw std::invalid_arguments
     */
    bool StorageService::lookupFile(WorkflowFile *file) {

      if (file == nullptr) {
        throw std::invalid_argument("StorageService::lookupFile(): Invalid arguments");
      }

      if (this->state == DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      // Send a message to the daemon
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("lookup_file");
      try {
        S4U_Mailbox::putMessage(this->mailbox_name, new StorageServiceFileLookupRequestMessage(
                answer_mailbox,
                file,
                this->getPropertyValueAsDouble(StorageServiceProperty::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD)));
      } catch (FailureCause &cause) {
        throw WorkflowExecutionException(&cause);
      }

      // Wait for a reply
      std::unique_ptr<SimulationMessage> message;
      try {
        message = S4U_Mailbox::getMessage(answer_mailbox);
      } catch (FailureCause &cause) {
        throw WorkflowExecutionException(&cause);
      }

      if (auto msg = dynamic_cast<StorageServiceFileLookupAnswerMessage *>(message.get())) {
        return msg->file_is_available;
      } else {
        throw std::runtime_error("StorageService::lookupFile(): Unexpected [" + msg->getName() + "] message");
      }
    }

    /**
     * @brief Synchronously read a file from the storage service
     *
     * @param file: the file
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     * @throw std::invalid_arguments
     */
    void StorageService::readFile(WorkflowFile *file) {

      if (file == nullptr) {
        throw std::invalid_argument("StorageService::readFile(): Invalid arguments");
      }

      if (this->state == DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      // Send a synchronous message to the daemon
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("read_file");
      try {
        S4U_Mailbox::putMessage(this->mailbox_name,
                                new StorageServiceFileReadRequestMessage(answer_mailbox,
                                                                         answer_mailbox,
                                                                         file,
                                                                         this->getPropertyValueAsDouble(
                                                                                 StorageServiceProperty::FILE_READ_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      } catch (std::shared_ptr<FatalFailure> &cause) {
        throw WorkflowExecutionException(cause);
      }

      // Wait for a reply
      std::unique_ptr<SimulationMessage> message = nullptr;

      try {
        message = S4U_Mailbox::getMessage(answer_mailbox);
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      } catch (std::shared_ptr<FatalFailure> &cause) {
        throw WorkflowExecutionException(cause);
      }

      if (auto msg = dynamic_cast<StorageServiceFileReadAnswerMessage *>(message.get())) {
        // If it's not a success, throw an exception
        if (not msg->success) {
          std::shared_ptr<FailureCause> &cause = msg->failure_cause;
          throw WorkflowExecutionException(cause);
        }

        // Otherwise, retrieve  the file
        std::unique_ptr<SimulationMessage> file_content_message = nullptr;
        try {
          file_content_message = S4U_Mailbox::getMessage(answer_mailbox);
        } catch (std::shared_ptr<NetworkError> &cause) {
          WRENCH_INFO("Network Error while getting a file content");
          throw WorkflowExecutionException(cause);
        } catch (std::shared_ptr<FatalFailure> &cause) {
          WRENCH_INFO("Unknown Error while getting a file content.... means we should just die. but throwing");
          throw WorkflowExecutionException(cause);
        }

        if (auto file_content_msg = dynamic_cast<StorageServiceFileContentMessage *>(file_content_message.get())) {
          // do nothing
        } else {
          throw std::runtime_error("StorageService::readFile(): Received an unexpected [" +
                                   file_content_message->getName() + "] message!");
        }

      } else {
        throw std::runtime_error("StorageService::readFile(): Received an unexpected [" +
                                 message->getName() + "] message!");
      }
    }


    /**
     * @brief Synchronously write a file to the storage service
     *
     * @param file: the file
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void StorageService::writeFile(WorkflowFile *file) {

      if (file == nullptr) {
        throw std::invalid_argument("StorageService::writeFile(): Invalid arguments");
      }

      if (this->state == DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      // Send a synchronous message to the daemon
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("write_file");
      try {
        S4U_Mailbox::putMessage(this->mailbox_name,
                                new StorageServiceFileWriteRequestMessage(answer_mailbox,
                                                                          file,
                                                                          this->getPropertyValueAsDouble(
                                                                                  StorageServiceProperty::FILE_WRITE_REQUEST_MESSAGE_PAYLOAD)));
      } catch (FailureCause &cause) {
        throw WorkflowExecutionException(&cause);
      } catch (std::exception &e) {
        WRENCH_INFO("Got a weird exception..... returning");
        return;
      }

      // Wait for a reply
      std::unique_ptr<SimulationMessage> message;

      try {
        message = S4U_Mailbox::getMessage(answer_mailbox);
      } catch (FailureCause &cause) {
        throw WorkflowExecutionException(&cause);
      }

      if (auto msg = dynamic_cast<StorageServiceFileWriteAnswerMessage *>(message.get())) {
        // If not a success, throw an exception
        if (not msg->success) {
          throw WorkflowExecutionException(msg->failure_cause);
        }

        // Otherwise, synchronously send the file up!
        try {
          S4U_Mailbox::putMessage(msg->data_write_mailbox_name, new StorageServiceFileContentMessage(file));
        } catch (FailureCause &cause) {
          throw WorkflowExecutionException(&cause);
        }

      } else {
        throw std::runtime_error("StorageService::writeFile(): Received an unexpected [" +
                                 message->getName() + "] message!");
      }
    }

    /**
     * @brief Synchronously and sequentially read a set of files from storage services
     *
     * @param files: the set of files to read
     * @param file_locations: a map of files to storage services
     * @param default_storage_service: the storage service to use when files don't appear in the file_locations map
     * @return nullptr on success, or a workflow execution failure cause on failure
     *
     * @throw std::runtime_error
     * @throw WorkflowExecutionException
     */
    void StorageService::readFiles(std::set<WorkflowFile *> files,
                                   std::map<WorkflowFile *, StorageService *> file_locations,
                                   StorageService *default_storage_service) {
      try {
        StorageService::writeOrReadFiles(READ, std::move(files), std::move(file_locations), default_storage_service);
      } catch (std::runtime_error &e) {
        throw;
      } catch (WorkflowExecutionException &e) {
        throw;
      }
    }

    /**
     * @brief Synchronously and sequentially uppload a set of files from storage services
     *
     * @param files: the set of files to write
     * @param file_locations: a map of files to storage services
     * @param default_storage_service: the storage service to use when files don't appear in the file_locations map
     * @return nullptr on success, or a workflow execution failure cause on failure
     *
     * @throw std::runtime_error
     * @throw WorkflowExecutionException
     */
    void StorageService::writeFiles(std::set<WorkflowFile *> files,
                                    std::map<WorkflowFile *, StorageService *> file_locations,
                                    StorageService *default_storage_service) {
      try {
        StorageService::writeOrReadFiles(WRITE, std::move(files), std::move(file_locations), default_storage_service);
      } catch (std::runtime_error &e) {
        throw;
      } catch (WorkflowExecutionException &e) {
        throw;
      }
    }

    /**
     * @brief Synchronously and sequentially write/read a set of files to/from storage services
     *
     * @param action: DONWLOAD or WRITE
     * @param files: the set of files to read/write
     * @param file_locations: a map of files to storage services
     * @param default_storage_service: the storage service to use when files don't appear in the file_locations map
     *
     * @throw std::runtime_error
     * @throw WorkflowExecutionException
     */
    void StorageService::writeOrReadFiles(FileOperation action,
                                          std::set<WorkflowFile *> files,
                                          std::map<WorkflowFile *, StorageService *> file_locations,
                                          StorageService *default_storage_service) {

      for (auto const &f : files) {
        if (f == nullptr) {
          throw std::invalid_argument("StorageService::writeOrReadFiles(): invalid files argument");
        }
      }
      for (auto const &l : file_locations) {
        if ((l.first == nullptr) || (l.second == nullptr)) {
          throw std::invalid_argument("StorageService::writeOrReadFiles(): invalid file location argument");
        }
      }
      for (auto const &f : files) {

        // Identify the Storage Service
        StorageService *storage_service = default_storage_service;
        if (file_locations.find(f) != file_locations.end()) {
          storage_service = file_locations[f];
        }
        if (storage_service == nullptr) {
          throw WorkflowExecutionException(new NoStorageServiceForFile(f));
        }
        if (action == READ) {
          try {
            WRENCH_INFO("Reading file %s from storage service %s", f->getId().c_str(), storage_service->getName().c_str());
            storage_service->readFile(f);
            WRENCH_INFO("File %s read", f->getId().c_str());
          } catch (std::runtime_error &e) {
            throw;
          } catch (WorkflowExecutionException &e) {
            throw;
          }
        } else {
          // Write the file
          try {
            WRENCH_INFO("Writing file %s to storage service %s", f->getId().c_str(), storage_service->getName().c_str());
            storage_service->writeFile(f);
            WRENCH_INFO("Wrote file %s", f->getId().c_str());
          } catch (std::runtime_error &e) {
            throw;
          } catch (WorkflowExecutionException &e) {
            throw;
          }
        }
      }
    }

    /**
     * @brief Synchronously asks the storage service to delete a file copy
     *
     * @param file: the file
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     * @throw std::invalid_argument
     */
    void StorageService::deleteFile(WorkflowFile *file) {

      if (file == nullptr) {
        throw std::invalid_argument("StorageService::deleteFile(): Invalid arguments");
      }

      if (this->state == DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      // Send a message to the daemon
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("delete_file");
      try {
        S4U_Mailbox::putMessage(this->mailbox_name, new StorageServiceFileDeleteRequestMessage(
                answer_mailbox,
                file,
                this->getPropertyValueAsDouble(StorageServiceProperty::FILE_DELETE_REQUEST_MESSAGE_PAYLOAD)));
      } catch (FailureCause &cause) {
        throw WorkflowExecutionException(&cause);
      }

      // Wait for a reply
      std::unique_ptr<SimulationMessage> message = nullptr;

      try {
        message = S4U_Mailbox::getMessage(answer_mailbox);
      } catch (FailureCause &cause) {
        throw WorkflowExecutionException(&cause);
      }

      if (auto msg = dynamic_cast<StorageServiceFileDeleteAnswerMessage *>(message.get())) {
        // On failure, throw an exception
        if (!msg->success) {
          throw WorkflowExecutionException(std::move(msg->failure_cause));
        }
        WRENCH_INFO("Deleted file %s on storage service %s", file->getId().c_str(), this->getName().c_str());
      } else {
        throw std::runtime_error("StorageService::deleteFile(): Unexpected [" + message->getName() + "] message");
      }
    }

    /**
     * @brief Synchronously and sequentially delete a set of files from storage services
     *
     * @param files: the set of files to delete
     * @param file_locations: a map of files to storage services
     * @param default_storage_service: the storage service to use when files don't appear in the file_locations map
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void StorageService::deleteFiles(std::set<WorkflowFile *> files,
                                     std::map<WorkflowFile *, StorageService *> file_locations,
                                     StorageService *default_storage_service) {
      for (auto f : files) {
        // Identify the Storage Service
        StorageService *storage_service = default_storage_service;
        if (file_locations.find(f) != file_locations.end()) {
          storage_service = file_locations[f];
        }
        if (storage_service == nullptr) {
          throw WorkflowExecutionException(new NoStorageServiceForFile(f));
        }

        // Remove the file
        try {
          storage_service->deleteFile(f);
        } catch (WorkflowExecutionException &e) {
          throw;
        } catch (std::runtime_error &e) {
          throw;
        }
      }
    }

    /**
     * @brief Synchronously asks the storage service to read a file from another storage service
     *
     * @param file: the file to copy
     * @param src: the storage service from which to read the file
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     * @throw std::invalid_argument
     */
    void StorageService::copyFile(WorkflowFile *file, StorageService *src) {


      if ((file == nullptr) || (src == nullptr)) {
        throw std::invalid_argument("StorageService::copyFile(): Invalid arguments");
      }

      if (src == this) {
        throw std::invalid_argument("StorageService::copyFile(): Cannot copy a file from oneself");
      }

      if (this->state == DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      // Send a message to the daemon
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("copy_file");
      try {
        S4U_Mailbox::putMessage(this->mailbox_name, new StorageServiceFileCopyRequestMessage(
                answer_mailbox,
                file,
                src,
                this->getPropertyValueAsDouble(StorageServiceProperty::FILE_COPY_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      } catch (std::shared_ptr<FatalFailure> &cause) {
        throw WorkflowExecutionException(cause);
      }

      // Wait for a reply
      std::unique_ptr<SimulationMessage> message = nullptr;

      try {
        message = S4U_Mailbox::getMessage(answer_mailbox);
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      } catch (std::shared_ptr<FatalFailure> &cause) {
        throw WorkflowExecutionException(cause);
      }

      if (auto msg = dynamic_cast<StorageServiceFileCopyAnswerMessage *>(message.get())) {
        if (msg->failure_cause) {
          throw WorkflowExecutionException(std::move(msg->failure_cause));
        }
      } else {
        throw std::runtime_error("StorageService::copyFile(): Unexpected [" + message->getName() + "] message");
      }
    }

    /**
     * @brief Asynchronously asks the storage service to read a file from another storage service
     *
     * @param answer_mailbox: the mailbox to which a notification message will be sent
     * @param file: the file
     * @param src: the storage service from which to read the file
     *
     * @throw WorkflowExecutionException
     * @throw std::invalid_argument
     * @throw std::runtime_error
     *
     */
    void StorageService::initiateFileCopy(std::string answer_mailbox, WorkflowFile *file, StorageService *src) {

      if ((file == nullptr) || (src == nullptr)) {
        throw std::invalid_argument("StorageService::initiateFileCopy(): Invalid arguments");
      }

      if (src == this) {
        throw std::invalid_argument("StorageService::copyFile(): Cannot copy a file from oneself");
      }

      if (this->state == DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      // Send a message to the daemon
      try {
        S4U_Mailbox::putMessage(this->mailbox_name, new StorageServiceFileCopyRequestMessage(
                answer_mailbox,
                file,
                src,
                this->getPropertyValueAsDouble(StorageServiceProperty::FILE_COPY_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      } catch (std::shared_ptr<FatalFailure> &cause) {
        throw WorkflowExecutionException(cause);
      }
    }


    /**
     * @brief Asynchrously read a file from the storage service
     *
     * @param answer_mailbox: the mailbox to which the file should be sent
     * @param file: the file
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     * @throw std::invalid_arguments
     */
    void StorageService::initiateFileRead(std::string mailbox_that_should_receive_file_content, WorkflowFile *file) {

      WRENCH_INFO("Initiating a file read operation for file %s on storage service %s",
                  file->getId().c_str(), this->getName().c_str());

      if (file == nullptr) {
        throw std::invalid_argument("StorageService::initiateFileRead(): Invalid arguments");
      }

      if (this->state == DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      // Send a synchronous message to the daemon
      std::string request_answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("read_file");

      try {
        S4U_Mailbox::putMessage(this->mailbox_name,
                                new StorageServiceFileReadRequestMessage(request_answer_mailbox,
                                                                         mailbox_that_should_receive_file_content,
                                                                         file,
                                                                         this->getPropertyValueAsDouble(
                                                                                 StorageServiceProperty::FILE_READ_REQUEST_MESSAGE_PAYLOAD)));
      } catch (FailureCause &cause) {
        throw WorkflowExecutionException(&cause);
      }


      // Wait for a reply to the request
      std::unique_ptr<SimulationMessage> message = nullptr;

      try {
        message = S4U_Mailbox::getMessage(request_answer_mailbox);
      } catch (FailureCause &cause) {
        throw WorkflowExecutionException(&cause);
      }

      if (auto msg = dynamic_cast<StorageServiceFileReadAnswerMessage *>(message.get())) {
        // If it's not a success, throw an exception
        if (not msg->success) {
          throw WorkflowExecutionException(msg->failure_cause);
        }
      } else {
        throw std::runtime_error("StorageService::initiateFileRead(): Received an unexpected [" +
                                 message->getName() + "] message!");
      }

      WRENCH_INFO("File read request accepted (will receive file content on mailbox_name %s)",
                  mailbox_that_should_receive_file_content.c_str());
      // At this point, the file should show up at some point on the mailbox_that_should_receive_file_content

    }
};
