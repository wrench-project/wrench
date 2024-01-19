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
#include <wrench/simgrid_S4U_util/S4U_CommPort.h>
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
//        this->release_held_mutexes();
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
        this->is_bufferized = false;
    }

    /**
     * @brief Process a transaction completion
     * @param transaction: the transaction
     */
    void SimpleStorageServiceNonBufferized::processTransactionCompletion(const std::shared_ptr<Transaction> &transaction) {

        if (transaction->src_location) {
            transaction->src_location->getStorageService()->decrementNumRunningOperationsForLocation(
                    transaction->src_location);
        }

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
            transaction->commport->dputMessage(new StorageServiceAckMessage(transaction->src_location));
        } else if (transaction->src_location == nullptr) {
            StorageService::createFileAtLocation(transaction->dst_location);
            WRENCH_INFO("File %s stored", transaction->dst_location->getFile()->getID().c_str());
            // Deal with time stamps, previously we could test whether a real timestamp was passed, now this.
            // Maybe no corresponding timestamp.
            //            WRENCH_INFO("Sending back an ack for a successful file read");
            transaction->commport->dputMessage(new StorageServiceAckMessage(transaction->dst_location));
        } else {
            if (transaction->dst_location->getStorageService() == shared_from_this()) {
                this->createFile(transaction->dst_location);
                WRENCH_INFO("File %s stored", transaction->dst_location->getFile()->getID().c_str());
                try {
                    this->simulation->getOutput().addTimestampFileCopyCompletion(
                            Simulation::getCurrentSimulatedDate(), transaction->dst_location->getFile(), transaction->src_location, transaction->dst_location);
                } catch (invalid_argument &ignore) {
                }
            }

            //            WRENCH_INFO("Sending back an ack for a file copy");
            transaction->commport->dputMessage(
                    new StorageServiceFileCopyAnswerMessage(
                            transaction->src_location,
                            transaction->dst_location,
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

        // In case this was a restart!
        this->stream_to_transactions.clear();
        this->pending_transactions.clear();
        this->running_transactions.clear();
        this->commport->reset();
        this->recv_commport->reset();

        for (auto const &fs: this->file_systems) {
            message = "  - mount point " + fs.first + ": " +
                      std::to_string(fs.second->getFreeSpace()) + "/" +
                      std::to_string(fs.second->getTotalCapacity()) + " Bytes";
            WRENCH_INFO("%s", message.c_str());
        }

        //        WRENCH_INFO("STREAMS PENDING: %zu", this->pending_transactions.size());
        //        WRENCH_INFO("STREAMS RUNNING: %zu", this->pending_transactions.size());

        // If writeback device simulation is activated
#ifdef PAGE_CACHE_SIMULATION
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
#endif


        /** Main loop **/
        bool commport_comm_has_been_posted = false;
        std::shared_ptr<S4U_PendingCommunication> commport_pending_comm;
        std::unique_ptr<SimulationMessage> simulation_message;
        while (true) {

            S4U_Simulation::computeZeroFlop();

            this->startPendingTransactions();

            // Create an async recv if needed
            if (not commport_comm_has_been_posted) {
                try {
                    commport_pending_comm = this->commport->igetMessage();
//                    commport_pending_comm = this->commport->get_async<void>((void **) (&(simulation_message)));
                } catch (wrench::ExecutionException &e) {
                    // oh well
                    continue;
                }
                commport_comm_has_been_posted = true;
            }

            // Create all activities to wait on
#if 0
            std::vector<simgrid::s4u::ActivityPtr> pending_activities;
            pending_activities.emplace_back(commport_pending_comm);
            for (auto const &transaction: this->running_transactions) {
                pending_activities.emplace_back(transaction->stream);
            }
#else
            simgrid::s4u::ActivitySet pending_activities;
            pending_activities.push(commport_pending_comm->comm_ptr);
            pending_activities.push(commport_pending_comm->mess_ptr);
            for (auto const &transaction: this->running_transactions) {
                pending_activities.push(transaction->stream);
            }
#endif

#if 0
            // Wait one activity to complete
            int finished_activity_index;
            try {
                finished_activity_index = (int) simgrid::s4u::Activity::wait_any(pending_activities);
            } catch (simgrid::NetworkFailureException &e) {
                // the comm failed
                commport_comm_has_been_posted = false;
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
#else
            // Wait for one activity to complete
            simgrid::s4u::ActivityPtr finished_activity;
            try {
                finished_activity = pending_activities.wait_any();
            } catch (simgrid::Exception &e) {
                auto failed_activity = pending_activities.get_failed_activity();
                if (failed_activity == commport_pending_comm->comm_ptr) {
                    // the comm failed
                    commport_comm_has_been_posted = false;
                    commport_pending_comm->mess_ptr->cancel();
                    commport_pending_comm->mess_ptr = nullptr;
                    commport_pending_comm->comm_ptr->cancel();
                    commport_pending_comm->comm_ptr = nullptr;
                    continue;// oh well
                }
                if (failed_activity == commport_pending_comm->mess_ptr) {
                    // the mess failed
                    commport_comm_has_been_posted = false;
                    commport_pending_comm->mess_ptr->cancel();
                    commport_pending_comm->mess_ptr = nullptr;
                    commport_pending_comm->comm_ptr->cancel();
                    commport_pending_comm->comm_ptr = nullptr;
                    continue;// oh well
                }

                auto stream = boost::dynamic_pointer_cast<simgrid::s4u::Io>(finished_activity);
                auto transaction = this->stream_to_transactions[stream];
                this->stream_to_transactions.erase(transaction->stream);
                processTransactionFailure(transaction);
                continue;
            }
#endif

            if (finished_activity == commport_pending_comm->comm_ptr) {
//                XXX IS THIS A GOOD IDEA???? SHOULD WE JuSt REPLICATE CODE FROM COMPORT
                commport_pending_comm->mess_ptr->cancel();
                auto msg = commport_pending_comm->simulation_message.get();
//                commport_pending_comm->mess_ptr->cancel();
                commport_comm_has_been_posted = false;
                if (not processNextMessage(msg)) break;
            } else if (finished_activity == commport_pending_comm->mess_ptr) {
//                simulation_message = commport_pending_comm->wait();
                auto msg = commport_pending_comm->simulation_message.get();
                commport_pending_comm->comm_ptr->cancel();
                commport_comm_has_been_posted = false;
                if (not processNextMessage(msg)) break;
            } else {
                auto stream = boost::dynamic_pointer_cast<simgrid::s4u::Io>(finished_activity);
                auto transaction = this->stream_to_transactions[stream];
                this->running_transactions.erase(transaction);
                this->stream_to_transactions.erase(transaction->stream);
                processTransactionCompletion(transaction);
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
            return processStopDaemonRequest(msg->ack_commport);

        } else if (auto msg = dynamic_cast<StorageServiceFreeSpaceRequestMessage *>(message)) {
            return processFreeSpaceRequest(msg->answer_commport, msg->path);

        } else if (auto msg = dynamic_cast<StorageServiceFileDeleteRequestMessage *>(message)) {
            return processFileDeleteRequest(msg->location, msg->answer_commport);

        } else if (auto msg = dynamic_cast<StorageServiceFileLookupRequestMessage *>(message)) {
            return processFileLookupRequest(msg->location, msg->answer_commport);

        } else if (auto msg = dynamic_cast<StorageServiceFileWriteRequestMessage *>(message)) {
            return processFileWriteRequest(msg->location,
                                           msg->num_bytes_to_write,
                                           msg->answer_commport,
                                           msg->requesting_host);

        } else if (auto msg = dynamic_cast<StorageServiceFileReadRequestMessage *>(message)) {
            return processFileReadRequest(msg->location,
                                          msg->num_bytes_to_read, msg->answer_commport, msg->requesting_host);

        } else if (auto msg = dynamic_cast<StorageServiceFileCopyRequestMessage *>(message)) {
            return processFileCopyRequest(msg->src, msg->dst, msg->answer_commport);
        } else {
            throw std::runtime_error(
                    "SimpleStorageServiceNonBufferized::processNextMessage(): Unexpected [" + message->getName() + "] message");
        }
    }

    /**
     * @brief Handle a file write request
     *
     * @param location: the location to write the file to
     * @param num_bytes_to_write: the number of bytes to write to the file
     * @param answer_commport: the commport to which the reply should be sent
     * @param requesting_host: the requesting host
     * @return true if this process should keep running
     */
    bool SimpleStorageServiceNonBufferized::processFileWriteRequest(std::shared_ptr<FileLocation> &location,
                                                                    double num_bytes_to_write,
                                                                    S4U_CommPort *answer_commport,
                                                                    simgrid::s4u::Host *requesting_host) {

        if (buffer_size >= 1.0) {
            throw std::runtime_error("SimpleStorageServiceNonBufferized::processFileWriteRequest(): Cannot process a write requests with a non-zero buffer size");
        }

        auto file = location->getFile();
        LogicalFileSystem *fs;

        // Figure out whether this succeeds or not
        std::shared_ptr<FailureCause> failure_cause = nullptr;

        std::string mount_point;
        std::string path_at_mount_point;
        if (not this->splitPath(location->getPath(), mount_point, path_at_mount_point)) {
            failure_cause = std::make_shared<InvalidDirectoryPath>(location);
        } else {
            fs = this->file_systems[mount_point].get();

            // If the file is not already there, do a capacity check/update
            // (If the file is already there, then there will just be an overwrite.
            bool file_already_there = fs->isFileInDirectory(file, path_at_mount_point);

            if (not file_already_there) {
                // Reserve space
                bool success = fs->reserveSpace(file, path_at_mount_point);
                if (not success) {
                    failure_cause = std::shared_ptr<FailureCause>(
                            new StorageServiceNotEnoughSpace(
                                    file,
                                    this->getSharedPtr<SimpleStorageService>()));
                }
            }
        }

        if (failure_cause) {
            try {
                answer_commport->dputMessage(
                        new StorageServiceFileWriteAnswerMessage(
                                location,
                                false,
                                std::shared_ptr<FailureCause>(
                                        new StorageServiceNotEnoughSpace(
                                                file,
                                                this->getSharedPtr<SimpleStorageService>())),
                                {},
                                0,
                                this->getMessagePayloadValue(
                                        SimpleStorageServiceMessagePayload::FILE_WRITE_ANSWER_MESSAGE_PAYLOAD)));
            } catch (wrench::ExecutionException &e) {
                return true;
            }
            return true;
        }

        // Create directory if need be
        if (not fs->doesDirectoryExist(path_at_mount_point)) {
            fs->createDirectory(path_at_mount_point);
        }

        // Reply with a "go ahead, send me the file" message
        answer_commport->dputMessage(
                new StorageServiceFileWriteAnswerMessage(
                        location,
                        true,
                        nullptr,
                        {},
                        this->buffer_size,
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
                answer_commport,
                num_bytes_to_write);

        // Add it to the Pool of pending data communications
        this->pending_transactions.push_back(transaction);

        return true;
    }

    /**
     * @brief Handle a file read request
     * @param location: the file's location
     * @param num_bytes_to_read: the number of bytes to read
     * @param answer_commport: the commport to which the answer should be sent
     * @param requesting_host: the requesting_host
     * @return
     */
    bool SimpleStorageServiceNonBufferized::processFileReadRequest(
            const std::shared_ptr<FileLocation> &location,
            double num_bytes_to_read,
            S4U_CommPort *answer_commport,
            simgrid::s4u::Host *requesting_host) {

        // Figure out whether this succeeds or not
        std::shared_ptr<FailureCause> failure_cause = nullptr;

        LogicalFileSystem *fs = nullptr;
        auto file = location->getFile();

        std::string mount_point;
        std::string path_at_mount_point;
        if ((not this->splitPath(location->getPath(), mount_point, path_at_mount_point)) or
            (not this->file_systems[mount_point]->doesDirectoryExist(path_at_mount_point))) {
            failure_cause = std::shared_ptr<FailureCause>(
                    new InvalidDirectoryPath(location));
        } else {
            fs = this->file_systems[mount_point].get();

            if (not fs->isFileInDirectory(file, path_at_mount_point)) {
                WRENCH_INFO(
                        "Received a read request for a file I don't have (%s)", location->toString().c_str());
                failure_cause = std::shared_ptr<FailureCause>(new FileNotFound(location));
            }
        }

        bool success = (failure_cause == nullptr);

        // If a success, create the chunk_receiving commport

        // Send back the corresponding ack
        try {
            answer_commport->putMessage(
                    new StorageServiceFileReadAnswerMessage(
                            location,
                            success,
                            failure_cause,
                            nullptr,// non-bufferized = no chunk-receiving commport
                            buffer_size,
                            1,
                            this->getMessagePayloadValue(StorageServiceMessagePayload::FILE_READ_ANSWER_MESSAGE_PAYLOAD)));
        } catch (ExecutionException &e) {
            return true; // oh well
        }

        // If success, then follow up with sending the file (ASYNCHRONOUSLY!)
        if (success) {
            // Make the file un-evictable
            location->getStorageService()->incrementNumRunningOperationsForLocation(location);

            fs->updateReadDate(location->getFile(), location->getPath());

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
                    answer_commport,
                    num_bytes_to_read);

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
     * @param answer_commport: the commport to which the answer should be sent
     * @return
     */
    bool SimpleStorageServiceNonBufferized::processFileCopyRequestIAmNotTheSource(
            std::shared_ptr<FileLocation> &src_location,
            std::shared_ptr<FileLocation> &dst_location,
            S4U_CommPort *answer_commport) {

        WRENCH_INFO("FileCopyRequest: %s -> %s",
                    src_location->toString().c_str(),
                    dst_location->toString().c_str());

        auto src_host = src_location->getStorageService()->getHost();
        auto dst_host = dst_location->getStorageService()->getHost();

        auto src_disk = src_location->getDiskOrNull();
        if (src_disk == nullptr) {
            throw std::runtime_error("SimpleStorageServiceNonBufferized::processFileCopyRequestIAmNotTheSource(): source disk not found - internal error");
        }
        auto dst_disk = dst_location->getDiskOrNull();
        if (dst_disk == nullptr) {
            throw std::runtime_error("SimpleStorageServiceNonBufferized::processFileCopyRequestIAmNotTheSource(): destination disk not found - internal error");
        }

        auto file = src_location->getFile();
        LogicalFileSystem *fs;
        std::shared_ptr<FailureCause> failure_cause = nullptr;

        std::string dst_mount_point;
        std::string dst_path_at_mount_point;
        if (not this->splitPath(dst_location->getPath(), dst_mount_point, dst_path_at_mount_point)) {
            failure_cause = std::make_shared<InvalidDirectoryPath>(dst_location);
        }

        if (!failure_cause) {

            fs = this->file_systems[dst_mount_point].get();

            // If file is not already here, reserve space for it
            bool file_is_already_here = fs->isFileInDirectory(dst_location->getFile(), dst_path_at_mount_point);
            if ((not file_is_already_here) and (not fs->reserveSpace(dst_location->getFile(), dst_path_at_mount_point))) {
                failure_cause = std::make_shared<StorageServiceNotEnoughSpace>(
                        file,
                        this->getSharedPtr<SimpleStorageService>());
            }
        }

        if (failure_cause) {
            this->simulation->getOutput().addTimestampFileCopyFailure(Simulation::getCurrentSimulatedDate(), file, src_location, dst_location);
            try {
                answer_commport->putMessage(
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

        src_location->getStorageService()->incrementNumRunningOperationsForLocation(src_location);

        // At this point, there is enough space
        // Create a Transaction
        auto transaction = std::make_shared<Transaction>(
                src_location,
                src_host,
                src_disk,
                dst_location,
                dst_host,
                dst_disk,
                answer_commport,
                file->getSize());

        this->pending_transactions.push_back(transaction);

        return true;
    }


    /**
     * @brief Handle a file copy request
     * @param src_location: the source location
     * @param dst_location: the destination location
     * @param answer_commport: the commport to which the answer should be sent
     * @return
     */
    bool SimpleStorageServiceNonBufferized::processFileCopyRequestIAmTheSource(
            std::shared_ptr<FileLocation> &src_location,
            std::shared_ptr<FileLocation> &dst_location,
            S4U_CommPort *answer_commport) {

        WRENCH_INFO("FileCopyRequest: %s -> %s",
                    src_location->toString().c_str(),
                    dst_location->toString().c_str());

        // TODO: This code is duplicated with the IAmNotTheSource version of this method
        auto src_host = src_location->getStorageService()->getHost();
        auto dst_host = dst_location->getStorageService()->getHost();

        auto src_disk = src_location->getDiskOrNull();
        if (src_disk == nullptr) {
            throw std::runtime_error("SimpleStorageServiceNonBufferized::processFileCopyRequestIAmTheSource(): source disk not found - internal error");
        }
        auto dst_disk = dst_location->getDiskOrNull();
        if (dst_disk == nullptr) {
            throw std::runtime_error("SimpleStorageServiceNonBufferized::processFileCopyRequestIAmTheSource(): destination disk not found - internal error");
        }

        auto file = src_location->getFile();

        std::string src_mount_point;
        std::string src_path_at_mount_point;

        if (not this->splitPath(src_location->getPath(), src_mount_point, src_path_at_mount_point)) {
            try {
                answer_commport->putMessage(
                        new StorageServiceFileCopyAnswerMessage(
                                src_location,
                                dst_location,
                                false,
                                std::make_shared<InvalidDirectoryPath>(src_location),
                                this->getMessagePayloadValue(
                                        SimpleStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD)));

            } catch (ExecutionException &e) {
                return true;
            }
            return true;
        }

        auto fs = this->file_systems[src_mount_point].get();
        auto my_fs = this->file_systems[src_mount_point].get();

        // Do I have the file
        if (not my_fs->isFileInDirectory(src_location->getFile(), src_path_at_mount_point)) {
            try {
                answer_commport->putMessage(
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

        // At this point, I have the file

        // Can file fit at the destination?
        //        auto dst_file_system = dst_location->getStorageService()->file_systems[dst_location->getMountPoint()].get();
        //        bool file_already_at_destination = dst_file_system->isFileInDirectory(dst_location->getFile(), dst_location->getAbsolutePathAtMountPoint());
        bool file_already_at_destination = StorageService::hasFileAtLocation(dst_location);

        // If not already at destination make space for it, and if not possible, then return an error
        if (not file_already_at_destination) {
            if (not dst_location->getStorageService()->reserveSpace(dst_location)) {
                try {
                    answer_commport->putMessage(
                            new StorageServiceFileCopyAnswerMessage(
                                    src_location,
                                    dst_location,
                                    false,
                                    std::shared_ptr<FailureCause>(
                                            new StorageServiceNotEnoughSpace(dst_location->getFile(),
                                                                             dst_location->getStorageService())),
                                    this->getMessagePayloadValue(
                                            SimpleStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD)));

                } catch (ExecutionException &e) {
                    return true;
                }
                return true;
            }
        }

        // At this point we're all good
        uint64_t transfer_size;
        transfer_size = (uint64_t) (file->getSize());

        src_location->getStorageService()->incrementNumRunningOperationsForLocation(src_location);

        // Create a Transaction
        auto transaction = std::make_shared<Transaction>(
                src_location,
                src_host,
                src_disk,
                dst_location,
                dst_host,
                dst_disk,
                answer_commport,
                transfer_size);

        // Add it to the Pool of pending data communications
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
            this->running_transactions.insert(transaction);
            sg_iostream->start();
        }
    }

    /**
    * @brief Get the load (number of concurrent reads) on the storage service
    * @return the load on the service
    */
    double SimpleStorageServiceNonBufferized::getLoad() {
        return (double) this->running_transactions.size() + (double) this->pending_transactions.size();
    }


    /**
     * @brief Process a file copy request
     * @param src: the source location
     * @param dst: the dst location
     * @param answer_commport: the answer commport
     * @return true is the service should continue;
     */
    bool SimpleStorageServiceNonBufferized::processFileCopyRequest(std::shared_ptr<FileLocation> &src,
                                                                   std::shared_ptr<FileLocation> &dst,
                                                                   S4U_CommPort *answer_commport) {

        // Check that src has the file
        if (not StorageService::hasFileAtLocation(src)) {
            answer_commport->dputMessage(
                    new StorageServiceFileCopyAnswerMessage(
                            src,
                            dst,
                            false,
                            std::make_shared<FileNotFound>(src),
                            this->getMessagePayloadValue(
                                    SimpleStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD)));

            return true;
        }

        if (src->getStorageService() != this->getSharedPtr<StorageService>()) {
            return processFileCopyRequestIAmNotTheSource(src, dst, answer_commport);
        } else {
            return processFileCopyRequestIAmTheSource(src, dst, answer_commport);
        }
    }

}// namespace wrench
