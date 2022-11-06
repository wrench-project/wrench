/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <climits>
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
        this->transactions.clear();
        this->pending_sg_iostreams.clear();
        this->running_sg_iostreams.clear();
        // Do nothing. It's fine to die. We'll just autorestart with our previous state
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
                                                                         WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list) :
            SimpleStorageService(hostname, std::move(mount_points), std::move(property_list), std::move(messagepayload_list),
                                 "_" + std::to_string(SimpleStorageService::getNewUniqueNumber())) {

        if (not Simulation::isSioS22CPUModelEnabled()) {
            throw std::runtime_error("To use a non-bufferized storage services (buffer size == 0), you need to run the simulator with the "
                                     "'--cfg=host/model:sio_S22' command-line argument. This may lead to less realistic simulations, but "
                                     "non-bufferized storage services typically make the simulation faster.");
        }
        this->buffer_size = 0;

    }

    /**
     * @brief Process a transaction completion
     * @param transaction: the transaction
     */
    void SimpleStorageServiceNonBufferized::processTransactionCompletion(const std::shared_ptr<Transaction>& transaction) {
        // Send back the relevant ack if this was a read
        if (transaction->dst_location == nullptr) {
//            WRENCH_INFO("Sending back an ack for a successful file read");
            S4U_Mailbox::dputMessage(transaction->mailbox, new StorageServiceAckMessage());
        } else if (transaction->src_location == nullptr) {
            WRENCH_INFO("File %s stored", transaction->file->getID().c_str());
            this->file_systems[transaction->dst_location->getMountPoint()]->storeFileInDirectory(
                    transaction->file, transaction->dst_location->getAbsolutePathAtMountPoint());
            // Deal with time stamps, previously we could test whether a real timestamp was passed, now this.
            // May be no corresponding timestamp.
//            WRENCH_INFO("Sending back an ack for a successful file read");
            S4U_Mailbox::dputMessage(transaction->mailbox, new StorageServiceAckMessage());
        } else {
            if (transaction->dst_location->getStorageService() == shared_from_this()) {
                WRENCH_INFO("File %s stored", transaction->file->getID().c_str());
                this->file_systems[transaction->dst_location->getMountPoint()]->storeFileInDirectory(
                        transaction->file, transaction->dst_location->getAbsolutePathAtMountPoint());
                try {
                    this->simulation->getOutput().addTimestampFileCopyCompletion(
                            Simulation::getCurrentSimulatedDate(), transaction->file, transaction->src_location, transaction->dst_location);
                } catch (invalid_argument &ignore) {
                }
            }

//            WRENCH_INFO("Sending back an ack for a file copy");
            S4U_Mailbox::dputMessage(
                    transaction->mailbox,
                    new StorageServiceFileCopyAnswerMessage(
                            transaction->file,
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
    void SimpleStorageServiceNonBufferized::processTransactionFailure(const std::shared_ptr<Transaction>& transaction) {
        throw std::runtime_error("SimpleStorageServiceNonBufferized::processTransactionFailure(): not implemented");
    }


    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int SimpleStorageServiceNonBufferized::main() {
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_CYAN);

        // "Start" all logical file systems
        for (auto const &fs: this->file_systems) {
            fs.second->init();
        }

        std::string message = "Simple Storage Service (Non-Bufferized) " + this->getName() + "  starting on host " + this->getHostname();
        WRENCH_INFO("%s", message.c_str());
        for (auto const &fs: this->file_systems) {
            message = "  - mount point " + fs.first + ": " +
                      std::to_string(fs.second->getFreeSpace()) + "/" +
                      std::to_string(fs.second->getTotalCapacity()) + " Bytes";
            WRENCH_INFO("%s", message.c_str())
        }

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
            for (auto const &stream : this->running_sg_iostreams) {
                pending_activities.emplace_back(stream);
            }

            // Wait one activity to complete
            ssize_t finished_activity_index;
            try {
                finished_activity_index = simgrid::s4u::Activity::wait_any(pending_activities);
            } catch (simgrid::NetworkFailureException &e) {
                // the comm failed
                continue; // oh well
            } catch (simgrid::Exception &e) {
                // This likely doesn't happen, but let's keep it here for now
                for (int i = 1; i < pending_activities.size(); i++) {
                    if (pending_activities.at(i)->get_state() == simgrid::s4u::Activity::State::FAILED) {
                        auto stream = this->running_sg_iostreams.at(i - 1);
                        processTransactionFailure(this->transactions[stream]);
                        continue;
                    }
                }
            }

            // It's a communication
            if (finished_activity_index == 0) {
                comm_ptr_has_been_posted = false;
                if (not processNextMessage(simulation_message.get())) break;
            } else if (finished_activity_index > 0) {
                auto finished_activity = this->running_sg_iostreams.at(finished_activity_index - 1);
                this->running_sg_iostreams.erase(this->running_sg_iostreams.begin() + finished_activity_index - 1);
                processTransactionCompletion(this->transactions[finished_activity]);
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

        WRENCH_DEBUG("Got a [%s] message", message->getName().c_str());

        if (auto msg = dynamic_cast<ServiceStopDaemonMessage *>(message)) {
            return processStopDaemonRequest(msg->ack_mailbox);

        } else if (auto msg = dynamic_cast<StorageServiceFreeSpaceRequestMessage *>(message)) {
            return processFreeSpaceRequest(msg->answer_mailbox);

        } else if (auto msg = dynamic_cast<StorageServiceFileDeleteRequestMessage *>(message)) {
            return processFileDeleteRequest(msg->file, msg->location, msg->answer_mailbox);

        } else if (auto msg = dynamic_cast<StorageServiceFileLookupRequestMessage *>(message)) {
            return processFileLookupRequest(msg->file, msg->location, msg->answer_mailbox);

        } else if (auto msg = dynamic_cast<StorageServiceFileWriteRequestMessage *>(message)) {
            return processFileWriteRequest(msg->file, msg->location, msg->answer_mailbox,
                                           msg->requesting_host,msg->buffer_size);

        } else if (auto msg = dynamic_cast<StorageServiceFileReadRequestMessage *>(message)) {
            return processFileReadRequest(msg->file, msg->location,
                                          msg->num_bytes_to_read, msg->answer_mailbox, msg->requesting_host);

        } else if (auto msg = dynamic_cast<StorageServiceFileCopyRequestMessage *>(message)) {
            return processFileCopyRequest(msg->file, msg->src, msg->dst, msg->answer_mailbox);

        } else {
            throw std::runtime_error(
                    "SimpleStorageServiceNonBufferized::processNextMessage(): Unexpected [" + message->getName() + "] message");
        }
    }

    /**
     * @brief Handle a file write request
     *
     * @param file: the file to write
     * @param location: the location to write the file to
     * @param answer_mailbox: the mailbox to which the reply should be sent
     * @param requesting_host: the requesting host
     * @param buffer_size: the buffer size to use
     * @return true if this process should keep running
     */
    bool SimpleStorageServiceNonBufferized::processFileWriteRequest(const std::shared_ptr<DataFile> &file, const std::shared_ptr<FileLocation> &location,
                                                                    simgrid::s4u::Mailbox *answer_mailbox, simgrid::s4u::Host *requesting_host,
                                                                    double buffer_size) {

        if (buffer_size >= 1.0) {
            throw std::runtime_error("SimpleStorageServiceNonBufferized::processFileWriteRequest(): Cannot process a write requests with a non-zero buffer size");
        }

        // Figure out whether this succeeds or not
        std::shared_ptr<FailureCause> failure_cause = nullptr;

        auto fs = this->file_systems[location->getMountPoint()].get();

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
        //        }

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
                            file,
                            location,
                            true,
                            nullptr,
                            nullptr,
                            this->getMessagePayloadValue(
                                    SimpleStorageServiceMessagePayload::FILE_WRITE_ANSWER_MESSAGE_PAYLOAD)));

            // Create the streaming activity
            auto me_host = simgrid::s4u::this_actor::get_host();
            simgrid::s4u::Disk *me_disk = fs->getDisk();


            auto sg_iostream = simgrid::s4u::Io::streamto_init(requesting_host,
                                                               nullptr,
                                                               me_host,
                                                               me_disk)->set_size((uint64_t)file->getSize());

            // Create a Transaction
            auto transaction = std::make_shared<Transaction>(
                    file,
                    nullptr,
                    location,
                    answer_mailbox);

            // Add it to the Pool of pending data communications
            this->transactions[sg_iostream] = transaction;
            this->pending_sg_iostreams.push_back(sg_iostream);

        } else {
            // Reply with a "failure" message
            S4U_Mailbox::dputMessage(
                    answer_mailbox,
                    new StorageServiceFileWriteAnswerMessage(
                            file,
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
     * @param file: the file
     * @param location: the file's location
     * @param num_bytes_to_read: the number of bytes to read
     * @param answer_mailbox: the mailbox to which the answer should be sent
     * @param requesting_host: the requesting_host
     * @return
     */
    bool SimpleStorageServiceNonBufferized::processFileReadRequest(const std::shared_ptr<DataFile> &file,
                                                                   const std::shared_ptr<FileLocation> &location,
                                                                   double num_bytes_to_read,
                                                                   simgrid::s4u::Mailbox *answer_mailbox,
                                                                   simgrid::s4u::Host *requesting_host) {
        // Figure out whether this succeeds or not
        std::shared_ptr<FailureCause> failure_cause = nullptr;

        LogicalFileSystem *fs;

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
                failure_cause = std::shared_ptr<FailureCause>(new FileNotFound(file, location));
            }
        }

        bool success = (failure_cause == nullptr);

        // Send back the corresponding ack, asynchronously and in a "fire and forget" fashion
        S4U_Mailbox::dputMessage(
                answer_mailbox,
                new StorageServiceFileReadAnswerMessage(
                        file,
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

            auto sg_iostream = simgrid::s4u::Io::streamto_init(me_host,
                                                               me_disk,
                                                               requesting_host,
                                                               nullptr)->set_size((uint64_t)file->getSize());

            // Create a Transaction
            auto transaction = std::make_shared<Transaction>(
                    file,
                    location,
                    nullptr,
                    answer_mailbox);

            // Add it to the Pool of pending data communications
            this->transactions[sg_iostream] = transaction;
            this->pending_sg_iostreams.push_back(sg_iostream);

        }

        return true;
    }

    /**
     * @brief Handle a file copy request
     * @param file: the file
     * @param src_location: the source location
     * @param dst_location: the destination location
     * @param answer_mailbox: the mailbox to which the answer should be sent
     * @return
     */
    bool SimpleStorageServiceNonBufferized::processFileCopyRequest(const std::shared_ptr<DataFile> &file,
                                                                   const std::shared_ptr<FileLocation> &src_location,
                                                                   const std::shared_ptr<FileLocation> &dst_location,
                                                                   simgrid::s4u::Mailbox *answer_mailbox) {

        WRENCH_INFO("FileCopyRequest: %s -> %s",
                    src_location->toString().c_str(),
                    dst_location->toString().c_str())

        auto fs = this->file_systems[dst_location->getMountPoint()].get();

        // Am I the source????
        if (src_location->getStorageService() == this->shared_from_this()) {

            // If the source does not exit, return a failure
            if (not this->file_systems[src_location->getMountPoint()]->isFileInDirectory(file, src_location->getAbsolutePathAtMountPoint())) {
                try {
                    S4U_Mailbox::putMessage(
                            answer_mailbox,
                            new StorageServiceFileCopyAnswerMessage(
                                    file,
                                    src_location,
                                    dst_location,
                                    nullptr, false,
                                    false,
                                    std::shared_ptr<FailureCause>(
                                            new FileNotFound(
                                                    file,
                                                    src_location)),
                                    this->getMessagePayloadValue(
                                            SimpleStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD)));

                } catch (ExecutionException &e) {
                    return true;
                }
                return true;
            }
            uint64_t transfer_size;
            if (src_location->equal(dst_location)) {
                WRENCH_WARN("Asked to copy file %s only itself (will take zero-ish time)", file->getID().c_str());
                transfer_size = 1; // TODO: Change that to ZERO whenever SimGrid is fixed.
            } else {
                transfer_size = (uint64_t)(file->getSize());
            }

            auto sg_iostream = simgrid::s4u::Io::streamto_init(simgrid::s4u::this_actor::get_host(),
                                                               fs->getDisk(),
                                                               simgrid::s4u::this_actor::get_host(),
                                                               nullptr)->set_size(transfer_size);

            // Create a Transaction
            auto transaction = std::make_shared<Transaction>(
                    file,
                    src_location,
                    dst_location,
                    answer_mailbox);

            // Add it to the Pool of pending data communications
            this->transactions[sg_iostream] = transaction;
            this->pending_sg_iostreams.push_back(sg_iostream);

            return true;

        }

        // Does the source have the file?
        if (not src_location->getStorageService()->lookupFile(file, src_location)) {
            try {
                S4U_Mailbox::putMessage(
                        answer_mailbox,
                        new StorageServiceFileCopyAnswerMessage(
                                file,
                                src_location,
                                dst_location,
                                nullptr, false,
                                false,
                                std::shared_ptr<FailureCause>(
                                        new FileNotFound(
                                                file,
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
                                    file,
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

//        WRENCH_INFO("Starting an activity to copy file %s from %s to %s",
//                    file->getID().c_str(),
//                    src_location->toString().c_str(),
//                    dst_location->toString().c_str());

        // Create the streaming activity
        auto src_host = simgrid::s4u::Host::by_name(src_location->getStorageService()->getHostname());
        // TODO: This disk identification is really ugly. Perhaps implement FileLocation::getDisk()?
        simgrid::s4u::Disk *src_disk = nullptr;
        auto src_location_sanitized_mount_point =  FileLocation::sanitizePath(src_location->getMountPoint() + "/");
        for (auto const &d: src_host->get_disks()) {
            if (src_location_sanitized_mount_point == FileLocation::sanitizePath(std::string(d->get_property("mount")) + "/")) {
                src_disk = d;
            }
        }
        if (src_disk == nullptr) {
            throw std::runtime_error("SimpleStorageServiceNonBufferized::processFileCopyRequest(): disk not found - internal error");
        }

        auto dst_host = simgrid::s4u::Host::by_name(dst_location->getStorageService()->getHostname());
        simgrid::s4u::Disk *dst_disk = fs->getDisk();

        auto sg_iostream = simgrid::s4u::Io::streamto_init(src_host,
                                                           src_disk,
                                                           dst_host,
                                                           dst_disk)->set_size((uint64_t)file->getSize());

        // Create a Transaction
        auto transaction = std::make_shared<Transaction>(
                file,
                src_location,
                dst_location,
                answer_mailbox);

// Add it to the Pool of pending data communications
        this->transactions[sg_iostream] = transaction;
        this->pending_sg_iostreams.push_back(sg_iostream);

        return true;
    }

/**
 * @brief Start pending file transfer threads if any and if possible
 */
    void SimpleStorageServiceNonBufferized::startPendingTransactions() {
        while ((not this->pending_sg_iostreams.empty()) and
               (this->running_sg_iostreams.size() < this->num_concurrent_connections)) {
            WRENCH_INFO("Starting pending transaction for file %s",
                        this->transactions[this->pending_sg_iostreams.at(0)]->file->getID().c_str());
            auto sg_iostream = this->pending_sg_iostreams.at(0);
            this->pending_sg_iostreams.pop_front();
            this->running_sg_iostreams.push_back(sg_iostream);
            sg_iostream->vetoable_start();
            WRENCH_INFO("Transaction started");
        }
    }

/**
 * @brief Get the load (number of concurrent reads) on the storage service
 * @return the load on the service
 */
    double SimpleStorageServiceNonBufferized::getLoad() {
        // TODO: TO RE-IMPLE<MENT FOR NON-BUFFERIZED
        return 0.0;
    }


};// namespace wrench
