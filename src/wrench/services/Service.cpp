/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <wrench/simulation/SimulationMessage.h>
#include "wrench/services/ServiceMessage.h"
#include "wrench/simgrid_S4U_util/S4U_Daemon.h"
#include "wrench/services/Service.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/exceptions/WorkflowExecutionException.h"
#include "wrench/services/ServiceMessagePayload.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(service, "Log category for Service");


namespace wrench {

    /**
     * @brief Constructor
     * @param hostname: the name of the host on which the service will run
     * @param process_name_prefix: the prefix for the process name
     * @param mailbox_name_prefix: the prefix for the mailbox name
     */
    Service::Service(std::string hostname, std::string process_name_prefix, std::string mailbox_name_prefix) :
            S4U_Daemon(hostname, process_name_prefix, mailbox_name_prefix) {
      this->name = process_name_prefix;
    }

    /**
      * @brief Set a property of the Service
      * @param property: the property
      * @param value: the property value
      */
    void Service::setProperty(std::string property, std::string value) {
      if (this->property_list.find(property) != this->property_list.end()) {
        this->property_list[property] = value;
      } else {
        this->property_list.insert(std::make_pair(property, value));
      }
    }

    /**
    * @brief Set a message payload of the Service
    * @param messagepayload: the message payload (which must a a string representation of a >=0 double)
    * @param value: the message payload value
    * @throw std::invalid_argument
    */
    void Service::setMessagePayload(std::string messagepayload, std::string value) {
      // Check that the value is a >=0 double
      double double_value;

      if ((sscanf(value.c_str(), "%lf", &double_value) != 1) || (double_value < 0)) {
        throw std::invalid_argument(
                "Service::setMessagePayload(): Invalid message payload value " + messagepayload + ": " +
                value);
      }

      if (this->messagepayload_list.find(messagepayload) != this->messagepayload_list.end()) {
        this->messagepayload_list[messagepayload] = value;
      } else {
        this->messagepayload_list.insert(std::make_pair(messagepayload, value));
      }
    }
    /**
     * @brief Get a property of the Service as a string
     * @param property: the property
     * @return the property value as a string
     *
     * @throw std::invalid_argument
     */
    std::string Service::getPropertyValueAsString(std::string property) {
      if (this->property_list.find(property) == this->property_list.end()) {
        throw std::invalid_argument("Service::getPropertyValueAsString(): Cannot find value for property " + property +
                                 " (perhaps a derived service class does not provide a default value?)");
      }
      return this->property_list[property];
    }

    /**
     * @brief Get a message payload of the Service as a string
     * @param message_payload: the message payload
     * @return the message payload value as a string
     *
     * @throw std::invalid_argument
     */
    std::string Service::getMessagePayloadValueAsString(std::string message_payload) {
      if (this->messagepayload_list.find(message_payload) == this->messagepayload_list.end()) {
        throw std::invalid_argument("Service::getMessagePayloadValueAsString(): Cannot find value for message_payload " + message_payload +
                                    " (perhaps a derived service class does not provide a default value?)");
      }
      return this->messagepayload_list[message_payload];
    }

    
    /**
     * @brief Get a property of the Service as a double
     * @param property: the property
     * @return the property value as a double
     *
     * @throw std::invalid_argument
     */
    double Service::getPropertyValueAsDouble(std::string property) {
      double value;
      std::string string_value;
      try {
        string_value = this->getPropertyValueAsString(property);
      } catch (std::invalid_argument &e) {
        throw;
      }
      if (sscanf(string_value.c_str(), "%lf", &value) != 1) {
        throw std::invalid_argument(
                "Service::getPropertyValueAsDouble(): Invalid double property value " + property + " " +
                this->getPropertyValueAsString(property));
      }
      return value;
    }

    /**
     * @brief Get a message payload of the Service as a double
     * @param message_payload: the message payload
     * @return the message payload value as a double
     *
     * @throw std::invalid_argument
     */
    double Service::getMessagePayloadValueAsDouble(std::string message_payload) {
      double value;
      std::string string_value;
      try {
        string_value = this->getMessagePayloadValueAsString(message_payload);
      } catch (std::invalid_argument &e) {
        throw;
      }
      if ((sscanf(string_value.c_str(), "%lf", &value) != 1) || (value < 0)) {
        throw std::invalid_argument(
                "Service::getMessagePayloadValueAsDouble(): Invalid double message payload value " + message_payload + " " +
                this->getMessagePayloadValueAsString(message_payload));
      }
      return value;
    }



    /**
     * @brief Get a property of the Service as a boolean
     * @param property: the property
     * @return the property value as a boolean
     *
     * @throw std::invalid_argument
     */
    bool Service::getPropertyValueAsBoolean(std::string property) {
      bool value;
      std::string string_value;
      try {
        string_value = this->getPropertyValueAsString(property);
      } catch (std::invalid_argument &e) {
        throw;
      }
      if (string_value == "true") {
        return true;
      } else if (string_value == "false") {
        return false;
      } else {
        throw std::invalid_argument(
                "Service::getPropertyValueAsBoolean(): Invalid boolean property value " + property + " " +
                this->getPropertyValueAsString(property));
      }
    }

    /**
     * @brief Start the service
     * @param this_service: a shared pointer to the service
     * @param daemonize: true if the daemon is to be daemonized, false otherwise
     * 
     * @throw std::runtime_error
     */
    void Service::start(std::shared_ptr<Service> this_service, bool daemonize) {
      try {
        this->state = Service::UP;
        this->createLifeSaver(this_service);
        this->startDaemon(daemonize);
      } catch (std::invalid_argument &e) {
        throw std::runtime_error("Service::start(): " + std::string(e.what()));
      }
    }

    /**
     * @brief Synchronously stop the service (does nothing if the service is already stopped)
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void Service::stop() {

      // Do nothing if the service is already down
      if (this->state == Service::DOWN) {
        return;
      }

      WRENCH_INFO("Telling the daemon listening on (%s) to terminate", this->mailbox_name.c_str());

      // Send a termination message to the daemon's mailbox_name - SYNCHRONOUSLY
      std::string ack_mailbox = S4U_Mailbox::generateUniqueMailboxName("stop");
      try {
        S4U_Mailbox::putMessage(this->mailbox_name,
                                new ServiceStopDaemonMessage(
                                        ack_mailbox,
                                        this->getMessagePayloadValueAsDouble(ServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }

      // Wait for the ack
      std::unique_ptr<SimulationMessage> message = nullptr;

      try {
        message = S4U_Mailbox::getMessage(ack_mailbox, this->network_timeout);
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }

      if (auto msg = dynamic_cast<ServiceDaemonStoppedMessage *>(message.get())) {
        this->state = Service::DOWN;
      } else {
        throw std::runtime_error("Service::stop(): Unexpected [" + message->getName() + "] message");
      }

      // Set the service state to down
      this->state = Service::DOWN;
    }

    /**
    * @brief Get the name of the host on which the service is / will be running
    * @return the hostname
    */
    std::string Service::getHostname() {
      return this->hostname;
    }

    /**
    * @brief Returns true if the service is UP, false otherwise
    * @return true or false
    */
    bool Service::isUp() {
      return (this->state == Service::UP);
    }

    /**
		 * @brief Set the state of the service to DOWN
		 */
    void Service::setStateToDown() {
      this->state = Service::DOWN;
    }

    /**
     * @brief Set default and user-defined properties
     * @param default_property_values: list of default properties
     * @param overridden_poperty_values: list of overridden properties (override the default)
     */
    void Service::setProperties(std::map<std::string, std::string> default_property_values,
                                std::map<std::string, std::string> overridden_poperty_values) {
      // Set default properties
      for (auto const &p : default_property_values) {
        this->setProperty(p.first, p.second);
      }

      // Set specified properties (possible overwriting default ones)
      for (auto const &p : overridden_poperty_values) {
        this->setProperty(p.first, p.second);
      }
    }

    /**
     * @brief Set default and user-defined message payloads
     * @param default_messagepayload_values: list of default message payloads
     * @param overridden_messagepayload_values: list of overridden message payloads (override the default)
     */
    void Service::setMessagePayloads(std::map<std::string, std::string> default_messagepayload_values,
                                std::map<std::string, std::string> overridden_messagepayload_values) {
      // Set default messagepayloads
      for (auto const &p : default_messagepayload_values) {
        this->setMessagePayload(p.first, p.second);
      }

      // Set specified messagepayloads (possible overwriting default ones)
      for (auto const &p : overridden_messagepayload_values) {
        this->setMessagePayload(p.first, p.second);
      }
    }

    /**
     * @brief Check whether the service is properly configured and running
     *
     * @throws WorkflowExecutionException
     */
    void Service::serviceSanityCheck() {
      if (this->state == Service::DOWN) {
        throw WorkflowExecutionException(std::shared_ptr<FailureCause>(new ServiceIsDown(this)));
      }
    }

    /**
     * @brief Returns the service's network timeout value
     * @return a duration in seconds
     */
    double Service::getNetworkTimeoutValue() {
      return this->network_timeout;
    }

    /**
     * @brief Sets the service's network timeout value
     * @param value: a duration in seconds (<0 means: never timeout)
     *
     */
    void Service::setNetworkTimeoutValue(double value) {
      this->network_timeout = value;
    }
};
