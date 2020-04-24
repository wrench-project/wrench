/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/logging/TerminalOutput.h>
#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <wrench/simulation/SimulationMessage.h>
#include <wrench/services/ServiceMessage.h>
#include <wrench/services/storage/StorageService.h>
#include <wrench/services/file_registry/FileRegistryService.h>
#include <wrench/exceptions/WorkflowExecutionException.h>
#include <services/storage/StorageServiceMessage.h>
#include <wrench/workflow/WorkflowFile.h>
#include <wrench/wms/WMS.h>
#include <wrench/managers/DataMovementManager.h>
#include  <wrench/workflow/failure_causes/FileAlreadyBeingCopied.h>
#include  <wrench/workflow/failure_causes/NetworkError.h>

WRENCH_LOG_CATEGORY(wrench_core_data_movement_manager, "Log category for Data Movement Manager");

namespace wrench {


    /**
     * @brief Constructor
     *
     * @param wms: the WMS that uses this data movement manager
     */
    DataMovementManager::DataMovementManager(std::shared_ptr<WMS> wms) :
            Service(wms->hostname, "data_movement_manager", "data_movement_manager") {

        this->wms = wms;


    }

    /**
     * @brief Kill the manager (brutally terminate the daemon)
     */
    void DataMovementManager::kill() {
        this->killActor();
    }

    /**
     * @brief Stop the manager
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void DataMovementManager::stop() {
        try {
            S4U_Mailbox::putMessage(this->mailbox_name, new ServiceStopDaemonMessage("", 0.0));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }
    }

    /**
     * @brief Ask the data manager to initiate an asynchronous file copy
     * @param file: the file to copy
     * @param src: the source location
     * @param dst: the destination location
     * @param file_registry_service: a file registry service to update once the file copy has (successfully) completed (none if nullptr)
     *
     * @throw std::invalid_argument
     * @throw WorkflowExecutionException
     */
    void DataMovementManager::initiateAsynchronousFileCopy(WorkflowFile *file,
                                                           std::shared_ptr<FileLocation> src,
                                                           std::shared_ptr<FileLocation> dst,
                                                           std::shared_ptr<FileRegistryService> file_registry_service) {
        if ((file == nullptr) || (src == nullptr) || (dst == nullptr)) {
            throw std::invalid_argument("DataMovementManager::initiateFileCopy(): Invalid arguments");
        }

        DataMovementManager::CopyRequestSpecs request(file, src, dst, file_registry_service);

        try {
            for (auto const &p : this->pending_file_copies) {
                if (*p == request) {
                    throw WorkflowExecutionException(
                            std::shared_ptr<FailureCause>(new FileAlreadyBeingCopied(file, src, dst)));
                }
            }
        } catch (WorkflowExecutionException &e) {
            throw;
        }


        try {
            this->pending_file_copies.push_front(std::unique_ptr<CopyRequestSpecs>(
                    new CopyRequestSpecs(file, src, dst, file_registry_service)));
            wrench::StorageService::initiateFileCopy(this->mailbox_name, file,
                                                       src, dst);
        } catch (WorkflowExecutionException &e) {
            throw;
        }
    }

    /**
     * @brief Ask the data manager to perform a synchronous file copy
     * @param file: the file to copy
     * @param src: the source location
     * @param dst: the destination location
     * @param file_registry_service: a file registry service to update once the file copy has (successfully) completed (none if nullptr)
     *
     * @throw std::invalid_argument
     * @throw WorkflowExecutionException
     */
    void DataMovementManager::doSynchronousFileCopy(WorkflowFile *file,
                                                    std::shared_ptr<FileLocation> src,
                                                    std::shared_ptr<FileLocation> dst,
                                                    std::shared_ptr<FileRegistryService> file_registry_service) {
        if ((file == nullptr) || (src == nullptr) || (dst == nullptr)) {
            throw std::invalid_argument("DataMovementManager::initiateFileCopy(): Invalid arguments");
        }

        DataMovementManager::CopyRequestSpecs request(file, src, dst, file_registry_service);

        try {
            for (auto const &p : this->pending_file_copies) {
                if (*p == request) {
                    throw WorkflowExecutionException(
                            std::shared_ptr<FailureCause>(new FileAlreadyBeingCopied(file, src, dst)));
                }
            }

            StorageService::copyFile(file, src, dst);
        } catch (WorkflowExecutionException &e) {
            throw;
        }

        try {
            if (file_registry_service) {
                file_registry_service->addEntry(file, dst);
            }
        } catch (WorkflowExecutionException &e) {
            throw;
        }
    }


/**
 * @brief Main method of the daemon that implements the DataMovementManager
 * @return 0 on success
 */
    int DataMovementManager::main() {

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_YELLOW);

        WRENCH_INFO("New Data Movement Manager starting (%s)", this->mailbox_name.c_str());

        while (processNextMessage()) {


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

        std::shared_ptr<SimulationMessage> message = nullptr;

        try {
            message = S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<NetworkError> &cause) {
            WRENCH_INFO("Oops... somebody tried to send a message, but that failed...");
            return true;
        }

        WRENCH_INFO("Data Movement Manager got a %s message", message->getName().c_str());

        if (auto msg = std::dynamic_pointer_cast<ServiceStopDaemonMessage>(message)) {
            // There shouldn't be any need to clean any state up
            return false;

        } else if (auto msg = std::dynamic_pointer_cast<StorageServiceFileCopyAnswerMessage>(message)) {

            // Remove the record and find the File Registry Service, if any
            DataMovementManager::CopyRequestSpecs request(msg->file, msg->src, msg->dst, nullptr);
            for (auto it = this->pending_file_copies.begin();
                 it != this->pending_file_copies.end();
                 ++it) {
                if (*(*it) == request) {
                    request.file_registry_service = (*it)->file_registry_service;
                    this->pending_file_copies.erase(it); // remove the entry
                    break;
                }
            }

            bool file_registry_service_updated = false;
            if (request.file_registry_service) {
                WRENCH_INFO("Trying to do a register");
                try {
                    request.file_registry_service->addEntry(request.file, request.dst);
                    file_registry_service_updated = true;
                } catch (WorkflowExecutionException &e) {
                    WRENCH_INFO("Oops, couldn't do it");
                    // don't throw, just keep file_registry_service_update to false
                }
            }

            WRENCH_INFO("Forwarding status message");
            // Forward it back
            S4U_Mailbox::dputMessage(msg->file->getWorkflow()->getCallbackMailbox(),
                                     new StorageServiceFileCopyAnswerMessage(msg->file,
                                                                             msg->src,
                                                                             msg->dst,
                                                                             request.file_registry_service,
                                                                             file_registry_service_updated,
                                                                             msg->success,
                                                                             std::move(msg->failure_cause),
                                                                             0
                                     ));
            return true;

        } else {
            throw std::runtime_error(
                    "DataMovementManager::waitForNextMessage(): Unexpected [" + message->getName() + "] message");
        }

    }


};
