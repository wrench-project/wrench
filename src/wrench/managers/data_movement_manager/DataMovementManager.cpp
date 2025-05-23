/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/logging/TerminalOutput.h"
#include "wrench/simgrid_S4U_util/S4U_CommPort.h"
#include "wrench/simulation/SimulationMessage.h"
#include "wrench/services/ServiceMessage.h"
#include "wrench/services/storage/StorageService.h"
#include "wrench/services/file_registry/FileRegistryService.h"
#include "wrench/exceptions/ExecutionException.h"
#include "wrench/services/storage/StorageServiceMessage.h"
#include "wrench/data_file/DataFile.h"
#include "wrench/managers/data_movement_manager/DataMovementManager.h"
#include "wrench/failure_causes/FileAlreadyBeingCopied.h"
#include "wrench/failure_causes/FileAlreadyBeingRead.h"
#include "wrench/failure_causes/FileAlreadyBeingWritten.h"
#include "wrench/managers/data_movement_manager/FileReaderThread.h"
#include "wrench/managers/data_movement_manager/FileWriterThread.h"
#include "wrench/managers/data_movement_manager/DataMovementManagerMessage.h"

#include <memory>

WRENCH_LOG_CATEGORY(wrench_core_data_movement_manager, "Log category for Data Movement Manager");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param hostname: the hostname on which the data movement manager is to run
     * @param creator_commport: the commport of the manager's creator
     */
    DataMovementManager::DataMovementManager(const std::string& hostname, S4U_CommPort *creator_commport) : Service(hostname, "data_movement_manager") {
        this->creator_commport = creator_commport;
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
     */
    void DataMovementManager::stop() {
        this->commport->putMessage(new ServiceStopDaemonMessage(nullptr, false, ComputeService::TerminationCause::TERMINATION_NONE, 0.0));
    }

    /**
     * @brief Ask the data manager to initiate an asynchronous file copy
     * @param src: the source location
     * @param dst: the destination location
     * @param file_registry_service: a file registry service to update once the file copy has (successfully) completed (none if nullptr)
     *
     */
    void DataMovementManager::initiateAsynchronousFileCopy(const std::shared_ptr<FileLocation> &src,
                                                           const std::shared_ptr<FileLocation> &dst,
                                                           const std::shared_ptr<FileRegistryService> &file_registry_service) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((src == nullptr) || (dst == nullptr)) {
            throw std::invalid_argument("DataMovementManager::initiateAsynchronousFileCopy(): Invalid nullptr arguments");
        }
#endif
        if (src->getFile() != dst->getFile()) {
            throw std::invalid_argument("DataMovementManager::initiateAsynchronousFileCopy(): src and dst locations should be for the same file");
        }

        DataMovementManager::CopyRequestSpecs request(src, dst, file_registry_service);

        for (auto const &p: this->pending_file_copies) {
            if (*p == request) {
                throw ExecutionException(
                    std::make_shared<FileAlreadyBeingCopied>(src, dst));
            }
        }


        this->pending_file_copies.push_front(std::make_unique<CopyRequestSpecs>(src, dst, file_registry_service));
        wrench::StorageService::initiateFileCopy(this->commport, src, dst);
    }

    /**
     * @brief Ask the data manager to initiate an asynchronous file read
     * @param location: the location to read from
     *
     */
    void DataMovementManager::initiateAsynchronousFileRead(const std::shared_ptr<FileLocation> &location) {
        this->initiateAsynchronousFileRead(location, location->getFile()->getSize());
    }

    /**
     * @brief Ask the data manager to initiate an asynchronous file read
     * @param location: the location to read from
     * @param num_bytes: the number of bytes to read
     *
     */
    void DataMovementManager::initiateAsynchronousFileRead(const std::shared_ptr<FileLocation> &location,
                                                           const sg_size_t num_bytes) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((location == nullptr)) {
            throw std::invalid_argument("DataMovementManager::initiateAsynchronousFileRead(): Invalid nullptr arguments");
        }
#endif

        DataMovementManager::ReadRequestSpecs request(location, num_bytes);

        for (auto const &p: this->pending_file_reads) {
            if (*p == request) {
                throw ExecutionException(
                    std::make_shared<FileAlreadyBeingRead>(location));
            }
        }

        this->pending_file_reads.push_front(std::make_unique<ReadRequestSpecs>(location, num_bytes));
        // Initiate the read in a thread
        auto frt = std::make_shared<FileReaderThread>(this->hostname, this->commport, location, num_bytes);
        frt->setSimulation(this->simulation_);
        frt->start(frt, true, false);
    }

    /**
     * @brief Ask the data manager to initiate an asynchronous file write
     * @param location: the location to read from
     * @param file_registry_service: a file registry service to update once the file write has (successfully) completed
     *
     */
    void DataMovementManager::initiateAsynchronousFileWrite(const std::shared_ptr<FileLocation> &location,
                                                            const std::shared_ptr<FileRegistryService> &file_registry_service) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((location == nullptr)) {
            throw std::invalid_argument("DataMovementManager::initiateAsynchronousFileWrite(): Invalid nullptr arguments");
        }
#endif

        DataMovementManager::WriteRequestSpecs request(location, file_registry_service);

        for (auto const &p: this->pending_file_writes) {
            if (*p == request) {
                throw ExecutionException(
                    std::make_shared<FileAlreadyBeingWritten>(location));
            }
        }

        this->pending_file_writes.push_front(std::make_unique<WriteRequestSpecs>(location, file_registry_service));

        // Initiate the write operation in a thread
        auto fwt = std::make_shared<FileWriterThread>(this->hostname, this->commport, location);
        fwt->setSimulation(this->simulation_);
        fwt->start(fwt, true, false);
    }


    /**
     * @brief Ask the data manager to perform a synchronous file copy
     * @param src: the source location
     * @param dst: the destination location
     * @param file_registry_service: a file registry service to update once the file copy has (successfully) completed (none if nullptr)
     *
     */
    void DataMovementManager::doSynchronousFileCopy(const std::shared_ptr<FileLocation> &src,
                                                    const std::shared_ptr<FileLocation> &dst,
                                                    const std::shared_ptr<FileRegistryService> &file_registry_service) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((src == nullptr) || (dst == nullptr)) {
            throw std::invalid_argument("DataMovementManager::doSynchronousFileCopy(): Invalid nullptr arguments");
        }
#endif
        if (src->getFile() != dst->getFile()) {
            throw std::invalid_argument("DataMovementManager::doSynchronousFileCopy(): src and dst locations should be for the same file");
        }

        DataMovementManager::CopyRequestSpecs request(src, dst, file_registry_service);

        for (auto const &p: this->pending_file_copies) {
            if (*p == request) {
                throw ExecutionException(
                    std::make_shared<FileAlreadyBeingCopied>(src, dst));
            }
        }

        StorageService::copyFile(src, dst);

        if (file_registry_service) {
            file_registry_service->addEntry(dst);
        }
    }


    /**
     * @brief Main method of the data movement manager
     * @return 0 on successful termination, non-zero otherwise
     */
    int DataMovementManager::main() {
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_YELLOW);

        WRENCH_INFO("New Data Movement Manager starting (%s)", this->commport->get_cname());

        while (processNextMessage()) {}

        WRENCH_INFO("Data Movement Manager terminating");

        return 0;
    }

    /**
     * @brief Process the next message
     * @return true if the daemon should continue, false otherwise
     *
     */
    bool DataMovementManager::processNextMessage() {
        std::shared_ptr<SimulationMessage> message = nullptr;

        try {
            message = this->commport->getMessage();
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

            // Replay
            this->creator_commport->dputMessage(
                    new DataManagerFileCopyAnswerMessage(msg->src,
                                                         msg->dst,
                                                         msg->success,
                                                         std::move(msg->failure_cause)));
            return true;

        } else if (auto msg = std::dynamic_pointer_cast<DataMovementManagerFileReaderThreadMessage>(message)) {
            // Remove the record and find the File Registry Service, if any
            ReadRequestSpecs request(msg->location, msg->num_bytes);
            for (auto it = this->pending_file_reads.begin();
                 it != this->pending_file_reads.end();
                 ++it) {
                if (*(*it) == request) {
                    this->pending_file_reads.erase(it);// remove the entry
                    break;
                }
            }

            // Forward it back
            this->creator_commport->dputMessage(
                    new DataManagerFileReadAnswerMessage(msg->location,
                                                         msg->num_bytes,
                                                         msg->success,
                                                         std::move(msg->failure_cause)));
            return true;

        } else if (auto msg = std::dynamic_pointer_cast<DataMovementManagerFileWriterThreadMessage>(message)) {

            // Remove the record and find the File Registry Service, if any
            DataMovementManager::WriteRequestSpecs request(msg->location, 0);
            for (auto it = this->pending_file_writes.begin();
                 it != this->pending_file_writes.end();
                 ++it) {
                if (*(*it) == request) {
                    // find out the file registry service if any
                    request.file_registry_service = (*it)->file_registry_service;
                    this->pending_file_writes.erase(it);// remove the entry
                    break;
                }
            }

            if (request.file_registry_service) {
                //                WRENCH_INFO("Trying to do a register");
                try {
                    request.file_registry_service->addEntry(request.location);
                } catch (ExecutionException &e) {
                    //                    WRENCH_INFO("Oops, couldn't do it");
                    // don't throw, just keep file_registry_service_update to false
                }
            }

            // Forward it back
            this->creator_commport->dputMessage(
                    new DataManagerFileWriteAnswerMessage(msg->location,
                                                          msg->success,
                                                          std::move(msg->failure_cause)));
            return true;

        } else {
            throw std::runtime_error(
                    "DataMovementManager::waitForNextMessage(): Unexpected [" + message->getName() + "] message");
        }
    }


}// namespace wrench
