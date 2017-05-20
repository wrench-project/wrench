/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include "StorageService.h"
#include "simulation/Simulation.h"
#include "StorageServiceProperty.h"
#include "StorageServiceMessage.h"
#include <logging/TerminalOutput.h>
#include <simgrid_S4U_util/S4U_Mailbox.h>
#include <simulation/SimulationMessage.h>
#include <exceptions/WorkflowExecutionException.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(storage_service, "Log category for Storage Service");


namespace wrench {

    /**
   * @brief Constructor
   *
   * @param service_name: the name of the storage service
   */
    StorageService::StorageService(std::string service_name, std::string mailbox_name_prefix, double capacity) :
            Service(service_name, mailbox_name_prefix) {
      this->capacity = capacity;
      this->simulation = nullptr; // will be filled in via Simulation::add()
      this->state = StorageService::UP;
    }

    /**
     * @brief Internal method to add a file to the storage in a StorageService
     *
     * @param file: a raw pointer to a WorkflowFile object
     *
     * @throw std::runtime_error
     */
    void StorageService::addFileToStorage(WorkflowFile *file) {

      if (file->getSize() > (this->capacity - this->occupied_space)) {
        throw std::runtime_error("File exceeds free space capacity on storage service");
      }
      this->stored_files.insert(file);
      this->occupied_space += file->getSize();
      WRENCH_INFO("Stored file %s (storage usage: %.10lf%%)", file->getId().c_str(),
               100.0 * this->occupied_space / this->capacity);
    }

    /**
     * @brief Internal method to delete a file from the storage  in a StorageService
     *
     * @param file: a raw pointer to a WorkflowFile object
     *
     * @throw std::runtime_error
     */
    void StorageService::removeFileFromStorage(WorkflowFile *file) {

      if (this->stored_files.find(file) == this->stored_files.end()) {
        throw std::runtime_error("Attempting to remove a file that's not on the storage service");
      }
      this->stored_files.erase(file);
      this->occupied_space -= file->getSize();
      XBT_INFO("Deleted file %s (storage usage: %.2lf%%)", file->getId().c_str(),
               100.0 * this->occupied_space / this->capacity);
    }

    /**
     * @brief Stop the compute service - should be called by the stop()
     *        method of derived classes, if any
     */
    void StorageService::stop() {
      // Notify the simulation that the service is terminated, if that
      // service was registered with the simulation
      if (this->simulation) {
        this->simulation->mark_storage_service_as_terminated(this);
      }

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
     * @throw std::runtime_error
     */
    double StorageService::howMuchFreeSpace() {
      if (this->state == DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      // Send a message to the daemon
      std::string answer_mailbox = S4U_Mailbox::getPrivateMailboxName();
      S4U_Mailbox::put(this->mailbox_name, new StorageServiceFreeSpaceRequestMessage(
              answer_mailbox,
              this->getPropertyValueAsDouble(StorageServiceProperty::FREE_SPACE_REQUEST_MESSAGE_PAYLOAD)));

      // Wait to the message back
      std::unique_ptr<SimulationMessage> message = S4U_Mailbox::get(answer_mailbox);
      if (StorageServiceFreeSpaceAnswerMessage *msg = dynamic_cast<StorageServiceFreeSpaceAnswerMessage*>(message.get())) {
        return msg->free_space;
      } else {
        throw std::runtime_error("StorageService::howMuchFreeSpace(): unexpected [" + msg->getName() + "] message");
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
     */
    bool StorageService::lookupFile(WorkflowFile *file) {
      if (this->state == DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      // Send a message to the daemon
      std::string answer_mailbox = S4U_Mailbox::getPrivateMailboxName();
      S4U_Mailbox::put(this->mailbox_name, new StorageServiceFileLookupRequestMessage(
              answer_mailbox,
              file,
              this->getPropertyValueAsDouble(StorageServiceProperty::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD)));

      // Wait to the message back
      std::unique_ptr<SimulationMessage> message = S4U_Mailbox::get(answer_mailbox);
      if (StorageServiceFileLookupAnswerMessage *msg = dynamic_cast<StorageServiceFileLookupAnswerMessage*>(message.get())) {
        return msg->file_is_available;
      } else {
        throw std::runtime_error("StorageService::lookupFile(): unexpected [" + msg->getName() + "] message");
      }

    }

    /**
     * @brief Synchronously read a file from the storage service
     *
     * @param file: the file
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void StorageService::readFile(WorkflowFile *file) {

      if (this->state == DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      // Send a synchronous message to the daemon
      std::string answer_mailbox = S4U_Mailbox::getPrivateMailboxName();
      S4U_Mailbox::put(this->mailbox_name,
                       new StorageServiceFileReadRequestMessage(answer_mailbox,
                                                      file,
                                                      this->getPropertyValueAsDouble(
                                                              StorageServiceProperty::FILE_READ_REQUEST_MESSAGE_PAYLOAD)));

      // Wait for the answer
      std::unique_ptr<SimulationMessage> message = S4U_Mailbox::get(answer_mailbox);
      if (StorageServiceFileReadAnswerMessage *msg = dynamic_cast<StorageServiceFileReadAnswerMessage*>(message.get())) {
        // If it's not a success, throw an exception
        if (!msg->success) {
          throw WorkflowExecutionException(msg->failure_cause);
        }

        // Otherwise, get the file
        std::unique_ptr<SimulationMessage> file_content_message = S4U_Mailbox::get(answer_mailbox);
        if (StorageServiceFileContentMessage *file_content_msg = dynamic_cast<StorageServiceFileContentMessage*>(file_content_message.get())) {
          // do nothing
        } else {
          throw std::runtime_error("StorageService::readFile(): Received an unexpected ["+
                                   file_content_message->getName() +"] message!");
        }

      } else {
        throw std::runtime_error("StorageService::readFile(): Received an unexpected ["+
                                 message->getName() +"] message!");
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

      if (this->state == DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      // Send a synchronous message to the daemon
      std::string answer_mailbox = S4U_Mailbox::getPrivateMailboxName();
      S4U_Mailbox::put(this->mailbox_name,
                       new StorageServiceFileWriteRequestMessage(answer_mailbox,
                                                      file,
                                                      this->getPropertyValueAsDouble(
                                                              StorageServiceProperty::FILE_WRITE_REQUEST_MESSAGE_PAYLOAD)));

      // Wait for the answer
      std::unique_ptr<SimulationMessage> message = S4U_Mailbox::get(answer_mailbox);

      if (StorageServiceFileWriteAnswerMessage *msg = dynamic_cast<StorageServiceFileWriteAnswerMessage*>(message.get())) {
        // If not a success, throw an exception
        if (!msg->success) {
          throw WorkflowExecutionException(msg->failure_cause);
        }

        // Otherwise, synchronously send the file up!
        S4U_Mailbox::put(msg->data_write_mailbox_name, new StorageServiceFileContentMessage(file));

      } else {
        throw std::runtime_error("StorageService::writeFile(): Received an unexpected ["+
                                 message->getName() +"] message!");
      }

      return;
    }

    /**
    * @brief Synchronously and sequentially read a set of files from storage services
    * @param files: the set of files to read
    * @param file_locations: a map of files to storage services
    * @param default_storage_service: the storage service to use when files don't appear in the file_locations map
    * @return nullptr on success, a pointer to a workflow execution failure cause on failure
    *
    * @throw std::runtime_error
    * @throw WorkflowExecutionException
    */
    void StorageService::readFiles(std::set<WorkflowFile *> files,
                                       std::map<WorkflowFile *, StorageService *> file_locations,
                                       StorageService *default_storage_service) {
      try {
        StorageService::writeOrReadFiles(READ, files, file_locations, default_storage_service);
      } catch (std::runtime_error &e) {
        throw;
      } catch (WorkflowExecutionException &e) {
        throw;
      }
    }

    /**
   * @brief Synchronously and sequentially uppload a set of files from storage services
   * @param files: the set of files to write
   * @param file_locations: a map of files to storage services
   * @param default_storage_service: the storage service to use when files don't appear in the file_locations map
   * @return nullptr on success, a pointer to a workflow execution failure cause on failure
   *
   *  @throw std::runtime_error
   * @throw WorkflowExecutionException
   */
    void StorageService::writeFiles(std::set<WorkflowFile *> files,
                                     std::map<WorkflowFile *, StorageService *> file_locations,
                                     StorageService *default_storage_service) {
      try {
        StorageService::writeOrReadFiles(WRITE, files, file_locations, default_storage_service);
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
     * @return nullptr on success, a pointer to a workflow execution failure cause on failure
     *
     * @throw std::runtime_error
     * @throw WorkflowExecutionException
     */
    void StorageService::writeOrReadFiles(Action action,
                                               std::set<WorkflowFile *> files,
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
        if (action == READ) {
          try {
            storage_service->readFile(f);
          } catch (std::runtime_error &e) {
            throw;
          } catch (WorkflowExecutionException &e) {
            throw;
          }
        } else {
          // Write the file
          try {
            storage_service->writeFile(f);
          } catch (std::runtime_error &e) {
            throw;
          } catch (WorkflowExecutionException &e) {
            throw;
          }
        }
      }

      return;
    }

    /**
     * @brief Synchronously asks the storage service to delete a file copy
     *
     * @param file: the file
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void StorageService::deleteFile(WorkflowFile *file) {
      if (this->state == DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      // Send a message to the daemon
      std::string answer_mailbox = S4U_Mailbox::getPrivateMailboxName();
      S4U_Mailbox::put(this->mailbox_name, new StorageServiceFileDeleteRequestMessage(
              answer_mailbox,
              file,
              this->getPropertyValueAsDouble(StorageServiceProperty::FILE_DELETE_REQUEST_MESSAGE_PAYLOAD)));

      // Wait to the message back
      std::unique_ptr<SimulationMessage> message = S4U_Mailbox::get(answer_mailbox);

      if (StorageServiceFileDeleteAnswerMessage *msg = dynamic_cast<StorageServiceFileDeleteAnswerMessage*>(message.get())) {
        // On failure, throw an exception
        if (!msg->success) {
          throw WorkflowExecutionException(msg->failure_cause);
        }
        XBT_INFO("Deleted file %s on storage service %s", file->getId().c_str(), this->getName().c_str());
      } else {
        throw std::runtime_error("StorageService::deleteFile(): unexpected [" + message->getName() + "] message");
      }

      return;
    }

    /**
     * @brief Synchronously and sequentially delete a set of files from storage services
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
     * @param file: the file to copy
     * @param src: the storage service from which to read the file
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void StorageService::copyFile(WorkflowFile *file, StorageService *src) {
      if (this->state == DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      // Send a message to the daemon
      std::string answer_mailbox = S4U_Mailbox::getPrivateMailboxName();
      S4U_Mailbox::put(this->mailbox_name, new StorageServiceFileCopyRequestMessage(
              answer_mailbox,
              file,
              src,
              this->getPropertyValueAsDouble(StorageServiceProperty::FILE_COPY_REQUEST_MESSAGE_PAYLOAD)));

      // Wait to the message back
      std::unique_ptr<SimulationMessage> message = S4U_Mailbox::get(answer_mailbox);
      if (StorageServiceFileCopyAnswerMessage *msg = dynamic_cast<StorageServiceFileCopyAnswerMessage*>(message.get())) {
        if (msg->failure_cause) {
          throw WorkflowExecutionException(msg->failure_cause);
        }
      } else {
        throw std::runtime_error("StorageService::copyFile(): unexpected [" + msg->getName() + "] message");
      }

      return;
    }
};