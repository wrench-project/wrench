/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <simgrid_S4U_util/S4U_Simulation.h>
#include <logging/TerminalOutput.h>
#include <simgrid_S4U_util/S4U_Mailbox.h>
#include <simulation/SimulationMessage.h>
#include <services/ServiceMessage.h>
#include "DataMovementManager.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(data_movement_manager, "Log category for Data Movement Manager");

namespace wrench {

    /**
     * @brief Constructor, which starts a DataMovementManager daemon
     *
     * @param workflow: a pointer to the Workflow whose data is to be managed
     */
    DataMovementManager::DataMovementManager(Workflow *workflow) :
            S4U_DaemonWithMailbox("data_movement_manager", "data_movement_manager") {

      this->workflow = workflow;

      // Start the daemon
      std::string localhost = S4U_Simulation::getHostName();
      this->start(localhost);
    }

    /**
     * @brief Destructor, which kills the daemon
     */
    DataMovementManager::~DataMovementManager() {
      this->kill();
    }

    /**
     * @brief Kill the manager (brutally terminate the daemon)
     */
    void DataMovementManager::kill() {
      this->kill_actor();
    }

    /**
     * @brief Stop the manager
     */
    void DataMovementManager::stop() {
      S4U_Mailbox::put(this->mailbox_name, new ServiceStopDaemonMessage("", 0.0));
    }


    /**
     * @brief Main method of the daemon that implements the DataMovementManager
     * @return 0 in success
     */
    int DataMovementManager::main() {

      TerminalOutput::setThisProcessLoggingColor(WRENCH_LOGGING_COLOR_YELLOW);

      WRENCH_INFO("New Data Movement Manager starting (%s)", this->mailbox_name.c_str());

      bool keep_going = true;
      while (keep_going) {
        std::unique_ptr<SimulationMessage> message = S4U_Mailbox::get(this->mailbox_name);

        // Clear finished asynchronous dput()
        S4U_Mailbox::clear_dputs();

        WRENCH_INFO("Data Movement Manager got a %s message", message->getName().c_str());

        if (ServiceStopDaemonMessage *msg = dynamic_cast<ServiceStopDaemonMessage *>(message.get())) {
          // There shouldn't be any need to clean any state up
          keep_going = false;
          break;
        } else {
          throw std::runtime_error("Unexpected ["+message->getName() + "] message");
        }
      }

      WRENCH_INFO("Data Movement Manager terminating");

      return 0;
    }

    /**
     * @brief Ask the data manager to initiate a file copy
     * @param file: the file
     * @param src: the source data storage
     * @param dst: the destination data storage
     *
     * @throw std::invalid_argument
     */
    void DataMovementManager::submitFileCopy(WorkflowFile *file,
                                             StorageService *src,
                                             StorageService *dst) {
      if ((file == nullptr) || (src == nullptr) || (dst == nullptr)) {
        throw std::invalid_argument("DataMovementManager::initiateFileCopy(): invalid arguments");
      }

    }

};