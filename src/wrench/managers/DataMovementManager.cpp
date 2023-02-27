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
#include <wrench/exceptions/ExecutionException.h>
#include <wrench/services/storage/StorageServiceMessage.h>
#include <wrench/data_file/DataFile.h>
#include <wrench/managers/DataMovementManager.h>
#include <wrench/failure_causes/FileAlreadyBeingCopied.h>

#include <memory>

WRENCH_LOG_CATEGORY(wrench_core_data_movement_manager, "Log category for Data Movement Manager");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param hostname: the hostname on which the data movement manager is to run
     * @param creator_mailbox: the mailbox of the manager's creator
     */
    DataMovementManager::DataMovementManager(std::string hostname, simgrid::s4u::Mailbox *creator_mailbox) : Service(hostname, "data_movement_manager") {
        this->creator_mailbox = creator_mailbox;
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
     * @throw ExecutionException
     * @throw std::runtime_error
     */
    void DataMovementManager::stop() {
        S4U_Mailbox::putMessage(this->mailbox, new ServiceStopDaemonMessage(nullptr, false, ComputeService::TerminationCause::TERMINATION_NONE, 0.0));
    }

    /**
     * @brief Ask the data manager to initiate an asynchronous file copy
     * @param src: the source location
     * @param dst: the destination location
     * @param file_registry_service: a file registry service to update once the file copy has (successfully) completed (none if nullptr)
     *
     * @throw std::invalid_argument
     * @throw ExecutionException
     */
    void DataMovementManager::initiateAsynchronousFileCopy(const std::shared_ptr<FileLocation> &src,
                                                           const std::shared_ptr<FileLocation> &dst,
                                                           const std::shared_ptr<FileRegistryService> &file_registry_service) {
        if ((src == nullptr) || (dst == nullptr)) {
            throw std::invalid_argument("DataMovementManager::initiateAsynchronousFileCopy(): Invalid nullptr arguments");
        }
        if (src->getFile() != dst->getFile()) {
            throw std::invalid_argument("DataMovementManager::initiateAsynchronousFileCopy(): src and dst locations should be for the same file");
        }

        DataMovementManager::CopyRequestSpecs request(src, dst, file_registry_service);

        try {
            for (auto const &p: this->pending_file_copies) {
                if (*p == request) {
                    throw ExecutionException(
                            std::shared_ptr<FailureCause>(new FileAlreadyBeingCopied(src->getFile(), src, dst)));
                }
            }
        } catch (ExecutionException &e) {
            throw;
        }


        try {
            this->pending_file_copies.push_front(std::make_unique<CopyRequestSpecs>(src, dst, file_registry_service));
            wrench::StorageService::initiateFileCopy(this->mailbox, src, dst);
        } catch (ExecutionException &e) {
            throw;
        }
    }

    /**
     * @brief Ask the data manager to perform a synchronous file copy
     * @param src: the source location
     * @param dst: the destination location
     * @param file_registry_service: a file registry service to update once the file copy has (successfully) completed (none if nullptr)
     *
     * @throw std::invalid_argument
     * @throw ExecutionException
     */
    void DataMovementManager::doSynchronousFileCopy(const std::shared_ptr<FileLocation> &src,
                                                    const std::shared_ptr<FileLocation> &dst,
                                                    const std::shared_ptr<FileRegistryService> &file_registry_service) {
        if ((src == nullptr) || (dst == nullptr)) {
            throw std::invalid_argument("DataMovementManager::doSynchronousFileCopy(): Invalid nullptr arguments");
        }
        if (src->getFile() != dst->getFile()) {
            throw std::invalid_argument("DataMovementManager::doSynchronousFileCopy(): src and dst locations should be for the same file");
        }

        DataMovementManager::CopyRequestSpecs request(src, dst, file_registry_service);

        try {
            for (auto const &p: this->pending_file_copies) {
                if (*p == request) {
                    throw ExecutionException(
                            std::shared_ptr<FailureCause>(new FileAlreadyBeingCopied(src->getFile(), src, dst)));
                }
            }

            StorageService::copyFile(src, dst);
        } catch (ExecutionException &e) {
            throw;
        }

        try {
            if (file_registry_service) {
                file_registry_service->addEntry(dst);
            }
        } catch (ExecutionException &e) {
            throw;
        }
    }


    /**
     * @brief Main method of the data movement manager
     * @return 0 on successful termination, non-zero otherwise
     */
    int DataMovementManager::main() {
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_YELLOW);

        WRENCH_INFO("New Data Movement Manager starting (%s)", this->mailbox->get_cname());

        while (processNextMessage()) {}

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
            message = S4U_Mailbox::getMessage(this->mailbox);
        } catch (ExecutionException &e) {
            WRENCH_INFO("Oops... somebody tried to send a message, but that failed...");
            return true;
        }

        WRENCH_INFO("Data Movement Manager got a %s message", message->getName().c_str());

        if (std::dynamic_pointer_cast<ServiceStopDaemonMessage>(message)) {
            // There shouldn't be any need to clean any state up
            return false;

        } else if (auto msg = std::dynamic_pointer_cast<StorageServiceFileCopyAnswerMessage>(message)) {
            // Remove the record and find the File Registry Service, if any
            DataMovementManager::CopyRequestSpecs request(msg->src, msg->dst, nullptr);
            msg->src->getStorageService();
            request.src->getStorageService();
            msg->dst->getStorageService();
            request.dst->getStorageService();
            for (auto it = this->pending_file_copies.begin();
                 it != this->pending_file_copies.end();
                 ++it) {
                request.src->getStorageService();
                request.dst->getStorageService();
                (*(*it)).src->getStorageService();
                (*(*it)).dst->getStorageService();
                if (*(*it) == request) {
                    request.file_registry_service = (*it)->file_registry_service;
                    this->pending_file_copies.erase(it);// remove the entry
                    break;
                }
            }

            if (request.file_registry_service) {
                WRENCH_INFO("Trying to do a register");
                try {
                    request.file_registry_service->addEntry(request.dst);
                } catch (ExecutionException &e) {
                    WRENCH_INFO("Oops, couldn't do it");
                    // don't throw, just keep file_registry_service_update to false
                }
            }

            // Forward it back
            S4U_Mailbox::dputMessage(this->creator_mailbox,
                                     new StorageServiceFileCopyAnswerMessage(msg->src,
                                                                             msg->dst,
                                                                             msg->success,
                                                                             std::move(msg->failure_cause),
                                                                             0));
            return true;

        } else {
            throw std::runtime_error(
                    "DataMovementManager::waitForNextMessage(): Unexpected [" + message->getName() + "] message");
        }
    }

    //    /** @brief Get the mailbox of the service that created this data movement manager
    //     *
    //     * @return a mailbox
    //     */
    //    simgrid::s4u::Mailbox *DataMovementManager::getCreatorMailbox() {
    //        return this->creator_mailbox;
    //    }


}// namespace wrench
