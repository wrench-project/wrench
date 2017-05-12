/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include "Service.h"

namespace wrench {

    /**
     * @brief Constructor that mostly calls the S4U_DaemonWithMailbox() constructor
     *
     * @param process_name_prefix: the prefix for the process name
     * @param mailbox_name_prefix: the prefix for the mailbox name
     */
    Service::Service(std::string process_name_prefix, std::string mailbox_name_prefix) : S4U_DaemonWithMailbox(process_name_prefix, mailbox_name_prefix) {

    }


    /**
   * @brief Set a property of the Service
   * @param property: the property as an integer
   * @param value: the property value
   */
    void Service::setProperty(int property, std::string value) {
      this->property_list[property] = value;
    }

    /**
     * @brief Get a property of the Service as a string
     * @param property: the property as an integer
     * @return the property value as a string
     */
    std::string Service::getPropertyValueAsString(int property) {
      return this->property_list[property];
    }

    /**
     * @brief Get a property of the Service as a double
     * @param property: the property
     * @return the property value as a double
     *
     * @throw std::runtime_error
     */
    double Service::getPropertyValueAsDouble(int property) {
      double value;
      if (sscanf(this->getPropertyValueAsString(property).c_str(), "%lf", &value) != 1) {
        throw std::runtime_error("Invalid double property value " +
                                 this->getPropertyValueAsString(property));
      }
      return value;
    }

};