/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <memory>
#include <utility>
#include <wrench/services/storage/storage_helpers/FileTransferThreadMessage.h>
#include <wrench/failure_causes/InvalidDirectoryPath.h>
#include <wrench/failure_causes/FileNotFound.h>
#include <wrench/failure_causes/StorageServiceNotEnoughSpace.h>

#include <wrench/services/storage/simple/SimpleStorageServiceBufferized.h>
#include <wrench/services/ServiceMessage.h>
#include "wrench/services/storage/StorageServiceMessage.h"
#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/simgrid_S4U_util/S4U_Simulation.h>
#include <wrench/data_file/DataFile.h>
#include <wrench/exceptions/ExecutionException.h>
#include <wrench/simulation/SimulationTimestampTypes.h>
#include <wrench/services/storage/storage_helpers/FileLocation.h>
#include <wrench/services/memory/MemoryManager.h>

WRENCH_LOG_CATEGORY(wrench_core_simple_storage_service_bufferized,
                    "Log category for Simple Storage Service Bufferized");

namespace wrench {

    /**
     * @brief Cleanup method
     *
     * @param has_returned_from_main: whether main() returned
     * @param return_value: the return value (if main() returned)
     */
    void SimpleStorageServiceBufferized::cleanup(bool has_returned_from_main, int return_value) {
        this->pending_file_transfer_threads.clear();
        this->running_file_transfer_threads.clear();
        // Do nothing. It's fine to die, and we'll just autorestart with our previous state
    }

    /**
     * @brief Public constructor
     *
     * @param hostname: the name of the host on which to start the service
     * @param mount_points: the set of mount points
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     */
    SimpleStorageServiceBufferized::SimpleStorageServiceBufferized(const std::string &hostname,
                                                                   std::set<std::string> mount_points,
                                                                   WRENCH_PROPERTY_COLLECTION_TYPE property_list,
                                                                   WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list) : SimpleStorageService(hostname, std::move(mount_points), std::move(property_list), std::move(messagepayload_list), "_" + std::to_string(getNewUniqueNumber())) {
        this->buffer_size = this->getPropertyValueAsSizeInByte(StorageServiceProperty::BUFFER_SIZE);
        this->is_bufferized = true;
    }

    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int SimpleStorageServiceBufferized::main() {
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_CYAN);

        std::string message = "Simple Storage Service (Bufferized) " + this->getName() + "  starting on host " + this->getHostname();
        WRENCH_INFO("%s", message.c_str());
        for (auto const &fs: this->file_systems) {
            message = "  - mount point " + fs.first + ": " +
                      std::to_string(fs.second->getFreeSpace()) + "/" +
                      std::to_string(fs.second->getTotalCapacity()) + " Bytes";
            WRENCH_INFO("%s", message.c_str());
        }

#ifdef PAGE_CACHE_SIMULATION
        // If writeback device simulation is activated
        if (Simulation::isPageCachingEnabled()) {
            //  Find the "memory" disk (we know there is one)
            auto host = S4U_Simulation::get_host_or_vm_by_name(this->getHostname());
            simgrid::s4u::Disk *memory_disk = nullptr;
            for (auto const &d: host->get_disks()) {
                // Get the disk's mount point
                const char *p = d->get_property("mount");
                if (!p) {
                    continue;
                }
                if (!strcmp(p, "/memory")) {
                    memory_disk = d;
                    break;
                }
            }
            // Start periodical flushing via a memory manager
            this->memory_manager = MemoryManager::initAndStart(this->simulation, memory_disk, 0.4, 5, 30, this->hostname);
            //            memory_manager_ptr->log();
        }
#endif

        /** Main loop **/
        while (this->processNextMessage()) {
            this->startPendingFileTransferThread();
        }

        WRENCH_INFO("Simple Storage Service %s on host %s cleanly terminating!",
                    this->getName().c_str(),
                    S4U_Simulation::getHostName().c_str());

        return 0;
    }

    /**
     * @brief Process a received control message
     *
     * @return false if the daemon should terminate
     */
    bool SimpleStorageServiceBufferized::processNextMessage() {
        S4U_Simulation::computeZeroFlop();

        // Wait for a message
        std::shared_ptr<SimulationMessage> message = nullptr;

        try {
            message = S4U_Mailbox::getMessage(this->mailbox);
        } catch (ExecutionException &e) {
            WRENCH_INFO("Got a network error while getting some message... ignoring");
            return true;
        }

        WRENCH_DEBUG("Got a [%s] message", message->getName().c_str());

        if (auto msg = std::dynamic_pointer_cast<ServiceStopDaemonMessage>(message)) {
            return processStopDaemonRequest(msg->ack_mailbox);

        } else if (auto msg = std::dynamic_pointer_cast<StorageServiceFreeSpaceRequestMessage>(message)) {
            return processFreeSpaceRequest(msg->answer_mailbox, msg->path);

        } else if (auto msg = std::dynamic_pointer_cast<StorageServiceFileDeleteRequestMessage>(message)) {
            return processFileDeleteRequest(msg->location, msg->answer_mailbox);

        } else if (auto msg = std::dynamic_pointer_cast<StorageServiceFileLookupRequestMessage>(message)) {
            return processFileLookupRequest(msg->location, msg->answer_mailbox);

        } else if (auto msg = std::dynamic_pointer_cast<StorageServiceFileWriteRequestMessage>(message)) {
            return processFileWriteRequest(msg->location, msg->num_bytes_to_write, msg->answer_mailbox);

        } else if (auto msg = std::dynamic_pointer_cast<StorageServiceFileReadRequestMessage>(message)) {
            return processFileReadRequest(msg->location, msg->num_bytes_to_read, msg->answer_mailbox);

        } else if (auto msg = std::dynamic_pointer_cast<StorageServiceFileCopyRequestMessage>(message)) {
            return processFileCopyRequest(msg->src, msg->dst, msg->answer_mailbox);

        } else if (auto msg = std::dynamic_pointer_cast<FileTransferThreadNotificationMessage>(message)) {
            return processFileTransferThreadNotification(
                    msg->file_transfer_thread,
                    msg->src_mailbox,
                    msg->src_location,
                    msg->dst_mailbox,
                    msg->dst_location,
                    msg->success,
                    msg->failure_cause,
                    msg->answer_mailbox_if_read,
                    msg->answer_mailbox_if_write,
                    msg->answer_mailbox_if_copy);
        } else {
            throw std::runtime_error(
                    "SimpleStorageServiceBufferized::processNextMessage(): Unexpected [" + message->getName() + "] message");
        }
    }

    /**
     * @brief Handle a file write request
     *
     * @param location: the location to write the file to
     * @param num_bytes_to_write: the number of bytes to write to the file
     * @param answer_mailbox: the mailbox to which the reply should be sent
     * @return true if this process should keep running
     */
    bool SimpleStorageServiceBufferized::processFileWriteRequest(std::shared_ptr<FileLocation> &location,
                                                                 double num_bytes_to_write,
                                                                 simgrid::s4u::Mailbox *answer_mailbox) {

        auto file = location->getFile();
        LogicalFileSystem *fs;

        // Figure out whether this succeeds or not
        std::shared_ptr<FailureCause> failure_cause = nullptr;

        std::string mountpoint;
        std::string path_at_mount_point;
        if (not this->splitPath(location->getPath(), mountpoint, path_at_mount_point)) {
            failure_cause = std::shared_ptr<FailureCause>(new InvalidDirectoryPath(location));
        }

        if (not failure_cause) {
            fs = this->file_systems[mountpoint].get();

            // If the file is not already there, do a capacity check/update
            // (If the file is already there, then there will just be an overwrite. Note that
            // if the overwrite fails, then the file will disappear, which is expected)

            bool file_already_there = fs->doesDirectoryExist(path_at_mount_point) and
                                      fs->isFileInDirectory(file, path_at_mount_point);
            if (not file_already_there) {
                if (not fs->reserveSpace(file, path_at_mount_point)) {
                    failure_cause = std::shared_ptr<FailureCause>(
                            new StorageServiceNotEnoughSpace(
                                    file,
                                    this->getSharedPtr<SimpleStorageService>()));
                }
            }
        }

        if (failure_cause == nullptr) {

            if (not fs->doesDirectoryExist(path_at_mount_point)) {
                fs->createDirectory(path_at_mount_point);
            }

            // Generate a mailbox_name name on which to receive the file
            auto file_reception_mailbox = S4U_Mailbox::getTemporaryMailbox();
            //            auto file_reception_mailbox = S4U_Mailbox::generateUniqueMailbox("faa_does_not_work");

            // Reply with a "go ahead, send me the file" message
            S4U_Mailbox::dputMessage(
                    answer_mailbox,
                    new StorageServiceFileWriteAnswerMessage(
                            location,
                            true,
                            nullptr,
                            {{file_reception_mailbox, location->getFile()->getSize()}},
                            this->buffer_size,
                            this->getMessagePayloadValue(
                                    SimpleStorageServiceMessagePayload::FILE_WRITE_ANSWER_MESSAGE_PAYLOAD)));

            // Create a FileTransferThread
            auto ftt = std::make_shared<FileTransferThread>(
                    this->hostname,
                    this->getSharedPtr<StorageService>(),
                    file,
                    num_bytes_to_write,
                    file_reception_mailbox,
                    location,
                    nullptr,
                    answer_mailbox,
                    nullptr,
                    this->buffer_size);
            ftt->setSimulation(this->simulation);

            // Add it to the Pool of pending data communications
            this->pending_file_transfer_threads.push_back(ftt);

        } else {
            // Reply with a "failure" message
            S4U_Mailbox::dputMessage(
                    answer_mailbox,
                    new StorageServiceFileWriteAnswerMessage(
                            location,
                            false,
                            failure_cause,
                            {},
                            0,
                            this->getMessagePayloadValue(
                                    SimpleStorageServiceMessagePayload::FILE_WRITE_ANSWER_MESSAGE_PAYLOAD)));
        }

        return true;
    }

    /**
     * @brief Handle a file read request
     * @param location: the file's location
     * @param num_bytes_to_read: the number of bytes to read
     * @param answer_mailbox: the mailbox to which the answer should be sent
     * @return
     */
    bool SimpleStorageServiceBufferized::processFileReadRequest(
            const std::shared_ptr<FileLocation> &location,
            double num_bytes_to_read,
            simgrid::s4u::Mailbox *answer_mailbox) {
        // Figure out whether this succeeds or not
        std::shared_ptr<FailureCause> failure_cause = nullptr;
        auto file = location->getFile();

        std::string mount_point;
        std::string path_at_mount_point;
        if ((not this->splitPath(location->getPath(), mount_point, path_at_mount_point)) or
            (not this->file_systems[mount_point]->doesDirectoryExist(path_at_mount_point))) {
            failure_cause = std::shared_ptr<FailureCause>(
                    new InvalidDirectoryPath(location));
        } else {
            if (not this->file_systems[mount_point]->isFileInDirectory(file, path_at_mount_point)) {
                WRENCH_INFO(
                        "Received a read request for a file I don't have (%s)", location->toString().c_str());
                failure_cause = std::shared_ptr<FailureCause>(new FileNotFound(location));
            }
        }


        bool success = (failure_cause == nullptr);
        simgrid::s4u::Mailbox *mailbox_to_receive_the_file_content = nullptr;
        if (success) {
            mailbox_to_receive_the_file_content = S4U_Mailbox::getTemporaryMailbox();
        }

        // Send back the corresponding ack, asynchronously and in a "fire and forget" fashion
        S4U_Mailbox::dputMessage(
                answer_mailbox,
                new StorageServiceFileReadAnswerMessage(
                        location,
                        success,
                        failure_cause,
                        mailbox_to_receive_the_file_content,
                        buffer_size,
                        1,
                        this->getMessagePayloadValue(StorageServiceMessagePayload::FILE_READ_ANSWER_MESSAGE_PAYLOAD)));

        // If success, then follow up with sending the file (ASYNCHRONOUSLY!)
        if (success) {
            // Create a FileTransferThread
            auto ftt = std::make_shared<FileTransferThread>(
                    this->hostname,
                    this->getSharedPtr<StorageService>(),
                    file,
                    num_bytes_to_read,
                    location,
                    mailbox_to_receive_the_file_content,
                    answer_mailbox,
                    nullptr,
                    nullptr,
                    buffer_size);
            ftt->setSimulation(this->simulation);

            // Add it to the Pool of pending data communications
            this->pending_file_transfer_threads.push_front(ftt);

            // Update the file read date in the file system
            this->file_systems[mount_point]->updateReadDate(file, path_at_mount_point);

            // Mark the file as unevictable
            this->file_systems[mount_point]->incrementNumRunningTransactionsForFileInDirectory(
                    location->getFile(), path_at_mount_point);
        }

        return true;
    }

    /**
     * @brief Handle a file copy request
     * @param src_location: the source location
     * @param dst_location: the destination location
     * @param answer_mailbox: the mailbox to which the answer should be sent
     * @return
     */
    bool SimpleStorageServiceBufferized::processFileCopyRequest(
            std::shared_ptr<FileLocation> &src_location,
            std::shared_ptr<FileLocation> &dst_location,
            simgrid::s4u::Mailbox *answer_mailbox) {


        auto file = src_location->getFile();


        // Check that the source has it
        if (not StorageService::hasFileAtLocation(src_location)) {
            try {
                S4U_Mailbox::putMessage(
                        answer_mailbox,
                        new StorageServiceFileCopyAnswerMessage(
                                src_location,
                                dst_location,
                                false,
                                std::shared_ptr<FailureCause>(
                                        new FileNotFound(
                                                src_location)),
                                this->getMessagePayloadValue(
                                        SimpleStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD)));

            } catch (ExecutionException &e) {
                return true;
            }
            return true;
        }

        bool i_am_the_source = (src_location->getStorageService() == this->getSharedPtr<StorageService>());
        bool file_already_there_at_destination = StorageService::hasFileAtLocation(dst_location);

        // If file is not already here, reserve space for it
        if ((not file_already_there_at_destination) and
            (not dst_location->getStorageService()->reserveSpace(dst_location))) {
            this->simulation->getOutput().addTimestampFileCopyFailure(Simulation::getCurrentSimulatedDate(), file, src_location, dst_location);

            try {
                S4U_Mailbox::putMessage(
                        answer_mailbox,
                        new StorageServiceFileCopyAnswerMessage(
                                src_location,
                                dst_location,
                                false,
                                std::shared_ptr<FailureCause>(
                                        new StorageServiceNotEnoughSpace(
                                                file,
                                                this->getSharedPtr<SimpleStorageService>())),
                                this->getMessagePayloadValue(
                                        SimpleStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD)));

            } catch (ExecutionException &e) {
                return true;
            }
            return true;
        }

        WRENCH_INFO("Starting a thread to copy file %s from %s to %s",
                    file->getID().c_str(),
                    src_location->toString().c_str(),
                    dst_location->toString().c_str());

        // Create a file transfer thread
        auto ftt = std::make_shared<FileTransferThread>(
                this->hostname,
                this->getSharedPtr<StorageService>(),
                file,
                file->getSize(),
                src_location,
                dst_location,
                nullptr,
                nullptr,
                answer_mailbox,
                this->buffer_size);
        ftt->setSimulation(this->simulation);
        this->pending_file_transfer_threads.push_back(ftt);

        src_location->getStorageService()->incrementNumRunningOperationsForLocation(src_location);

        return true;
    }

    /**
     * @brief Start pending file transfer threads if any and if possible
     */
    void SimpleStorageServiceBufferized::startPendingFileTransferThread() {
        while ((not this->pending_file_transfer_threads.empty()) and
               (this->running_file_transfer_threads.size() < this->num_concurrent_connections)) {
            // Start a communications!
            auto ftt = this->pending_file_transfer_threads.at(0);
            this->pending_file_transfer_threads.pop_front();
            this->running_file_transfer_threads.insert(ftt);
            ftt->start(ftt, true, false);// Daemonize, non-auto-restart
        }
    }

    /**
     * @brief Process a notification received from a file transfer thread
     * @param ftt: the file transfer thread
     * @param src_mailbox: the transfer's source mailbox (or "" if source was not a mailbox)
     * @param src_location: the transfer's source location (or nullptr if source was not a location)
     * @param dst_mailbox: the transfer's destination mailbox (or "" if source was not a mailbox)
     * @param dst_location: the transfer's destination location (or nullptr if destination was not a location)
     * @param success: whether the transfer succeeded or not
     * @param failure_cause: the failure cause (nullptr if success)
     * @param answer_mailbox_if_read: the mailbox to send a read notification ("" if not a copy)
     * @param answer_mailbox_if_write: the mailbox to send a write notification ("" if not a copy)
     * @param answer_mailbox_if_copy: the mailbox to send a copy notification ("" if not a copy)
     * @return false if the daemon should terminate
     */
    bool SimpleStorageServiceBufferized::processFileTransferThreadNotification(const std::shared_ptr<FileTransferThread> &ftt,
                                                                               simgrid::s4u::Mailbox *src_mailbox,
                                                                               const std::shared_ptr<FileLocation> &src_location,
                                                                               simgrid::s4u::Mailbox *dst_mailbox,
                                                                               const std::shared_ptr<FileLocation> &dst_location,
                                                                               bool success,
                                                                               std::shared_ptr<FailureCause> failure_cause,
                                                                               simgrid::s4u::Mailbox *answer_mailbox_if_read,
                                                                               simgrid::s4u::Mailbox *answer_mailbox_if_write,
                                                                               simgrid::s4u::Mailbox *answer_mailbox_if_copy) {
        // Remove the ftt from the list of running ftt
        if (this->running_file_transfer_threads.find(ftt) == this->running_file_transfer_threads.end()) {
            WRENCH_INFO(
                    "Got a notification from a non-existing File Transfer Thread. Perhaps this is from a former life... ignoring");
        } else {
            this->running_file_transfer_threads.erase(ftt);
        }

        if (src_location) {
            src_location->getStorageService()->decrementNumRunningOperationsForLocation(src_location);
        }

        // Was the destination me?
        if (dst_location and (dst_location->getStorageService().get() == this)) {
            auto file = dst_location->getFile();
            std::string dst_mount_point;
            std::string dst_path_at_mount_point;
            if (not this->splitPath(dst_location->getPath(), dst_mount_point, dst_path_at_mount_point)) {
                throw std::runtime_error("SimpleStorageServiceBufferized::processFileTransferThreadNotification(): splitPath should not have failed on the dst_location");
            }
            if (success) {
                WRENCH_INFO("File %s stored!", file->getID().c_str());
                this->file_systems[dst_mount_point]->storeFileInDirectory(
                        file, dst_path_at_mount_point);
                // Deal with time stamps, previously we could test whether a real timestamp was passed, now this.
                // Maybe no corresponding timestamp.
                try {
                    this->simulation->getOutput().addTimestampFileCopyCompletion(Simulation::getCurrentSimulatedDate(), file, src_location, dst_location);
                } catch (invalid_argument &ignore) {
                }

            } else {
                // Process the failure, meaning, just un-decrease the free space
                this->file_systems[dst_mount_point]->unreserveSpace(file, dst_path_at_mount_point);
            }
        }

        // Send back the relevant ack if this was a read
        if (answer_mailbox_if_read and success) {
            WRENCH_INFO(
                    "Sending back an ack since this was a file read and some client is waiting for me to say something");
            S4U_Mailbox::dputMessage(answer_mailbox_if_read, new StorageServiceAckMessage(src_location));
        }

        // Send back the relevant ack if this was a write operation
        if (answer_mailbox_if_write and success) {
            WRENCH_INFO(
                    "Sending back an ack since this was a file write and some client is waiting for me to say something");
            S4U_Mailbox::dputMessage(answer_mailbox_if_write, new StorageServiceAckMessage(dst_location));
        }

        // Send back the relevant ack if this was a copy
        if (answer_mailbox_if_copy) {
            WRENCH_INFO(
                    "Sending back an ack since this was a file copy and some client is waiting for me to say something");
            if ((src_location == nullptr) or (dst_location == nullptr)) {
                throw std::runtime_error("SimpleStorageServiceBufferized::processFileTransferThreadNotification(): "
                                         "src_location and dst_location must be non-null");
            }
            S4U_Mailbox::dputMessage(
                    answer_mailbox_if_copy,
                    new StorageServiceFileCopyAnswerMessage(
                            src_location,
                            dst_location,
                            success,
                            std::move(failure_cause),
                            this->getMessagePayloadValue(
                                    SimpleStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD)));
        }

        return true;
    }

    /**
     * @brief Get number of File Transfer Threads that are currently running or are pending
     * @return The number of threads
     */
    double SimpleStorageServiceBufferized::countRunningFileTransferThreads() {
        return (double) this->running_file_transfer_threads.size() + (double) this->pending_file_transfer_threads.size();
    }

    /**
     * @brief Get the load (number of concurrent reads) on the storage service
     * @return the load on the service
     */
    double SimpleStorageServiceBufferized::getLoad() {
        return countRunningFileTransferThreads();
    }


}// namespace wrench
