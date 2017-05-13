/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <services/compute_services/multicore_job_executor/MulticoreJobExecutor.h>
#include <logging/TerminalOutput.h>
#include <simgrid_S4U_util/S4U_Simulation.h>
#include <simgrid_S4U_util/S4U_Mailbox.h>

#include "FileRegistryService.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(file_registry_service, "Log category for File Registry Service");

namespace wrench {


    FileRegistryService::FileRegistryService(std::string hostname,
                                             std::map<std::string, std::string> plist) :
            FileRegistryService(hostname, plist, "") {

    }


    FileRegistryService::FileRegistryService(
            std::string hostname,
            std::map<std::string, std::string> plist,
            std::string suffix) :
            Service("file_registry_service" + suffix, "file_registry_service" + suffix) {

      // Set default properties
      for (auto p : this->default_property_values) {
        this->setProperty(p.first, p.second);
      }

      // Set specified properties
      for (auto p : plist) {
        this->setProperty(p.first, p.second);
      }

      this->hostname = hostname;

      // Start the daemon on the same host
      try {
        this->start(hostname);
      } catch (std::invalid_argument e) {
        throw e;
      }
    }


//

    /**
     * @brief Notify the FileRegistryService that a WorkflowFile is available at some StorageService
     * @param file: a raw pointer to a WorkflowFile
     * @param ss: a raw pointer to a StorageService
     *
     * @throw std::invalid_argument
     */
    void FileRegistryService::addEntry(WorkflowFile *file, StorageService *ss) {
      if ((file == nullptr) || (ss == nullptr)) {
        throw std::invalid_argument("Invalid input argument");
      }
      if (this->entries.find(file) == this->entries.end()) {
        this->entries[file] = {};
      }
      this->entries[file].insert(ss);
    }

    /**
     * @brief Remove an entry from a FileRegistryService
     * @param file: a raw pointer to a WorkflowFile
     * @param ss: a raw pointer to a StorageService
     *
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    void FileRegistryService::removeEntry(WorkflowFile *file, StorageService *ss) {
      if ((file == nullptr) || (ss == nullptr)) {
        throw std::invalid_argument("Invalid input argument");
      }
      if (this->entries.find(file) == this->entries.end()) {
        throw std::runtime_error("Trying to remove entry for non-existent file in FileRegistryService");
      }
      if (this->entries[file].find(ss) == this->entries[file].end()) {
        throw std::runtime_error("Trying to remove non-existing entry in FileRegistryService");
      }
      this->entries[file].erase(ss);
    }


    /**
     * @brief Remove all entries for a particular WorkflowFile in a FileRegistryService
     *
     * @param file: a raw pointer to a WorkflowFile
     *
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    void FileRegistryService::removeAllEntries(WorkflowFile *file) {
      if (file == nullptr) {
        throw std::invalid_argument("Invalid input argument");
      }
      if (this->entries.find(file) == this->entries.end()) {
        throw std::runtime_error("Trying to remove all entries for non-existent file in FileRegistryService");
      }
      this->entries[file].clear();
    }


    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int FileRegistryService::main() {

      TerminalOutput::setThisProcessLoggingColor(WRENCH_LOGGING_COLOR_MAGENTA);

      WRENCH_INFO("File Registry Service starting on host %s!", S4U_Simulation::getHostName().c_str());

      /** Main loop **/
      while (this->processNextMessage()) {

        // Clear pending asynchronous puts that are done
        S4U_Mailbox::clear_dputs();

      }

      WRENCH_INFO("File Registry Service on host %s terminated!", S4U_Simulation::getHostName().c_str());
      return 0;
    }


    /**
     * @brief Helper function to process incoming messages
     * @return false if the daemon should terminate after processing this message
     */
    bool FileRegistryService::processNextMessage() {

      // Wait for a message
      std::unique_ptr<SimulationMessage> message = S4U_Mailbox::get(this->mailbox_name);


      WRENCH_INFO("Got a [%s] message", message->toString().c_str());

      switch (message->type) {

        case SimulationMessage::STOP_DAEMON: {
          std::unique_ptr<StopDaemonMessage> m(static_cast<StopDaemonMessage *>(message.release()));

          // This is Synchronous
          S4U_Mailbox::put(m->ack_mailbox,
                           new DaemonStoppedMessage(this->getPropertyValueAsDouble(FileRegistryServiceProperty::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
          return false;
        }

        default: {
          throw std::runtime_error("Unknown message type: " + std::to_string(message->type));
        }
      }
    }


};
