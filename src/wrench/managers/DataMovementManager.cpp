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
#include <services/storage/StorageServiceMessage.h>
#include <wrench/data_file/DataFile.h>
#include <wrench/managers/DataMovementManager.h>
#include  <wrench/failure_causes/FileAlreadyBeingCopied.h>
#include  <wrench/failure_causes/NetworkError.h>

#include <memory>

WRENCH_LOG_CATEGORY(wrench_core_data_movement_manager, "Log category for Data Movement Manager");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param hostname: the hostname on which the data movement manager is to run
     * @param creator_mailbox: the mailbox of the manager's creator
     */
    DataMovementManager::DataMovementManager(std::string hostname, simgrid::s4u::Mailbox *creator_mailbox) :
            Service(hostname, "data_movement_manager") {

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
        try {
            S4U_Mailbox::putMessage(this->mailbox, new ServiceStopDaemonMessage(nullptr, false, ComputeService::TerminationCause::TERMINATION_NONE, 0.0));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw ExecutionException(cause);
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
     * @throw ExecutionException
     */
    void DataMovementManager::initiateAsynchronousFileCopy(std::shared_ptr<DataFile>file,
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
                    throw ExecutionException(
                            std::shared_ptr<FailureCause>(new FileAlreadyBeingCopied(file, src, dst)));
                }
            }
        } catch (ExecutionException &e) {
            throw;
        }


        std::cerr << "PENDING\n";
        std::cerr << "REF: " << src.use_count() << "\n";
        try {
            this->pending_file_copies.push_front(std::make_unique<CopyRequestSpecs>(file, src, dst, file_registry_service));
        std::cerr << "REF: " << src.use_count() << "\n";
            wrench::StorageService::initiateFileCopy(this->mailbox, file,src, dst);
        std::cerr << "REF: " << src.use_count() << "\n";
        } catch (ExecutionException &e) {
            throw;
        }
        std::cerr << "AFTER PENDING\n";
    }

    /**
     * @brief Ask the data manager to perform a synchronous file copy
     * @param file: the file to copy
     * @param src: the source location
     * @param dst: the destination location
     * @param file_registry_service: a file registry service to update once the file copy has (successfully) completed (none if nullptr)
     *
     * @throw std::invalid_argument
     * @throw ExecutionException
     */
    void DataMovementManager::doSynchronousFileCopy(std::shared_ptr<DataFile>file,
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
                    throw ExecutionException(
                            std::shared_ptr<FailureCause>(new FileAlreadyBeingCopied(file, src, dst)));
                }
            }

            StorageService::copyFile(file, src, dst);
        } catch (ExecutionException &e) {
            throw;
        }

        try {
            if (file_registry_service) {
                file_registry_service->addEntry(file, dst);
            }
        } catch (ExecutionException &e) {
            throw;
        }
    }


/**
 * @brief Main method of the daemon that implements the DataMovementManager
 * @return 0 on success
 */
    int DataMovementManager::main() {

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_YELLOW);

        WRENCH_INFO("New Data Movement Manager starting (%s)", this->mailbox->get_cname());

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

        std::unique_ptr<SimulationMessage> message = nullptr;

        try {
            message = S4U_Mailbox::getMessage(this->mailbox);
        } catch (std::shared_ptr<NetworkError> &cause) {
            WRENCH_INFO("Oops... somebody tried to send a message, but that failed...");
            return true;
        }

        WRENCH_INFO("Data Movement Manager got a %s message", message->getName().c_str());

        if (auto msg = dynamic_cast<ServiceStopDaemonMessage*>(message.get())) {
            // There shouldn't be any need to clean any state up
            return false;

        } else if (auto msg = dynamic_cast<StorageServiceFileCopyAnswerMessage*>(message.get())) {

            // Remove the record and find the File Registry Service, if any
            DataMovementManager::CopyRequestSpecs request(msg->file, msg->src, msg->dst, nullptr);
            std::cerr << "msg->src " << msg->src.use_count() << "\n";
            std::cerr << "msg->dst " << msg->dst.use_count() << "\n";
            std::cerr << "msg->src " << request.src.use_count() << "\n";
            std::cerr << "msg->dst " << request.dst.use_count() << "\n";
            msg->src->getStorageService();
            request.src->getStorageService();
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
                    std::cerr << "ERASING PENDING FILE COPY\n";
                    this->pending_file_copies.erase(it); // remove the entry
                    std::cerr << "ERASED PENDING FILE COPY\n";
                    break;
                }
            }

            bool file_registry_service_updated = false;
            if (request.file_registry_service) {
                WRENCH_INFO("Trying to do a register");
                try {
                    request.file_registry_service->addEntry(request.file, request.dst);
                    file_registry_service_updated = true;
                } catch (ExecutionException &e) {
                    WRENCH_INFO("Oops, couldn't do it");
                    // don't throw, just keep file_registry_service_update to false
                }
            }

            WRENCH_INFO("Forwarding status message");
            // Forward it back
            S4U_Mailbox::dputMessage(this->creator_mailbox,
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

    /** @brief Get the mailbox of the service that created this data movement manager
     *
     * @return a mailbox
     */
    simgrid::s4u::Mailbox *DataMovementManager::getCreatorMailbox() {
        return this->creator_mailbox;
    }


};
