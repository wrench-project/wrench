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
#include <services/storage_services/StorageService.h>
#include <exceptions/WorkflowExecutionException.h>
#include <services/storage_services/StorageServiceMessage.h>
#include <workflow/WorkflowFile.h>
#include <workflow/Workflow.h>
#include "DataMovementManager.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(data_movement_manager, "Log category for Data Movement Manager");

namespace wrench {

    /**
     * @brief Constructor, which starts a DataMovementManager daemon
     *
     * @param workflow: the workflow whose data (files) are to be managed
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
     *
     * throw WorkflowExecutionException
     * throw std::runtime_error
     */
    void DataMovementManager::stop() {
      try {
        S4U_Mailbox::putMessage(this->mailbox_name, new ServiceStopDaemonMessage("", 0.0));
      } catch (std::runtime_error &e) {
        if (!strcmp(e.what(), "network_error")) {
          throw WorkflowExecutionException(new NetworkError());
        } else {
          throw std::runtime_error("DataMovementManager::stop(): Unknown exception: " + std::string(e.what()));
        }
      }
    }

    /**
     * @brief Ask the data manager to initiate an asynchronous file copy
     * @param file: the file
     * @param src: the source data storage
     * @param dst: the destination data storage
     *
     * @throw std::invalid_argument
     * @throw WorkflowExecutionException
     */
    void DataMovementManager::initiateAsynchronousFileCopy(WorkflowFile *file,
                                             StorageService *src,
                                             StorageService *dst) {
      if ((file == nullptr) || (src == nullptr) || (dst == nullptr)) {
        throw std::invalid_argument("DataMovementManager::initiateFileCopy(): Invalid arguments");
      }

      try {
        dst->initiateFileCopy(this->mailbox_name, file, src);
      } catch (WorkflowExecutionException &e) {
        throw;
      }
    }

    /**
     * @brief Ask the data manager to perform a synchronous file copy
     * @param file: the file
     * @param src: the source data storage
     * @param dst: the destination data storage
     *
     * @throw std::invalid_argument
     * @throw WorkflowExecutionException
     */
    void DataMovementManager::doSynchronousFileCopy(WorkflowFile *file,
                                             StorageService *src,
                                             StorageService *dst) {
      if ((file == nullptr) || (src == nullptr) || (dst == nullptr)) {
        throw std::invalid_argument("DataMovementManager::initiateFileCopy(): Invalid arguments");
      }

      try {
        dst->copyFile(file, src);
      } catch (WorkflowExecutionException &e) {
        throw;
      }
    }



    /**
     * @brief Main method of the daemon that implements the DataMovementManager
     * @return 0 in success
     */
    int DataMovementManager::main() {

      TerminalOutput::setThisProcessLoggingColor(WRENCH_LOGGING_COLOR_YELLOW);

      WRENCH_INFO("New Data Movement Manager starting (%s)", this->mailbox_name.c_str());

      while (processNextMessage()) {

        // Clear finished asynchronous dputMessage()
        S4U_Mailbox::clear_dputs();

      }

      WRENCH_INFO("Data Movement Manager terminating");

      return 0;
    }

    /**
     * @brief Process the next message
     * @return true if the daemon should continue, false otherwise
     *
     * @throw std::runtime_error
     */
    bool DataMovementManager::processNextMessage() {

      std::unique_ptr<SimulationMessage> message = nullptr;

      try {
        message = S4U_Mailbox::getMessage(this->mailbox_name);
      } catch (std::runtime_error &e) {
        return true;
      }

      if (message == nullptr) {
        WRENCH_INFO("Got a NULL message... Likely this means we're all done. Aborting!");
        return false;
      }

      WRENCH_INFO("Data Movement Manager got a %s message", message->getName().c_str());

      if (ServiceStopDaemonMessage *msg = dynamic_cast<ServiceStopDaemonMessage *>(message.get())) {
        // There shouldn't be any need to clean any state up
        return false;

      } else if (StorageServiceFileCopyAnswerMessage *msg = dynamic_cast<StorageServiceFileCopyAnswerMessage *>(message.get())) {

        // Forward it back
        try {
          S4U_Mailbox::dputMessage(msg->file->getWorkflow()->getCallbackMailbox(),
                                   new StorageServiceFileCopyAnswerMessage(msg->file,
                                                                           msg->storage_service, msg->success,
                                                                           msg->failure_cause, 0));
        } catch  (std::runtime_error &e) {
          return true;
        }
        return true;

      } else {
        throw std::runtime_error("DataMovementManager::processNextMessage(): Unexpected [" + message->getName() + "] message");
      }

      return false;
    }


};