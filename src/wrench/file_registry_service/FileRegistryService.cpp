/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <compute_services/multicore_job_executor/MulticoreJobExecutor.h>
#include <logging/TerminalOutput.h>
#include <simgrid_S4U_util/S4U_Simulation.h>
#include <simgrid_S4U_util/S4U_Mailbox.h>

#include "FileRegistryService.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(file_registry_service, "Log category for File Registry Service");

namespace wrench {


    FileRegistryService::FileRegistryService(std::string hostname,
                                             std::map<FileRegistryService::Property, std::string> plist) :
            FileRegistryService(hostname, plist, "") {

    }


    FileRegistryService::FileRegistryService(
            std::string hostname,
            std::map<FileRegistryService::Property, std::string> plist,
            std::string suffix) :
            S4U_DaemonWithMailbox("file_registry_service" + suffix, "file_registry_service" + suffix) {

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


    /**
   * @brief Get a property name as a string
   * @return the name as a string
   *
   * @throw std::invalid_argument
   */
    std::string FileRegistryService::getPropertyString(FileRegistryService::Property property) {
      switch (property) {
        case STOP_DAEMON_MESSAGE_PAYLOAD: return "STOP_DAEMON_MESSAGE_PAYLOAD";
        case DAEMON_STOPPED_MESSAGE_PAYLOAD: return "DAEMON_STOPPED_MESSAGE_PAYLOAD";
        case REQUEST_MESSAGE_PAYLOAD: return "REQUEST_MESSAGE_PAYLOAD";
        case ANSWER_MESSAGE_PAYLOAD: return "ANSWER_MESSAGE_PAYLOAD";
        case REMOVE_ENTRY_PAYLOAD: return "REMOVE_ENTRY_PAYLOAD";
        case LOOKUP_OVERHEAD:return "LOOKUP_OVERHEAD";

        default:
          throw new std::invalid_argument(
                  "FileRegistryService property" + std::to_string(property) + "has no string name");
      }
    }

    /**
     * @brief Set a property of the FileRegistryService
     * @param property: the property
     * @param value: the property value
     */
    void FileRegistryService::setProperty(FileRegistryService::Property property, std::string value) {
      this->property_list[property] = value;
    }

    /**
     * @brief Get a property of the FileRegistryService as a string
     * @param property: the property
     * @return the property value as a string
     */
    std::string FileRegistryService::getPropertyValueAsString(FileRegistryService::Property property) {
      return this->property_list[property];
    }

    /**
     * @brief Get a property of the FileRegistryService as a double
     * @param property: the property
     * @return the property value as a double
     *
     * @throw std::runtime_error
     */
    double FileRegistryService::getPropertyValueAsDouble(FileRegistryService::Property property) {
      double value;
      if (sscanf(this->getPropertyValueAsString(property).c_str(), "%lf", &value) != 1) {
        throw std::runtime_error("Invalid " + this->getPropertyString(property) + " property value " +
                                 this->getPropertyValueAsString(property));
      }
      return value;
    }


    /**
     * @brief Stop the service
     *
     * @throw std::runtime_error
     */
    void FileRegistryService::stop() {

      this->state = FileRegistryService::DOWN;

      WRENCH_INFO("Telling the daemon listening on (%s) to terminate", this->mailbox_name.c_str());
      // Send a termination message to the daemon's mailbox - SYNCHRONOUSLY
      std::string ack_mailbox = this->mailbox_name + "_kill";
      S4U_Mailbox::put(this->mailbox_name,
                       new StopDaemonMessage(
                               ack_mailbox,
                               this->getPropertyValueAsDouble(STOP_DAEMON_MESSAGE_PAYLOAD)));
      // Wait for the ack
      std::unique_ptr<SimulationMessage> message = S4U_Mailbox::get(ack_mailbox);
      if (message->type != SimulationMessage::Type::DAEMON_STOPPED) {
        throw std::runtime_error("Wrong message type received while expecting DAEMON_STOPPED");
      }
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


    bool FileRegistryService::processNextMessage() {

      // Wait for a message
      std::unique_ptr<SimulationMessage> message = S4U_Mailbox::get(this->mailbox_name);


      WRENCH_INFO("Got a [%s] message", message->toString().c_str());

      switch (message->type) {

        case SimulationMessage::STOP_DAEMON: {
          std::unique_ptr<StopDaemonMessage> m(static_cast<StopDaemonMessage *>(message.release()));

          // This is Synchronous
          S4U_Mailbox::put(m->ack_mailbox,
                           new DaemonStoppedMessage(this->getPropertyValueAsDouble(DAEMON_STOPPED_MESSAGE_PAYLOAD)));
          return false;
        }

        default: {
          throw std::runtime_error("Unknown message type: " + std::to_string(message->type));
        }
      }
    }
};
