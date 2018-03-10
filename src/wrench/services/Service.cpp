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
#include "wrench/services/ServiceProperty.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(service, "Log category for Service");


namespace wrench {

    /**
     * @brief Constructor that mostly calls the S4U_DaemonWithMailbox() constructor
     *
     * @param hostname: the name of the host on which the service runs
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
     * @brief Get a property of the Service as a string
     * @param property: the property
     * @return the property value as a string
     *
     * @throw std::runtime_error
     */
    std::string Service::getPropertyValueAsString(std::string property) {
      if (this->property_list.find(property) == this->property_list.end()) {
        throw std::runtime_error("Service::getPropertyValueAsString(): Cannot find value for property " + property +
                                 " (perhaps a derived service class does not provide a default value?)");
      }
      return this->property_list[property];
    }

    /**
     * @brief Get a property of the Service as a double
     * @param property: the property
     * @return the property value as a double
     *
     * @throw std::runtime_error
     */
    double Service::getPropertyValueAsDouble(std::string property) {
      double value;
      if (sscanf(this->getPropertyValueAsString(property).c_str(), "%lf", &value) != 1) {
        throw std::runtime_error(
                "Service::getPropertyValueAsDouble(): Invalid double property value " + property + " " +
                this->getPropertyValueAsString(property));
      }
      return value;
    }

    /**
     * @brief Start the service
     * @param this_service: a shared pointer to this service object
     * @param daemonize: true if the daemon is to be truly daemonized, false otherwise
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
                                        this->getPropertyValueAsDouble(ServiceProperty::STOP_DAEMON_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }

      // Wait for the ack
      std::unique_ptr<SimulationMessage> message = nullptr;

      try {
        message = S4U_Mailbox::getMessage(ack_mailbox);
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
    * @brief Get the name of the host on which the service is running
    * @return the hostname
    */
    std::string Service::getHostname() {
      return this->hostname;
    }

    /**
    * @brief Find out whether the service is UP
    * @return true if the service is UP, false otherwise
    */
    bool Service::isUp() {
      return (this->state == Service::UP);
    }

    /**
		 * @brief Set the state of the compute service to DOWN
		 */
    void Service::setStateToDown() {
      this->state = Service::DOWN;
    }

    /**
     * @brief Set the simulation
     * @param simulation: a simulation
     */
    void Service::setSimulation(Simulation *simulation) {
      this->simulation = simulation;
    }

    /**
     * @brief Set default and user defined properties
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
     * @brief Check whether the service is properly configured and running
     *
     * @throws WorkflowExecutionException
     */
    void Service::serviceSanityCheck() {
      if (this->state == Service::DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }
    }
};
