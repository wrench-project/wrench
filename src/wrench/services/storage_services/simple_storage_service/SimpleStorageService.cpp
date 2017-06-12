/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench-dev.h"
#include "services/ServiceMessage.h"
#include "services/storage_services/StorageServiceMessage.h"
#include "simgrid_S4U_util/S4U_Mailbox.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(simple_storage_service, "Log category for Simple Storage Service");


namespace wrench {

    /**
     * @brief Public constructor
     *
     * @param hostname: the name of the host on which to start the service
     * @param capacity: the storage capacity in bytes
     * @param plist: the optional property list
     */
    SimpleStorageService::SimpleStorageService(std::string hostname,
                                               double capacity,
                                               std::map<std::string, std::string> plist) :
            SimpleStorageService(hostname, capacity, plist, "") {}

    /**
     * @brief Private constructor
     *
     * @param hostname: the name of the host on which to start the service
     * @param capacity: the storage capacity in bytes
     * @param plist: the property list
     * @param suffix: the suffix (for the service name)
     *
     * @throw std::invalid_argument
     */
    SimpleStorageService::SimpleStorageService(
            std::string hostname,
            double capacity,
            std::map<std::string, std::string> plist,
            std::string suffix) :
            capacity(capacity),
            StorageService("simple_storage_service" + suffix, "simple_storage_service" + suffix, capacity) {

      if (capacity < 0) {
        throw std::invalid_argument("SimpleStorageService::SimpleStorageService(): Invalid argument");
      }

      // Set default properties
      for (auto p : this->default_property_values) {
        this->setProperty(p.first, p.second);
      }

      // Set specified properties
      for (auto p : plist) {
        this->setProperty(p.first, p.second);
      }

      // Set the name of the data mailbox
      this->data_write_mailbox_name = S4U_Mailbox::generateUniqueMailboxName(
              "simple_storage_service" + suffix + "_data_");

      // Start the daemon on the same host
      try {
        this->start(hostname);
      } catch (std::invalid_argument &e) {
        throw;
      }
    }

    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int SimpleStorageService::main() {

      TerminalOutput::setThisProcessLoggingColor(WRENCH_LOGGING_COLOR_CYAN);

      WRENCH_INFO("Simple Storage Service starting on host %s!", S4U_Simulation::getHostName().c_str());

      /** Main loop **/
      while (this->processNextMessage()) {
        // Clear pending asynchronous puts that are done
        S4U_Mailbox::clear_dputs();

        // Clear pending asynchronous puts that are done
        S4U_Mailbox::clear_dputs();
      }

      WRENCH_INFO("Simple Storage Service on host %s terminated!", S4U_Simulation::getHostName().c_str());
      return 0;
    }

    /**
     * @brief Helper function to process incoming messages
     * @return false if the daemon should terminate after processing this message
     */
    bool SimpleStorageService::processNextMessage() {

      std::unique_ptr<SimulationMessage> message;

      // Wait for a message
      try {
        message = S4U_Mailbox::getMessage(this->mailbox_name);
      } catch (std::runtime_error &e) {
        if (!strcmp(e.what(), "network_error")) {
          WRENCH_INFO("Giving up on this message...");
          return true;
        } else {
          throw std::runtime_error("SimpleStorageService::processNextMessage(): Got an unknown exception while receiving a message.");
        }
      }

      if (message == nullptr) {
        WRENCH_INFO("Got a NULL message. This likely means that we're all done...Aborting!");
        return false;
      }

      WRENCH_INFO("Got a [%s] message", message->getName().c_str());

      if (ServiceStopDaemonMessage *msg = dynamic_cast<ServiceStopDaemonMessage *>(message.get())) {
        S4U_Mailbox::putMessage(msg->ack_mailbox,
                                new ServiceDaemonStoppedMessage(this->getPropertyValueAsDouble(
                                        SimpleStorageServiceProperty::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
        return false;

      } else if (StorageServiceFreeSpaceRequestMessage *msg = dynamic_cast<StorageServiceFreeSpaceRequestMessage *>(message.get())) {
        double free_space = this->capacity - this->occupied_space;

        S4U_Mailbox::putMessage(msg->answer_mailbox,
                                new StorageServiceFreeSpaceAnswerMessage(free_space, this->getPropertyValueAsDouble(
                                        SimpleStorageServiceProperty::FREE_SPACE_ANSWER_MESSAGE_PAYLOAD)));
        return true;

      } else if (StorageServiceFileDeleteRequestMessage *msg = dynamic_cast<StorageServiceFileDeleteRequestMessage *>(message.get())) {

        bool success = true;
        WorkflowExecutionFailureCause *failure_cause = nullptr;
        if (this->stored_files.find(msg->file) == this->stored_files.end()) {
          success = false;
          failure_cause = new FileNotFound(msg->file, this);
        } else {
          this->removeFileFromStorage(msg->file);
        }

        S4U_Mailbox::putMessage(msg->answer_mailbox,
                                new StorageServiceFileDeleteAnswerMessage(msg->file,
                                                                          this,
                                                                          success,
                                                                          failure_cause,
                                                                          this->getPropertyValueAsDouble(
                                                                                  SimpleStorageServiceProperty::FILE_DELETE_ANSWER_MESSAGE_PAYLOAD)));

        return true;

      } else if (StorageServiceFileLookupRequestMessage *msg = dynamic_cast<StorageServiceFileLookupRequestMessage *>(message.get())) {

        bool file_found = (this->stored_files.find(msg->file) != this->stored_files.end());
        S4U_Mailbox::putMessage(msg->answer_mailbox,
                                new StorageServiceFileLookupAnswerMessage(msg->file, file_found,
                                                                          this->getPropertyValueAsDouble(
                                                                                  SimpleStorageServiceProperty::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD)));

        return true;

      } else if (StorageServiceFileWriteRequestMessage *msg = dynamic_cast<StorageServiceFileWriteRequestMessage *>(message.get())) {

        if (msg->file->getSize() > (this->capacity - this->occupied_space)) {
          std::cerr << " NO SPACE " << msg->file->getSize() << " " << this->capacity << " " << this->occupied_space
                    << std::endl;
          S4U_Mailbox::putMessage(msg->answer_mailbox,
                                  new StorageServiceFileWriteAnswerMessage(msg->file,
                                                                           this,
                                                                           false,
                                                                           new StorageServiceFull(msg->file, this),
                                                                           "",
                                                                           this->getPropertyValueAsDouble(
                                                                                   SimpleStorageServiceProperty::FILE_WRITE_ANSWER_MESSAGE_PAYLOAD)));
        } else {
          this->occupied_space += msg->file->getSize();
          S4U_Mailbox::putMessage(msg->answer_mailbox,
                                  new StorageServiceFileWriteAnswerMessage(msg->file,
                                                                           this,
                                                                           true,
                                                                           nullptr,
                                                                           this->data_write_mailbox_name,
                                                                           this->getPropertyValueAsDouble(
                                                                                   SimpleStorageServiceProperty::FILE_WRITE_ANSWER_MESSAGE_PAYLOAD)));

          // TODO: THIS REALLY CANNOT BE SYNCHRONOUS, BUT UNTIL WAIT_FOR_ANY WORKS IT WILL HAVE TO DO
          std::unique_ptr<SimulationMessage> file_content_message = nullptr;
          try {
            file_content_message = S4U_Mailbox::getMessage(this->data_write_mailbox_name);
          } catch (std::runtime_error &e) {
            if (!strcmp(e.what(), "network_error")) {
              WRENCH_INFO("Giving up on this message...");
              return true;
            } else {
              throw std::runtime_error("SimpleStorageService::processNextMessage(): Got an unknown exception while receiving a file content.");
            }
          }

          if (StorageServiceFileContentMessage *file_content_msg = dynamic_cast<StorageServiceFileContentMessage *>(file_content_message.get())) {
            this->addFileToStorage(file_content_msg->file);
          } else {
            throw std::runtime_error("SimpleStorageService::processNextMessage(): Unexpected [ " + msg->getName() + "] message");
          }
        }
        return true;

      } else if (StorageServiceFileReadRequestMessage *msg = dynamic_cast<StorageServiceFileReadRequestMessage *>(message.get())) {
        // Figure out whether this succeeds or not
        bool success = true;
        WorkflowExecutionFailureCause *failure_cause = nullptr;
        if (this->stored_files.find(msg->file) == this->stored_files.end()) {
          success = false;
          failure_cause = new FileNotFound(msg->file, this);
        }

        // Send back the corresponding ack
        try {
          S4U_Mailbox::putMessage(msg->answer_mailbox,
                                  new StorageServiceFileReadAnswerMessage(msg->file, this, success, failure_cause,
                                                                          this->getPropertyValueAsDouble(
                                                                                  SimpleStorageServiceProperty::FILE_READ_ANSWER_MESSAGE_PAYLOAD)));
        } catch (std::runtime_error &e) {
          return true;
        }

        // If success, then follow up with sending the file (ASYNCHRONOUSLY!)
        if (success) {
          try {
            S4U_Mailbox::dputMessage(msg->answer_mailbox, new StorageServiceFileContentMessage(msg->file));
          } catch (std::runtime_error &e) {
            return true;
          }
        }
        return true;

      } else if (StorageServiceFileCopyRequestMessage *msg = dynamic_cast<StorageServiceFileCopyRequestMessage *>(message.get())) {


        // Figure out whether this succeeds or not
        if (msg->file->getSize() > this->capacity - this->occupied_space) {
          try {
            S4U_Mailbox::putMessage(msg->answer_mailbox,
                                    new StorageServiceFileCopyAnswerMessage(msg->file, this, false,
                                                                            new StorageServiceFull(msg->file, this),
                                                                            this->getPropertyValueAsDouble(
                                                                                    SimpleStorageServiceProperty::FILE_COPY_ANSWER_MESSAGE_PAYLOAD)));
          } catch (std::runtime_error &e) {
            return true;
          }
        }

        // Get the file from the source
        // TODO: THIS SHOULDN'T BE SYNCHRONOUS!!!!!
        try {
          msg->src->readFile(msg->file);
        } catch (WorkflowExecutionException &e) {
          try {
            S4U_Mailbox::putMessage(msg->answer_mailbox,
                                    new StorageServiceFileCopyAnswerMessage(msg->file, this, false, e.getCause(),
                                                                            this->getPropertyValueAsDouble(
                                                                                    SimpleStorageServiceProperty::FILE_COPY_ANSWER_MESSAGE_PAYLOAD)));
          } catch (std::runtime_error &e) {
            return true;
          }
        }

        // Send back the corresponding ack
        try {
          S4U_Mailbox::putMessage(msg->answer_mailbox,
                                  new StorageServiceFileCopyAnswerMessage(msg->file, this, true, nullptr,
                                                                          this->getPropertyValueAsDouble(
                                                                                  SimpleStorageServiceProperty::FILE_COPY_ANSWER_MESSAGE_PAYLOAD)));
        } catch (std::runtime_error &e) {
          return true;
        }

        return true;

      } else {
        throw std::runtime_error("SimpleStorageService::processNextMessage(): Unexpected [" + message->getName() + "] message");
      }
    }

};
