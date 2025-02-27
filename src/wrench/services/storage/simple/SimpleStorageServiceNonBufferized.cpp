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
#include <wrench/services/storage/storage_helpers/FileLocation.h>
//#include <wrench/services/memory/MemoryManager.h>

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
                                                                         const std::set<std::string>& mount_points,
                                                                         WRENCH_PROPERTY_COLLECTION_TYPE property_list,
                                                                         WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE messagepayload_list) :
            SimpleStorageService(hostname, mount_points, std::move(property_list), std::move(messagepayload_list),
                                 "_" + std::to_string(SimpleStorageService::getNewUniqueNumber())) {
        this->buffer_size = 0;
        this->is_bufferized = false;
    }

    /**
     * @brief Process a transaction completion
     * @param transaction: the transaction
     */
    void SimpleStorageServiceNonBufferized::processTransactionCompletion(const std::shared_ptr<Transaction> &transaction) {

        // Deal with possibly open source file
        if (transaction->src_opened_file) {
            // Doing a 1-byte write just so that the file access date is updated
            transaction->src_opened_file->seek(0, SEEK_SET);
            // TODO: passing true raises a crazy bug in lmm solver:
            // TODO: "throws std::length_error with description "vector"
            // TODO: Luckily for this method, we should pass false, but passing
            // TODO: true by mistake has likely highlighted a strange C++ memory bug
            // TODO: in the core of SimGrid's LMM solver... pretty scary.
            transaction->src_opened_file->read(1, false);
            transaction->src_opened_file->close();
        }
        // Deal with possibly opened destination file
        if (transaction->dst_opened_file) {
            auto dst_file_system = transaction->dst_opened_file->get_file_system();
            auto dst_file_path = transaction->dst_opened_file->get_path();
            transaction->dst_opened_file->close();
            if (not dst_file_system->file_exists(transaction->dst_location->getFilePath())) {
                dst_file_system->move_file(dst_file_path, transaction->dst_location->getFilePath());
            }
        }

//        // If I was the source and the destination was bufferized, I am the one creating the file there! (yes,
//        // this is ugly and lame, and one day we'll clean the storage service implementation
//        if (transaction->src_location != nullptr and
//            transaction->src_location->getStorageService() == shared_from_this() and
//            transaction->dst_location != nullptr and
//            transaction->dst_location->getStorageService()->isBufferized()) {
//            transaction->dst_location->getStorageService()->createFile(transaction->dst_location);
//        }


        // TODO: If below I do dputMessage instead of putMessage, the MessImpl objects accumulate
        //       and are freed only at the end of the simulation.

        // Send back the relevant ack if this was a read
        if (transaction->dst_location == nullptr) {
            //            WRENCH_INFO("Sending back an ack for a successful file read");
            transaction->commport->dputMessage(new StorageServiceAckMessage(transaction->src_location));
        } else if (transaction->src_location == nullptr) {
//            StorageService::createFileAtLocation(transaction->dst_location);
            WRENCH_INFO("File %s stored", transaction->dst_location->getFile()->getID().c_str());
            // Deal with time stamps, previously we could test whether a real timestamp was passed, now this.
            // Maybe no corresponding timestamp.
            //            WRENCH_INFO("Sending back an ack for a successful file read");
            transaction->commport->dputMessage(new StorageServiceAckMessage(transaction->dst_location));
        } else {
            if (transaction->dst_location->getStorageService() == shared_from_this()) {
//                this->createFile(transaction->dst_location);
                WRENCH_INFO("File %s stored", transaction->dst_location->getFile()->getID().c_str());
                try {
                    this->simulation_->getOutput().addTimestampFileCopyCompletion(
                            Simulation::getCurrentSimulatedDate(), transaction->dst_location->getFile(), transaction->src_location, transaction->dst_location);
                } catch (invalid_argument &ignore) {
                }
            }

            WRENCH_INFO("Sending back an ack for a file copy");
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

        for (auto const &part: this->file_system->get_partitions()) {
            message = "  - mount point " + part->get_name() + ": " +
                      std::to_string(part->get_free_space()) + "/" +
                      std::to_string(part->get_size()) + " Bytes";
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
            this->memory_manager = MemoryManager::initAndStart(this->simulation_, memory_disk, 0.4, 5, 30, this->hostname);
        }
#endif


        /** Main loop **/
        bool comm_has_been_posted = false;
        bool mess_has_been_posted = false;
        simgrid::s4u::CommPtr comm_ptr;
        simgrid::s4u::MessPtr mess_ptr;
        std::unique_ptr<SimulationMessage> simulation_message;

        while (true) {

            S4U_Simulation::computeZeroFlop();

            this->startPendingTransactions();

            // Create an async recv on the mailbox if needed
            if (not comm_has_been_posted) {
                try {
                    comm_ptr = this->commport->s4u_mb->get_async<void>(reinterpret_cast<void**>(&(simulation_message)));
                } catch (wrench::ExecutionException &e) {
                    // oh well
                    continue;
                }
                comm_has_been_posted = true;
            }

            // Create an async recv on the message queue if needed
            if (not mess_has_been_posted) {
                mess_ptr = this->commport->s4u_mq->get_async<void>(reinterpret_cast<void**>(&(simulation_message)));
                mess_has_been_posted = true;
            } else {

            }

            // Create all activities to wait on
            simgrid::s4u::ActivitySet pending_activities;
            pending_activities.push(comm_ptr);
            pending_activities.push(mess_ptr);
            for (auto const &transaction: this->running_transactions) {
                pending_activities.push(transaction->stream);
            }

            // Wait for one activity to complete
            simgrid::s4u::ActivityPtr finished_activity;
            try {
                finished_activity = pending_activities.wait_any();
            } catch (simgrid::Exception &e) {
                auto failed_activity = pending_activities.get_failed_activity();
                if (failed_activity == comm_ptr) {
                    // the comm failed
                    comm_has_been_posted = false;
                    comm_ptr->cancel();
                    comm_ptr = nullptr;
                    continue;// oh well
                }
                if (failed_activity == mess_ptr) {
                    // the mess failed
                    mess_has_been_posted = false;
                    mess_ptr->cancel();
                    mess_ptr = nullptr;
                    continue;// oh well
                }

                auto stream = boost::dynamic_pointer_cast<simgrid::s4u::Io>(finished_activity);
                auto transaction = this->stream_to_transactions[stream];
                this->stream_to_transactions.erase(transaction->stream);
                processTransactionFailure(transaction);
                continue;
            }

            if (finished_activity == comm_ptr) {
                auto msg = simulation_message.get();
                comm_has_been_posted = false;
                if (not processNextMessage(msg)) break;
            } else if (finished_activity == mess_ptr) {
                auto msg = simulation_message.get();
                mess_has_been_posted = false;
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
                                                                    sg_size_t num_bytes_to_write,
                                                                    S4U_CommPort *answer_commport,
                                                                    simgrid::s4u::Host *requesting_host) {

        if (buffer_size >= 1) {
            throw std::runtime_error("SimpleStorageServiceNonBufferized::processFileWriteRequest(): Cannot process a write requests with a non-zero buffer size");
        }

        WRENCH_INFO("Processing FileWriteRequests");

        std::shared_ptr<simgrid::fsmod::File> opened_file;
        auto failure_cause = validateFileWriteRequest(location, num_bytes_to_write, opened_file);

        if (failure_cause) {
            WRENCH_INFO("ERROR CAUSE: %s", failure_cause->toString().c_str());
            try {
                answer_commport->dputMessage(
                        new StorageServiceFileWriteAnswerMessage(
                                location,
                                false,
                                std::make_shared<StorageServiceNotEnoughSpace>(
                                    location->getFile(),
                                    this->getSharedPtr<SimpleStorageService>()),
                                {},
                                0,
                                this->getMessagePayloadValue(
                                        SimpleStorageServiceMessagePayload::FILE_WRITE_ANSWER_MESSAGE_PAYLOAD)));
            } catch (wrench::ExecutionException &) {
                return true;
            }
            return true;
        }

//        // Create directory if need be
//        if (not this->file_system->directory_exists(location->getDirectoryPath())) {
//            this->file_system->create_directory(location->getDirectoryPath());
//        }

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
        auto me_disk = location->getDiskOrNull();

        // Create a Transaction
        auto transaction = std::make_shared<Transaction>(
                nullptr,
                nullptr,
                requesting_host,
                nullptr,
                location,
                opened_file,
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
            sg_size_t num_bytes_to_read,
            S4U_CommPort *answer_commport,
            simgrid::s4u::Host *requesting_host) {

        std::shared_ptr<simgrid::fsmod::File> opened_file;
        auto failure_cause = validateFileReadRequest(location, opened_file);
        bool success = (failure_cause == nullptr);

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
            return true;// oh well
        }


        // If success, then follow up with sending the file (ASYNCHRONOUSLY!)
        if (success) {
            // Create the transaction
            auto me_host = simgrid::s4u::this_actor::get_host();
            auto me_disk = location->getDiskOrNull();
            auto transaction = std::make_shared<Transaction>(
                    location,
                    opened_file,
                    me_host,
                    me_disk,
                    nullptr,
                    nullptr,
                    requesting_host,
                    nullptr,
                    answer_commport,
                    num_bytes_to_read);

            // Add it to the Pool of pending data communications
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
//    bool SimpleStorageServiceNonBufferized::processFileCopyRequestIAmNotTheSource(
    bool SimpleStorageServiceNonBufferized::processFileCopyRequest(
            std::shared_ptr<FileLocation> &src_location,
            std::shared_ptr<FileLocation> &dst_location,
            S4U_CommPort *answer_commport) {

        WRENCH_INFO("FileCopyRequest: %s -> %s",
                    src_location->toString().c_str(),
                    dst_location->toString().c_str());

        std::shared_ptr<simgrid::fsmod::File> src_opened_file;
        std::shared_ptr<simgrid::fsmod::File> dst_opened_file;
        auto failure_cause = validateFileCopyRequest(src_location, dst_location, src_opened_file, dst_opened_file);

        if (failure_cause) {
            this->simulation_->getOutput().addTimestampFileCopyFailure(Simulation::getCurrentSimulatedDate(), src_location->getFile(), src_location, dst_location);
            try {
                answer_commport->dputMessage(
                        new StorageServiceFileCopyAnswerMessage(
                                src_location,
                                dst_location,
                                false,
                                failure_cause,
                                this->getMessagePayloadValue(
                                        SimpleStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD)));
            } catch (ExecutionException &e) {
                return true;
            }
            return true;
        }

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

        uint64_t transfer_size;
        transfer_size = (uint64_t) (src_location->getFile()->getSize());
        auto transaction = std::make_shared<Transaction>(
                src_location,
                src_opened_file,
                src_host,
                src_disk,
                dst_location,
                dst_opened_file,
                dst_host,
                dst_disk,
                answer_commport,
                transfer_size);

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

            // TODO: Sadly we cannot do this with simgrid::fsmod...
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
        return static_cast<double>(this->running_transactions.size()) + static_cast<double>(this->pending_transactions.size());
    }


}// namespace wrench
