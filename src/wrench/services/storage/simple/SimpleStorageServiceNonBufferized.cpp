/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <utility>
#include <wrench/failure_causes/InvalidDirectoryPath.h>
#include <wrench/failure_causes/FileNotFound.h>
#include <wrench/failure_causes/StorageServiceNotEnoughSpace.h>

#include <wrench/services/storage/simple/SimpleStorageServiceNonBufferized.h>
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

WRENCH_LOG_CATEGORY(wrench_core_simple_storage_service_non_bufferized,
                    "Log category for Simple Storage Service Non Bufferized");

namespace wrench {

    /**
     * @brief Cleanup method
     *
     * @param has_returned_from_main: whether main() returned
     * @param return_value: the return value (if main() returned)
     */
    void SimpleStorageServiceNonBufferized::cleanup(bool has_returned_from_main, int return_value) {
    }

    /**
     * @brief Public constructor
     *
     * @param hostname: the name of the host on which to start the service
     * @param mount_points: the set of mount points
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     */
    SimpleStorageServiceNonBufferized::SimpleStorageServiceNonBufferized(const std::string &hostname,
                                                                         std::set<std::string> mount_points,
                                                                         WRENCH_PROPERTY_COLLECTION_TYPE property_list,
                                                                         WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list) : SimpleStorageService(hostname, std::move(mount_points), std::move(property_list), std::move(messagepayload_list),
                                                                                                                                                           "_" + std::to_string(SimpleStorageService::getNewUniqueNumber())) {

        this->buffer_size = 0;
    }

    /**
     * @brief Process a transaction completion
     * @param transaction: the transaction
     */
    void SimpleStorageServiceNonBufferized::processTransactionCompletion(const std::shared_ptr<Transaction> &transaction) {

        // If I was the source and the destination was bufferized, I am the one creating the file there! (yes,
        // this is ugly and lame, and one day we'll clean the storage service implementation
        if (transaction->src_location != nullptr and
            transaction->src_location->getStorageService() == shared_from_this() and
            transaction->dst_location != nullptr and
            transaction->dst_location->getStorageService()->isBufferized()) {
            transaction->dst_location->getStorageService()->createFile(transaction->dst_location);
        }


        // Send back the relevant ack if this was a read
        if (transaction->dst_location == nullptr) {
            //            WRENCH_INFO("Sending back an ack for a successful file read");
            S4U_Mailbox::dputMessage(transaction->mailbox, new StorageServiceAckMessage());
        } else if (transaction->src_location == nullptr) {
            WRENCH_INFO("File %s stored", transaction->dst_location->getFile()->getID().c_str());
            this->file_systems[transaction->dst_location->getMountPoint()]->storeFileInDirectory(
                    transaction->dst_location->getFile(), transaction->dst_location->getAbsolutePathAtMountPoint());
            // Deal with time stamps, previously we could test whether a real timestamp was passed, now this.
            // Maybe no corresponding timestamp.
            //            WRENCH_INFO("Sending back an ack for a successful file read");
            S4U_Mailbox::dputMessage(transaction->mailbox, new StorageServiceAckMessage());
        } else {
            if (transaction->dst_location->getStorageService() == shared_from_this()) {
                WRENCH_INFO("File %s stored", transaction->dst_location->getFile()->getID().c_str());
                this->file_systems[transaction->dst_location->getMountPoint()]->storeFileInDirectory(
                        transaction->dst_location->getFile(), transaction->dst_location->getAbsolutePathAtMountPoint());
                try {
                    this->simulation->getOutput().addTimestampFileCopyCompletion(
                            Simulation::getCurrentSimulatedDate(), transaction->dst_location->getFile(), transaction->src_location, transaction->dst_location);
                } catch (invalid_argument &ignore) {
                }
            }

            //            WRENCH_INFO("Sending back an ack for a file copy");
            S4U_Mailbox::dputMessage(
                    transaction->mailbox,
                    new StorageServiceFileCopyAnswerMessage(
                            transaction->src_location,
                            transaction->dst_location,
                            nullptr,
                            false,
                            true,
                            nullptr,
                            this->getMessagePayloadValue(
                                    SimpleStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD)));
        }
    }

    /**
     * @brief Process a transaction failure
     * @param transaction: the transaction
     */
    void SimpleStorageServiceNonBufferized::processTransactionFailure(const std::shared_ptr<Transaction> &transaction) {
        throw std::runtime_error("SimpleStorageServiceNonBufferized::processTransactionFailure(): not implemented");
    }


    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int SimpleStorageServiceNonBufferized::main() {
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_CYAN);
        std::string message = "Simple Storage Service (Non-Bufferized) " + this->getName() + "  starting on host " + this->getHostname();
        WRENCH_INFO("%s", message.c_str());

        // "Start" all logical file systems
        for (auto const &fs: this->file_systems) {
            fs.second->init();
        }

        // In case this was a restart!
        this->stream_to_transactions.clear();
        this->pending_transactions.clear();
        this->running_transactions.clear();

        for (auto const &fs: this->file_systems) {
            message = "  - mount point " + fs.first + ": " +
                      std::to_string(fs.second->getFreeSpace()) + "/" +
                      std::to_string(fs.second->getTotalCapacity()) + " Bytes";
            WRENCH_INFO("%s", message.c_str())
        }

        //        WRENCH_INFO("STREAMS PENDING: %zu", this->pending_transactions.size());
        //        WRENCH_INFO("STREAMS RUNNING: %zu", this->pending_transactions.size());

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
        }

        /** Main loop **/
        bool comm_ptr_has_been_posted = false;
        simgrid::s4u::CommPtr comm_ptr;
        std::unique_ptr<SimulationMessage> simulation_message;
        while (true) {

            S4U_Simulation::computeZeroFlop();

            this->startPendingTransactions();

            // Create an async recv if needed
            if (not comm_ptr_has_been_posted) {
                try {
                    comm_ptr = this->mailbox->get_async<void>((void **) (&(simulation_message)));
                } catch (simgrid::NetworkFailureException &e) {
                    // oh well
                    continue;
                }
                comm_ptr_has_been_posted = true;
            }

            // Create all activities to wait on
            std::vector<simgrid::s4u::ActivityPtr> pending_activities;
            pending_activities.emplace_back(comm_ptr);

            // TODO: DEBUGGING REMOVED THAT
            for (auto const &transaction: this->running_transactions) {
                pending_activities.emplace_back(transaction->stream);
            }

            // Wait one activity to complete
            int finished_activity_index;
            try {
                finished_activity_index = (int) simgrid::s4u::Activity::wait_any(pending_activities);
            } catch (simgrid::NetworkFailureException &e) {
                // the comm failed
                comm_ptr_has_been_posted = false;
                continue;// oh well
            } catch (simgrid::Exception &e) {
                // This likely doesn't happen, but let's keep it here for now
                for (int i = 1; i < (int) pending_activities.size(); i++) {
                    if (pending_activities.at(i)->get_state() == simgrid::s4u::Activity::State::FAILED) {
                        auto finished_transaction = this->running_transactions[i - 1];
                        this->stream_to_transactions.erase(finished_transaction->stream);
                        processTransactionFailure(finished_transaction);
                        continue;
                    }
                }
                continue;
            } catch (std::exception &e) {
                continue;
            }

            // It's a communication
            if (finished_activity_index == 0) {
                comm_ptr_has_been_posted = false;
                if (not processNextMessage(simulation_message.get())) break;
            } else if (finished_activity_index > 0) {
                auto finished_transaction = this->running_transactions.at(finished_activity_index - 1);
                this->running_transactions.erase(this->running_transactions.begin() + finished_activity_index - 1);
                this->stream_to_transactions.erase(finished_transaction->stream);
                processTransactionCompletion(finished_transaction);
            } else if (finished_activity_index == -1) {
                throw std::runtime_error("wait_any() returned -1. Not sure what to do with this. ");
            }
        }

        WRENCH_INFO("Simple Storage Service (Non-Bufferized) %s on host %s cleanly terminating!",
                    this->getName().c_str(),
                    S4U_Simulation::getHostName().c_str());

        return 0;
    }

    /**
     * @brief Process a received control message
     *
     * @return false if the daemon should terminate
     */
    bool SimpleStorageServiceNonBufferized::processNextMessage(SimulationMessage *message) {

        WRENCH_INFO("Got a [%s] message", message->getName().c_str());

        if (auto msg = dynamic_cast<ServiceStopDaemonMessage *>(message)) {
            return processStopDaemonRequest(msg->ack_mailbox);

        } else if (auto msg = dynamic_cast<StorageServiceFreeSpaceRequestMessage *>(message)) {
            return processFreeSpaceRequest(msg->answer_mailbox);

        } else if (auto msg = dynamic_cast<StorageServiceFileDeleteRequestMessage *>(message)) {
            return processFileDeleteRequest(msg->location, msg->answer_mailbox);

        } else if (auto msg = dynamic_cast<StorageServiceFileLookupRequestMessage *>(message)) {
            return processFileLookupRequest(msg->location, msg->answer_mailbox);

        } else if (auto msg = dynamic_cast<StorageServiceFileWriteRequestMessage *>(message)) {
            return processFileWriteRequest(msg->location, msg->answer_mailbox,
                                           msg->requesting_host, msg->buffer_size);

        } else if (auto msg = dynamic_cast<StorageServiceFileReadRequestMessage *>(message)) {
            return processFileReadRequest(msg->location,
                                          msg->num_bytes_to_read, msg->answer_mailbox, msg->requesting_host);

        } else if (auto msg = dynamic_cast<StorageServiceFileCopyRequestMessage *>(message)) {
            return processFileCopyRequest(msg->src, msg->dst, msg->answer_mailbox);

        } else {
            throw std::runtime_error(
                    "SimpleStorageServiceNonBufferized::processNextMessage(): Unexpected [" + message->getName() + "] message");
        }
    }

    /**
     * @brief Handle a file write request
     *
     * @param location: the location to write the file to
     * @param answer_mailbox: the mailbox to which the reply should be sent
     * @param requesting_host: the requesting host
     * @param buffer_size: the buffer size to use
     * @return true if this process should keep running
     */
    bool SimpleStorageServiceNonBufferized::processFileWriteRequest(const std::shared_ptr<FileLocation> &location,
                                                                    simgrid::s4u::Mailbox *answer_mailbox, simgrid::s4u::Host *requesting_host,
                                                                    double buffer_size) {

        if (buffer_size >= 1.0) {
            throw std::runtime_error("SimpleStorageServiceNonBufferized::processFileWriteRequest(): Cannot process a write requests with a non-zero buffer size");
        }

        // Figure out whether this succeeds or not
        std::shared_ptr<FailureCause> failure_cause = nullptr;

        auto fs = this->file_systems[location->getMountPoint()].get();
        auto file = location->getFile();

        // If the file is not already there, do a capacity check/update
        // (If the file is already there, then there will just be an overwrite. Note that
        // if the overwrite fails, then the file will disappear, which is expected)

        if ((not fs->doesDirectoryExist(location->getAbsolutePathAtMountPoint())) or
            (not fs->isFileInDirectory(file, location->getAbsolutePathAtMountPoint()))) {
            if (not fs->hasEnoughFreeSpace(file->getSize())) {
                failure_cause = std::shared_ptr<FailureCause>(
                        new StorageServiceNotEnoughSpace(
                                file,
                                this->getSharedPtr<SimpleStorageService>()));
            }
        }

        if (failure_cause == nullptr) {

            if (not fs->doesDirectoryExist(location->getAbsolutePathAtMountPoint())) {
                fs->createDirectory(location->getAbsolutePathAtMountPoint());
            }

            // Update occupied space, in advance (will have to be decreased later in case of failure)
            fs->reserveSpace(file, location->getAbsolutePathAtMountPoint());

            // Reply with a "go ahead, send me the file" message
            S4U_Mailbox::dputMessage(
                    answer_mailbox,
                    new StorageServiceFileWriteAnswerMessage(
                            location,
                            true,
                            nullptr,
                            nullptr,
                            this->getMessagePayloadValue(
                                    SimpleStorageServiceMessagePayload::FILE_WRITE_ANSWER_MESSAGE_PAYLOAD)));

            // Create the streaming activity
            auto me_host = simgrid::s4u::this_actor::get_host();
            simgrid::s4u::Disk *me_disk = fs->getDisk();

            // Create a Transaction
            auto transaction = std::make_shared<Transaction>(
                    nullptr,
                    requesting_host,
                    nullptr,
                    location,
                    me_host,
                    me_disk,
                    answer_mailbox,
                    file->getSize());

            // Add it to the Pool of pending data communications
            this->pending_transactions.push_back(transaction);

        } else {
            // Reply with a "failure" message
            S4U_Mailbox::dputMessage(
                    answer_mailbox,
                    new StorageServiceFileWriteAnswerMessage(
                            location,
                            false,
                            failure_cause,
                            nullptr,
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
     * @param requesting_host: the requesting_host
     * @return
     */
    bool SimpleStorageServiceNonBufferized::processFileReadRequest(
            const std::shared_ptr<FileLocation> &location,
            double num_bytes_to_read,
            simgrid::s4u::Mailbox *answer_mailbox,
            simgrid::s4u::Host *requesting_host) {

        // Figure out whether this succeeds or not
        std::shared_ptr<FailureCause> failure_cause = nullptr;

        LogicalFileSystem *fs;
        auto file = location->getFile();

        //        if ((this->file_systems.find(location->getMountPoint()) == this->file_systems.end()) or
        if (not this->file_systems[location->getMountPoint()]->doesDirectoryExist(
                    location->getAbsolutePathAtMountPoint())) {
            failure_cause = std::shared_ptr<FailureCause>(
                    new InvalidDirectoryPath(
                            this->getSharedPtr<SimpleStorageService>(),
                            location->getMountPoint() + "/" +
                                    location->getAbsolutePathAtMountPoint()));
        } else {
            fs = this->file_systems[location->getMountPoint()].get();

            if (not fs->isFileInDirectory(file, location->getAbsolutePathAtMountPoint())) {
                WRENCH_INFO(
                        "Received a read request for a file I don't have (%s)", location->toString().c_str());
                failure_cause = std::shared_ptr<FailureCause>(new FileNotFound(location));
            }
        }

        bool success = (failure_cause == nullptr);

        // Send back the corresponding ack, asynchronously and in a "fire and forget" fashion
        S4U_Mailbox::dputMessage(
                answer_mailbox,
                new StorageServiceFileReadAnswerMessage(
                        location,
                        success,
                        failure_cause,
                        buffer_size,
                        this->getMessagePayloadValue(
                                SimpleStorageServiceMessagePayload::FILE_READ_ANSWER_MESSAGE_PAYLOAD)));

        // If success, then follow up with sending the file (ASYNCHRONOUSLY!)
        if (success) {

            // Create the streaming activity
            auto me_host = simgrid::s4u::this_actor::get_host();
            simgrid::s4u::Disk *me_disk = fs->getDisk();

            // Create a Transaction
            auto transaction = std::make_shared<Transaction>(
                    location,
                    me_host,
                    me_disk,
                    nullptr,
                    requesting_host,
                    nullptr,
                    answer_mailbox,
                    location->getFile()->getSize());

            // Add it to the Pool of pending data communications
            //            this->transactions[sg_iostream] = transaction;
            this->pending_transactions.push_back(transaction);
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
    bool SimpleStorageServiceNonBufferized::processFileCopyRequest(
            const std::shared_ptr<FileLocation> &src_location,
            const std::shared_ptr<FileLocation> &dst_location,
            simgrid::s4u::Mailbox *answer_mailbox) {

        WRENCH_INFO("FileCopyRequest: %s -> %s",
                    src_location->toString().c_str(),
                    dst_location->toString().c_str())

        auto src_host = simgrid::s4u::Host::by_name(src_location->getStorageService()->getHostname());
        auto dst_host = simgrid::s4u::Host::by_name(dst_location->getStorageService()->getHostname());
        // TODO: This disk identification is really ugly.
        simgrid::s4u::Disk *src_disk = nullptr;
        auto src_location_sanitized_mount_point = FileLocation::sanitizePath(src_location->getMountPoint() + "/");
        for (auto const &d: src_host->get_disks()) {
            if (src_location_sanitized_mount_point == FileLocation::sanitizePath(std::string(d->get_property("mount")) + "/")) {
                src_disk = d;
            }
        }
        if (src_disk == nullptr) {
            throw std::runtime_error("SimpleStorageServiceNonBufferized::processFileCopyRequest(): source disk not found - internal error");
        }
        simgrid::s4u::Disk *dst_disk = nullptr;
        auto dst_location_sanitized_mount_point = FileLocation::sanitizePath(dst_location->getMountPoint() + "/");
        for (auto const &d: dst_host->get_disks()) {
            if (dst_location_sanitized_mount_point == FileLocation::sanitizePath(std::string(d->get_property("mount")) + "/")) {
                dst_disk = d;
            }
        }
        if (dst_disk == nullptr) {
            throw std::runtime_error("SimpleStorageServiceNonBufferized::processFileCopyRequest(): destination disk not found - internal error");
        }

        auto file = src_location->getFile();

        // Am I the source????
        if (src_location->getStorageService() == this->shared_from_this()) {

            // If the source does not exit, return a failure
            if (not this->file_systems[src_location->getMountPoint()]->isFileInDirectory(file, src_location->getAbsolutePathAtMountPoint())) {
                try {
                    S4U_Mailbox::putMessage(
                            answer_mailbox,
                            new StorageServiceFileCopyAnswerMessage(
                                    src_location,
                                    dst_location,
                                    nullptr, false,
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

            uint64_t transfer_size;
            transfer_size = (uint64_t) (file->getSize());

            // Create a Transaction
            auto transaction = std::make_shared<Transaction>(
                    src_location,
                    src_host,
                    src_disk,
                    dst_location,
                    dst_host,
                    dst_disk,
                    answer_mailbox,
                    transfer_size);

            // Add it to the Pool of pending data communications
            this->pending_transactions.push_back(transaction);

            return true;
        }

        // I am not the source
        // Does the source have the file?
        auto fs = this->file_systems[dst_location->getMountPoint()].get();

        bool file_is_there;

        try {
            file_is_there = src_location->getStorageService()->lookupFile(src_location);
        } catch (ExecutionException &e) {
            try {
                S4U_Mailbox::putMessage(
                        answer_mailbox,
                        new StorageServiceFileCopyAnswerMessage(
                                src_location,
                                dst_location,
                                nullptr, false,
                                false,
                                e.getCause(),
                                this->getMessagePayloadValue(
                                        SimpleStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD)));
            } catch (ExecutionException &e) {
                return true;
            }
            return true;
        }

        if (not file_is_there) {
            try {
                S4U_Mailbox::putMessage(
                        answer_mailbox,
                        new StorageServiceFileCopyAnswerMessage(
                                src_location,
                                dst_location,
                                nullptr, false,
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

        // Is there enough space here?
        if (not fs->isFileInDirectory(file, dst_location->getAbsolutePathAtMountPoint())) {
            if (not fs->hasEnoughFreeSpace(file->getSize())) {
                this->simulation->getOutput().addTimestampFileCopyFailure(Simulation::getCurrentSimulatedDate(), file, src_location, dst_location);

                try {
                    S4U_Mailbox::putMessage(
                            answer_mailbox,
                            new StorageServiceFileCopyAnswerMessage(
                                    src_location,
                                    dst_location,
                                    nullptr, false,
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
            fs->reserveSpace(file, dst_location->getAbsolutePathAtMountPoint());
        }

        // Create a Transaction
        auto transaction = std::make_shared<Transaction>(
                src_location,
                src_host,
                src_disk,
                dst_location,
                dst_host,
                dst_disk,
                answer_mailbox,
                file->getSize());

        this->pending_transactions.push_back(transaction);

        return true;
    }

    /**
 * @brief Start pending file transfer threads if any and if possible
 */
    void SimpleStorageServiceNonBufferized::startPendingTransactions() {
        while ((not this->pending_transactions.empty()) and
               (this->running_transactions.size() < this->num_concurrent_connections)) {
            //            WRENCH_INFO("Starting pending transaction for file %s",
            //                        this->transactions[this->pending_sg_iostreams.at(0)]->file->getID().c_str());

            auto transaction = this->pending_transactions.front();
            this->pending_transactions.pop_front();

            auto sg_iostream = simgrid::s4u::Io::streamto_init(transaction->src_host,
                                                               transaction->src_disk,
                                                               transaction->dst_host,
                                                               transaction->dst_disk)
                                       ->set_size((uint64_t) (transaction->transfer_size));

            transaction->stream = sg_iostream;

            this->stream_to_transactions[sg_iostream] = transaction;
            this->running_transactions.push_back(transaction);
            sg_iostream->vetoable_start();
        }
    }

    /**
 * @brief Get the load (number of concurrent reads) on the storage service
 * @return the load on the service
 */
    double SimpleStorageServiceNonBufferized::getLoad() {
        return (double) this->running_transactions.size() + (double) this->pending_transactions.size();
    }


};// namespace wrench
