/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/exceptions/WorkflowExecutionException.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/services/storage/StorageService.h"
#include "wrench/services/compute/cloud/CloudComputeService.h"
#include "wrench/services/compute/virtualized_cluster/VirtualizedClusterComputeService.h"
#include "services/storage/StorageServiceMessage.h"
#include "wrench/services/storage/StorageServiceMessagePayload.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/simulation/Simulation.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(storage_service, "Log category for Storage Service");


namespace wrench {

    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host on which the service should run
     * @param service_name: the name of the storage service
     * @param mailbox_name_prefix: the mailbox name prefix
     * @param capacity: the storage capacity in bytes
     */
    StorageService::StorageService(const std::string &hostname, const std::string &service_name,
                                   const std::string &mailbox_name_prefix, double capacity) :
            Service(hostname, service_name, mailbox_name_prefix) {

      if (capacity < 0) {
        throw std::invalid_argument("SimpleStorageService::SimpleStorageService(): Invalid argument");
      }

      this->state = StorageService::UP;
      this->capacity = capacity;
    }

    /**
     * @brief Store a file on the storage service BEFORE the simulation is launched
     *
     * @param file: a file
     *
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    void StorageService::stageFile(WorkflowFile *file) {

      if (file == nullptr) {
        throw std::invalid_argument("StorageService::stageFile(): Invalid arguments");
      }

      if (!simgrid::s4u::this_actor::is_maestro()) {
        throw std::runtime_error("StorageService::stageFile(): Can only be called before the simulation starts");
      }

      if (file->getSize() > (this->capacity - this->occupied_space)) {
        WRENCH_WARN("File exceeds free space capacity on storage service (file size: %lf, storage free space: %lf)",
                    file->getSize(), (this->capacity - this->occupied_space));
        throw std::runtime_error("StorageService::stageFile(): File exceeds free space capacity on storage service");
      }
      if (this->stored_files.find("/") != this->stored_files.end()) {
        this->stored_files["/"].insert(file);
      } else {
        this->stored_files["/"] = {file}; // By default all the staged files will go to the "/" partition
      }
      this->occupied_space += file->getSize();
      WRENCH_INFO("Stored file %s (storage usage: %.4lf%%)", file->getID().c_str(),
                  100.0 * this->occupied_space / this->capacity);
    }


    /**
     * @brief Remove a file from storage (internal method)
     *
     * @param file: a file
     * @param dst_partition: the partition in which the file will be deleted
     *
     * @throw std::runtime_error
     */
    void StorageService::removeFileFromStorage(WorkflowFile *file, std::string dst_partition) {

      if (file == nullptr) {
        throw std::invalid_argument("StorageService::removeFileFromStorage(): Invalid arguments");
      }

      // Empty partition means "/"
      if (dst_partition.empty()) {
        dst_partition = "/";
      }

      std::set<WorkflowFile *> files = this->stored_files[dst_partition];
      if (not files.empty()) {
        if (files.find(file) == files.end()) {
          throw std::runtime_error(
                  "StorageService::removeFileFromStorage(): Attempting to remove a file that is not on the storage service");
        }
        this->stored_files[dst_partition].erase(file);
        this->occupied_space -= file->getSize();
        WRENCH_INFO("Deleted file %s (storage usage: %.2lf%%)", file->getID().c_str(),
                    100.0 * this->occupied_space / this->capacity);
      } else {
        throw std::runtime_error(
                "StorageService::removeFileFromStorage(): Attempting to remove a file that is not on the storage service");
      }
    }

    /**
     * @brief Stop the service
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
     * @return The free space in bytes
     *
     * @throw WorkflowExecutionException
     *
     * @throw std::runtime_error
     *
     */
    double StorageService::getFreeSpace() {

      assertServiceIsUp();

      // Send a message to the daemon
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("how_much_free_space");
      try {
        S4U_Mailbox::putMessage(this->mailbox_name, new StorageServiceFreeSpaceRequestMessage(
                answer_mailbox,
                this->getMessagePayloadValue(
                        StorageServiceMessagePayload::FREE_SPACE_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }

      // Wait for a reply
      std::unique_ptr<SimulationMessage> message = nullptr;
      try {
        message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
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
     * @throw std::invalid_arguments
     */
    bool StorageService::lookupFile(WorkflowFile *file) {

      if (file == nullptr) {
        throw std::invalid_argument("StorageService::lookupFile(): Invalid arguments");
      }

      assertServiceIsUp();

      std::string dst_partition = "/";
      return this->lookupFile(file, dst_partition);
    }

    /**
     * @brief Synchronously asks the storage service whether it holds a file
     *
     * @param file: the file
     * @param job: the job for whom we are doing the look up, the file is stored in this job's partition
     *
     * @return true or false
     *
     * @throw WorkflowExecutionException
     * @throw std::invalid_arguments
     */
    bool StorageService::lookupFile(WorkflowFile *file, WorkflowJob *job) {

      if (file == nullptr) {
        throw std::invalid_argument("StorageService::lookupFile(): Invalid arguments");
      }

      assertServiceIsUp();

      std::string dst_partition = "/";
      if (job != nullptr) {
        dst_partition += job->getName();
      }
      return this->lookupFile(file, dst_partition);
    }

    /**
     * @brief Synchronously asks the storage service whether it holds a file
     *
     * @param file: the file
     * @param dst_partition: the partition in which to perform the lookup
     *
     * @return true or false
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     * @throw std::invalid_arguments
     */
    bool StorageService::lookupFile(WorkflowFile *file, std::string dst_partition) {

      if (file == nullptr) {
        throw std::invalid_argument("StorageService::lookupFile(): Invalid arguments");
      }

      assertServiceIsUp();

      // Empty partition means "/"
      if (dst_partition.empty()) {
        dst_partition = "/";
      }

      // Send a message to the daemon
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("lookup_file");
      try {
        S4U_Mailbox::putMessage(this->mailbox_name, new StorageServiceFileLookupRequestMessage(
                answer_mailbox,
                file,
                dst_partition,
                this->getMessagePayloadValue(
                        StorageServiceMessagePayload::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }

      // Wait for a reply
      std::unique_ptr<SimulationMessage> message;
      try {
        message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
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
     * @throw std::invalid_arguments
     */
    void StorageService::readFile(WorkflowFile *file) {

      std::string src_partition = "/";
      this->readFile(file, src_partition);
    }

    /**
     * @brief Synchronously read a file from the storage service
     *
     * @param file: the file
     * @param job: the job associated to the read of the workflow file
     *
     * @throw WorkflowExecutionException
     * @throw std::invalid_arguments
     */
    void StorageService::readFile(WorkflowFile *file, WorkflowJob *job) {


      std::string src_partition = "/";
      if (job != nullptr) {
        src_partition += job->getName();
      }
      this->readFile(file, src_partition);
    }

    /**
     * @brief Synchronously read a file from the storage service
     *
     * @param file: the file
     * @param src_partition: the partition from which to read the file
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     * @throw std::invalid_arguments
     */

    void StorageService::readFile(WorkflowFile *file, std::string src_partition) {

      if (file == nullptr) {
        throw std::invalid_argument("StorageService::readFile(): Invalid arguments");
      }

      assertServiceIsUp();

      // Empty partition means "/"
      if (src_partition.empty()) {
        src_partition = "/";
      }


      // Send a message to the daemon
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("read_file");
      try {
        S4U_Mailbox::putMessage(this->mailbox_name,
                                new StorageServiceFileReadRequestMessage(
                                        answer_mailbox,
                                        answer_mailbox,
                                        file,
                                        src_partition,
                                        this->getMessagePayloadValue(
                                                StorageServiceMessagePayload::FILE_READ_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }

      // Wait for a reply
      std::unique_ptr<SimulationMessage> message = nullptr;

      try {
        message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
      } catch (std::shared_ptr<NetworkError> &cause) {
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
     */
    void StorageService::writeFile(WorkflowFile *file) {

      std::string dst_partition = "/";
      this->writeFile(file, dst_partition);
    }


    /**
     * @brief Synchronously write a file to the storage service
     *
     * @param file: the file
     * @param job: the job associated to the write of the workflow file
     *
     * @throw WorkflowExecutionException
     */
    void StorageService::writeFile(WorkflowFile *file, WorkflowJob *job) {

      std::string dst_partition = "/";
      if (job != nullptr) {
        dst_partition += job->getName();
      }
      this->writeFile(file, dst_partition);
    }

    /**
     * @brief Synchronously write a file to the storage service
     *
     * @param file: the file
     * @param dst_partition: the partition in which to write the file
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void StorageService::writeFile(WorkflowFile *file, std::string dst_partition) {

      if (file == nullptr) {
        throw std::invalid_argument("StorageService::writeFile(): Invalid arguments");
      }

      assertServiceIsUp();

      // Empty partition means "/"
      if (dst_partition.empty()) {
        dst_partition = "/";
      }


      // Send a  message to the daemon
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("write_file");
      try {
        S4U_Mailbox::putMessage(this->mailbox_name,
                                new StorageServiceFileWriteRequestMessage(
                                        answer_mailbox,
                                        file,
                                        dst_partition,
                                        this->getMessagePayloadValue(
                                                StorageServiceMessagePayload::FILE_WRITE_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }

      // Wait for a reply
      std::unique_ptr<SimulationMessage> message;

      try {
        message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }

      if (auto msg = dynamic_cast<StorageServiceFileWriteAnswerMessage *>(message.get())) {
        // If not a success, throw an exception
        if (not msg->success) {
          throw WorkflowExecutionException(msg->failure_cause);
        }

        // Otherwise, synchronously send the file up!
        try {
          S4U_Mailbox::putMessage(msg->data_write_mailbox_name, new StorageServiceFileContentMessage(file));
        } catch (std::shared_ptr<NetworkError> &cause) {
          throw WorkflowExecutionException(cause);
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
     * @param default_storage_service: the storage service to use when files don't appear in the file_locations map (which must be a compute service's scratch storage)
     * @param files_in_scratch: the set of files that have been written to the default storage service (which must be a compute service's scratch storage)
     * @param job: the job which is doing the read of the files
     *
     * @throw std::runtime_error
     * @throw WorkflowExecutionException
     */
    void StorageService::readFiles(std::set<WorkflowFile *> files,
                                   std::map<WorkflowFile *, std::shared_ptr<StorageService>> file_locations,
                                   std::shared_ptr<StorageService> default_storage_service,
                                   std::set<WorkflowFile *> &files_in_scratch,
                                   WorkflowJob *job) {
      try {
        StorageService::writeOrReadFiles(READ, std::move(files), std::move(file_locations), default_storage_service,
                                         files_in_scratch, job);
      } catch (std::runtime_error &e) {
        throw;
      } catch (WorkflowExecutionException &e) {
        throw;
      }
    }

    /**
     * @brief Synchronously and sequentially upload a set of files from storage services
     *
     * @param files: the set of files to write
     * @param file_locations: a map of files to storage services
     * @param default_storage_service: the storage service to use when files don't appear in the file_locations map (which must be a compute service's scratch storage)
     * @param files_in_scratch: the set of files that have been writted to the default storage service (which must be a compute service's scratch storage)
     * @param job: the job which is doing the write of the files
     *
     *
     * @throw std::runtime_error
     * @throw WorkflowExecutionException
     */
    void StorageService::writeFiles(std::set<WorkflowFile *> files,
                                    std::map<WorkflowFile *, std::shared_ptr<StorageService>> file_locations,
                                    std::shared_ptr<StorageService> default_storage_service,
                                    std::set<WorkflowFile *> &files_in_scratch,
                                    WorkflowJob *job) {
      try {
        StorageService::writeOrReadFiles(WRITE, std::move(files), std::move(file_locations), default_storage_service,
                                         files_in_scratch, job);
      } catch (std::runtime_error &e) {
        throw;
      } catch (WorkflowExecutionException &e) {
        throw;
      }
    }

    /**
     * @brief Synchronously and sequentially write/read a set of files to/from storage services
     *
     * @param action: FileOperation::READ (download) or FileOperation::WRITE
     * @param files: the set of files to read/write
     * @param file_locations: a map of files to storage services
     * @param default_storage_service: the storage service to use when files don't appear in the file_locations map (which must be a compute service's scratch storage)
     * @param files_in_scratch: the set of files that have been written to the default storage service (which must be a compute service's scratch storage)
     * @param job: the job associated to the write/read of the files
     *
     * @throw std::runtime_error
     * @throw WorkflowExecutionException
     */
    void StorageService::writeOrReadFiles(FileOperation action,
                                          std::set<WorkflowFile *> files,
                                          std::map<WorkflowFile *, std::shared_ptr<StorageService>> file_locations,
                                                  std::shared_ptr<StorageService> default_storage_service,
                                                  std::set<WorkflowFile *> &files_in_scratch,
                                                  WorkflowJob *job) {

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

      // Create a temporary sorted list of files so that the order in which files are read/written is deterministic!
      std::map<std::string, WorkflowFile *> sorted_files;
      for (auto const &f : files) {
        sorted_files.insert(std::make_pair(f->getID(), f));
      }

      for (auto const &f : sorted_files) {

        WorkflowFile *file = f.second;

        // Identify the Storage Service
        std::shared_ptr<StorageService> storage_service = default_storage_service;
        if (file_locations.find(file) != file_locations.end()) {
          storage_service = file_locations[file];
        }
        if (storage_service == nullptr) {
          throw WorkflowExecutionException(std::shared_ptr<FailureCause>(new NoStorageServiceForFile(file)));
        }

        if (action == READ) {
          try {
            WRENCH_INFO("Reading file %s from storage service %s", file->getID().c_str(),
                        storage_service->getName().c_str());
            if (storage_service != default_storage_service) {
              //if the storage service where I am going to read from is not the default storage service (scratch), then I
              // don't want to read from job's temp partition, rather I would like to read from / partition of the storage service
              storage_service->readFile(file, nullptr);
            } else {
              storage_service->readFile(file, job);
            }
            WRENCH_INFO("File %s read", file->getID().c_str());
          } catch (std::runtime_error &e) {
            throw;
          } catch (WorkflowExecutionException &e) {
            throw;
          }
        } else {
          try {
            WRENCH_INFO("Writing file %s to storage service %s", file->getID().c_str(),
                        storage_service->getName().c_str());
            // Write the file
            if (storage_service == default_storage_service) {
              files_in_scratch.insert(file);
              storage_service->writeFile(file, job);
            } else {
              storage_service->writeFile(file, nullptr);
            }
            WRENCH_INFO("Wrote file %s", file->getID().c_str());
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
     * @param file_registry_service: a file registry service that should be updated once the
     *         file deletion has (successfully) completed (none if nullptr)
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     * @throw std::invalid_argument
     */
    void StorageService::deleteFile(WorkflowFile *file, std::shared_ptr<FileRegistryService> file_registry_service) {

      if (file == nullptr) {
        throw std::invalid_argument("StorageService::deleteFile(): Invalid arguments");
      }

      assertServiceIsUp();

      std::string dst_partition = "/";
      this->deleteFile(file, dst_partition, file_registry_service);
    }

    /**
     * @brief Synchronously ask the storage service to delete a file copy
     *
     * @param file: the file
     * @param job: the job associated to deleting this file
     * @param file_registry_service: a file registry service that should be updated once the
     *         file deletion has (successfully) completed (none if nullptr)
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     * @throw std::invalid_argument
     */
    void StorageService::deleteFile(WorkflowFile *file, WorkflowJob *job, std::shared_ptr<FileRegistryService>file_registry_service) {

      if (file == nullptr) {
        throw std::invalid_argument("StorageService::deleteFile(): Invalid arguments");
      }

      assertServiceIsUp();

      std::string dst_partition = "/";
      if (job != nullptr) {
        dst_partition += job->getName();
      }
      this->deleteFile(file, dst_partition, file_registry_service);
    }

    /** @brief Synchronously ask the storage service to delete a file copy
    *
    * @param file: the file
    * @param dst_partition: the partition in which to delete the file
    * @param file_registry_service: a file registry service that should be updated once the
    *         file deletion has (successfully) completed (none if nullptr)
    *
    * @throw WorkflowExecutionException
    * @throw std::runtime_error
    * @throw std::invalid_argument
    */
    void StorageService::deleteFile(WorkflowFile *file, std::string dst_partition,
                                    std::shared_ptr<FileRegistryService> file_registry_service) {

      // Empty partition means "/"
      if (dst_partition.empty()) {
        dst_partition = "/";
      }

      bool unregister = (file_registry_service != nullptr);
      // Send a message to the daemon
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("delete_file");
      try {
        S4U_Mailbox::putMessage(this->mailbox_name, new StorageServiceFileDeleteRequestMessage(
                answer_mailbox,
                file,
                dst_partition,
                this->getMessagePayloadValue(
                        StorageServiceMessagePayload::FILE_DELETE_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }

      // Wait for a reply
      std::unique_ptr<SimulationMessage> message = nullptr;

      try {
        message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }

      if (auto msg = dynamic_cast<StorageServiceFileDeleteAnswerMessage *>(message.get())) {
        // On failure, throw an exception
        if (!msg->success) {
          throw WorkflowExecutionException(std::move(msg->failure_cause));
        }
        WRENCH_INFO("Deleted file %s on storage service %s", file->getID().c_str(), this->getName().c_str());

        if (unregister) {
          file_registry_service->removeEntry(file,
                  this->getSharedPtr<StorageService>());
        }

      } else {
        throw std::runtime_error("StorageService::deleteFile(): Unexpected [" + message->getName() + "] message");
      }
    }

    /**
     * @brief Synchronously and sequentially delete a set of files from storage services
     *
     * @param files: the set of files to delete
     * @param file_locations: a map of files to storage services (all must be in the "/" partition of their storage services)
     * @param default_storage_service: the storage service to use when files don't appear in the file_locations map (or nullptr if none)
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void StorageService::deleteFiles(std::set<WorkflowFile *> files,
                                     std::map<WorkflowFile *, std::shared_ptr<StorageService>> file_locations,
                                     std::shared_ptr<StorageService> default_storage_service) {
      for (auto f : files) {
        // Identify the Storage Service
        std::shared_ptr<StorageService> storage_service = default_storage_service;
        if (file_locations.find(f) != file_locations.end()) {
          storage_service = file_locations[f];
        }
        if (storage_service == nullptr) {
          throw WorkflowExecutionException(std::shared_ptr<FailureCause>(new NoStorageServiceForFile(f)));
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
     * @brief Synchronously ask the storage service to read a file from another storage service
     *
     * @param file: the file to copy
     * @param src: the storage service from which to read the file
     * @throw WorkflowExecutionException
     * @throw std::invalid_argument
     */
    void StorageService::copyFile(WorkflowFile *file, std::shared_ptr<StorageService> src) {


      if ((file == nullptr) || (src == nullptr)) {
        throw std::invalid_argument("StorageService::copyFile(): Invalid arguments");
      }

      if (src == this->getSharedPtr<StorageService>()) {
        throw std::invalid_argument(
                "StorageService::copyFile(file,src): Cannot redundantly copy a file to its own partition");
      }

      assertServiceIsUp();

      std::string src_partition = "/";

      std::string dst_partition = "/";

      this->copyFile(file, src, src_partition, dst_partition);
    }

    /**
     * @brief Synchronously ask the storage service to read a file from another storage service
     *
     * @param file: the file to copy
     * @param src: the storage service from which to read the file
     * @param src_job: the job from whose partition we are copying this file
     * @param dst_job: the job to whose partition we are copying this file
     * @throw WorkflowExecutionException
     * @throw std::invalid_argument
     */
    void StorageService::copyFile(WorkflowFile *file, std::shared_ptr<StorageService> src, WorkflowJob *src_job, WorkflowJob *dst_job) {


      if ((file == nullptr) || (src == nullptr)) {
        throw std::invalid_argument("StorageService::copyFile(): Invalid arguments");
      }

      if (src == this->getSharedPtr<StorageService>() && src_job == nullptr && dst_job == nullptr) {
        throw std::invalid_argument("StorageService::copyFile(): Cannot redundantly copy a file to its own partition");
      }

      if (src_job != nullptr && dst_job != nullptr) {
        throw std::invalid_argument(
                "StorageService::copyFile(file,src,src_job,dst_job): Cannot copy files from one job's private partition to another job's private partition");
      }

      assertServiceIsUp();

      std::string src_partition = "/";
      if (src_job != nullptr) {
        src_partition += src_job->getName();
      }

      std::string dst_partition = "/";
      if (dst_job != nullptr) {
        dst_partition += dst_job->getName();
      }

      this->copyFile(file, src, src_partition, dst_partition);
    }

    /**
     * @brief Synchronously ask the storage service to read a file from another storage service
     *
     * @param file: the file to copy
     * @param src: the storage service from which to read the file
     * @param src_partition: the partition in which the file will be read
     * @param dst_partition: the partition in which the file will be written
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     * @throw std::invalid_argument
     */
    void StorageService::copyFile(WorkflowFile *file, std::shared_ptr<StorageService> src, std::string src_partition,
                                  std::string dst_partition) {

      if (src.get() == this && (src_partition == dst_partition)) {
        throw std::invalid_argument(
                "StorageService::copyFile(file,src,src_partition,dst_partition): Cannot redundantly copy a file to its own partition");
      }

      // Empty partition means "/"
      if (dst_partition.empty()) {
        dst_partition = "/";
      }
      if (src_partition.empty()) {
        src_partition = "/";
      }

      // Send a message to the daemon
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("copy_file");
      auto start_timestamp = new SimulationTimestampFileCopyStart(file, src, src_partition, this->getSharedPtr<StorageService>(), dst_partition);
      this->simulation->getOutput().addTimestamp<SimulationTimestampFileCopyStart>(start_timestamp);

      try {
        S4U_Mailbox::putMessage(this->mailbox_name, new StorageServiceFileCopyRequestMessage(
                answer_mailbox,
                file,
                src,
                src_partition,
                this->getSharedPtr<StorageService>(),
                dst_partition,
                nullptr,
                start_timestamp,
                this->getMessagePayloadValue(StorageServiceMessagePayload::FILE_COPY_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }

      // Wait for a reply
      std::unique_ptr<SimulationMessage> message = nullptr;

      try {
        message = S4U_Mailbox::getMessage(answer_mailbox);
      } catch (std::shared_ptr<NetworkError> &cause) {
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
     * @brief Asynchronously ask the storage service to read a file from another storage service
     *
     * @param answer_mailbox: the mailbox to which a notification message will be sent
     * @param file: the file
     * @param src: the storage service from which to read the file
     * @param src_partition: the source partition
     * @param dst_partition: the destination partition
     *
     * @throw WorkflowExecutionException
     * @throw std::invalid_argument
     *
     */
    void StorageService::initiateFileCopy(std::string answer_mailbox, WorkflowFile *file, std::shared_ptr<StorageService> src,
                                          std::string src_partition, std::string dst_partition) {

      if ((file == nullptr) || (src == nullptr)) {
        throw std::invalid_argument("StorageService::initiateFileCopy(): Invalid arguments");
      }

      // Empty partition means "/"
      if (src_partition.empty()) {
        src_partition = "/";
      }
      if (dst_partition.empty()) {
        dst_partition = "/";
      }

      if ((src.get() == this) && (src_partition == dst_partition)) {
        throw std::invalid_argument(
                "StorageService::copyFile(): Cannot redundantly copy a file to the its own partition");
      }

      assertServiceIsUp();

      auto start_timestamp = new SimulationTimestampFileCopyStart(file, src, src_partition, this->getSharedPtr<StorageService>(), dst_partition);
      this->simulation->getOutput().addTimestamp<SimulationTimestampFileCopyStart>(start_timestamp);

      // Send a message to the daemon
      try {
        S4U_Mailbox::putMessage(this->mailbox_name, new StorageServiceFileCopyRequestMessage(
                answer_mailbox,
                file,
                src,
                src_partition,
                this->getSharedPtr<StorageService>(),
                dst_partition,
                nullptr,
                start_timestamp,
                this->getMessagePayloadValue(StorageServiceMessagePayload::FILE_COPY_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }
    }


    /**
     * @brief Asynchronously read a file from the storage service
     *
     * @param mailbox_that_should_receive_file_content: the mailbox to which the file should be sent
     * @param file: the file
     * @param src_partition: the partition in which the file will be read
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     * @throw std::invalid_arguments
     */
    void StorageService::initiateFileRead(std::string mailbox_that_should_receive_file_content, WorkflowFile *file,
                                          std::string src_partition) {

      WRENCH_INFO("Initiating a file read operation for file %s on storage service %s",
                  file->getID().c_str(), this->getName().c_str());

      if (file == nullptr) {
        throw std::invalid_argument("StorageService::initiateFileRead(): Invalid arguments");
      }

      assertServiceIsUp();

      // Empty partition means "/"
      if (src_partition.empty()) {
        src_partition = "/";
      }

      // Send a message to the daemon
      std::string request_answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("read_file");

      try {
        S4U_Mailbox::putMessage(this->mailbox_name,
                                new StorageServiceFileReadRequestMessage(request_answer_mailbox,
                                                                         mailbox_that_should_receive_file_content,
                                                                         file,
                                                                         src_partition,
                                                                         this->getMessagePayloadValue(
                                                                                 StorageServiceMessagePayload::FILE_READ_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }


      // Wait for a reply to the request
      std::unique_ptr<SimulationMessage> message = nullptr;

      try {
        message = S4U_Mailbox::getMessage(request_answer_mailbox, this->network_timeout);
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
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

    /**
     * @brief Get the total static capacity of the storage service (in zero simulation time)
     * @return capacity of the storage service (double)
     */
    double StorageService::getTotalSpace() {
      return this->capacity;
    }
};
