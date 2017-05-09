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
    * @brief Main method of the daemon
    *
    * @return 0 on termination
    */
    int FileRegistryService::main() {

      TerminalOutput::setThisProcessLoggingColor(WRENCH_LOGGING_COLOR_MAGENTA);

      WRENCH_INFO("File Registry Service starting on host %s!", S4U_Simulation::getHostName().c_str());

      WRENCH_INFO("File Registry Service on host %s terminated!", S4U_Simulation::getHostName().c_str());
      return 0;
    }
};