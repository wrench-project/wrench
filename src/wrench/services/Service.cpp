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

XBT_LOG_NEW_DEFAULT_CATEGORY(service, "Log category for Service");


namespace wrench {

    /**
     * @brief Constructor that mostly calls the S4U_DaemonWithMailbox() constructor
     *
     * @param process_name_prefix: the prefix for the process name
     * @param mailbox_name_prefix: the prefix for the mailbox name
     */
    Service::Service(std::string process_name_prefix, std::string mailbox_name_prefix) :
            S4U_DaemonWithMailbox(process_name_prefix, mailbox_name_prefix) {
    }

    /**
   * @brief Set a property of the Service
   * @param property: the property
   * @param value: the property value
   */
    void Service::setProperty(std::string property, std::string value) {
//      std::cerr << "SETTING [" << property << ", " << value << "]" << std::endl;
      this->property_list[property] = value;
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
        throw std::runtime_error("Cannot find value for property " + property +
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
        throw std::runtime_error("Invalid double property value " + property + " " +
                                 this->getPropertyValueAsString(property));
      }
      return value;
    }


    /**
    * @brief Stop the service
    *
    * @throw std::runtime_error
    */
    void Service::stop() {

      this->state = Service::DOWN;

      WRENCH_INFO("Telling the daemon listening on (%s) to terminate", this->mailbox_name.c_str());

      // Send a termination message to the daemon's mailbox - SYNCHRONOUSLY
      std::string ack_mailbox = this->mailbox_name + "_kill";
      double value;
      value = this->getPropertyValueAsDouble(ServiceProperty::STOP_DAEMON_MESSAGE_PAYLOAD);
      S4U_Mailbox::put(this->mailbox_name,
                       new StopDaemonMessage(
                               ack_mailbox,
                               this->getPropertyValueAsDouble(ServiceProperty::STOP_DAEMON_MESSAGE_PAYLOAD)));
      // Wait for the ack
      std::unique_ptr<SimulationMessage> message = S4U_Mailbox::get(ack_mailbox);
      if (message->type != SimulationMessage::Type::DAEMON_STOPPED) {
        throw std::runtime_error("Wrong message type received while expecting DAEMON_STOPPED");
      }
    }

    /**
   * @brief Get the name of the service
   * @return the compute service name
   */
    std::string Service::getName() {
      return this->name;
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

};