/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench-dev.h>
#include <simgrid_S4U_util/S4U_Mailbox.h>
#include "SimpleStorageService.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(simple_storage_service, "Log category for Simple Storage Service");


namespace wrench {


    /**
     * @brief Public constructor
     * @param hostname: the name of the host on which to start the service
     * @param capacity: the storage capacity in bytes
     * @param plist: the optional property list
     */
    SimpleStorageService::SimpleStorageService(std::string hostname,
                                               double capacity,
                                             std::map<SimpleStorageService::Property, std::string> plist) :
            SimpleStorageService(hostname, capacity, plist, "") {

    }


    /**
     * @brief Private constructor
     * @param hostname: the name of the host on which to start the service
     * @param capacity: the storage capacity in bytes
     * @param plist: the property list
     * @param suffix: the suffix (for the service name)
     */
    SimpleStorageService::SimpleStorageService(
            std::string hostname,
            double capacity,
            std::map<SimpleStorageService::Property, std::string> plist,
            std::string suffix) :
            StorageService("simple_storage_service", capacity),
            S4U_DaemonWithMailbox("simple_storage_service" + suffix, "simple_storage_service" + suffix) {

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
    std::string SimpleStorageService::getPropertyString(SimpleStorageService::Property property) {
      switch (property) {
        case STOP_DAEMON_MESSAGE_PAYLOAD: return "STOP_DAEMON_MESSAGE_PAYLOAD";
        case DAEMON_STOPPED_MESSAGE_PAYLOAD: return "DAEMON_STOPPED_MESSAGE_PAYLOAD";

        default:
          throw new std::invalid_argument(
                  "SimpleStorageService property" + std::to_string(property) + "has no string name");
      }
    }

    /**
     * @brief Set a property of the SimpleStorageService
     * @param property: the property
     * @param value: the property value
     */
    void SimpleStorageService::setProperty(SimpleStorageService::Property property, std::string value) {
      this->property_list[property] = value;
    }

    /**
     * @brief Get a property of the SimpleStorageService as a string
     * @param property: the property
     * @return the property value as a string
     */
    std::string SimpleStorageService::getPropertyValueAsString(SimpleStorageService::Property property) {
      return this->property_list[property];
    }

    /**
     * @brief Get a property of the SimpleStorageService as a double
     * @param property: the property
     * @return the property value as a double
     *
     * @throw std::runtime_error
     */
    double SimpleStorageService::getPropertyValueAsDouble(SimpleStorageService::Property property) {
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
    void SimpleStorageService::stop() {

      this->state = SimpleStorageService::DOWN;

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

    void SimpleStorageService::copyFile(WorkflowFile *file, StorageService *src) {
      // Send a request to the daemon


    }

    void SimpleStorageService::downloadFile(WorkflowFile *file) {

    }

    void SimpleStorageService::uploadFile(WorkflowFile *file) {

    }

    void SimpleStorageService::deleteFile(WorkflowFile *file) {

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

      }

      WRENCH_INFO("Simple Storage Service on host %s terminated!", S4U_Simulation::getHostName().c_str());
      return 0;
    }


    /**
     * @brief Helper function to process incoming messages
     * @return false if the daemon should terminate after processing this message
     */
    bool SimpleStorageService::processNextMessage() {

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
