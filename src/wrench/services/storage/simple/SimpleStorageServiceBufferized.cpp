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
#include <wrench/simgrid_S4U_util/S4U_CommPort.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/simgrid_S4U_util/S4U_Simulation.h>
#include <wrench/data_file/DataFile.h>
#include <wrench/exceptions/ExecutionException.h>
#include <wrench/simulation/SimulationTimestampTypes.h>
#include <wrench/services/storage/storage_helpers/FileLocation.h>
#include <wrench/services/memory/MemoryManager.h>

namespace sgfs = simgrid::fsmod;

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
        //        this->release_held_mutexes();
        for (auto const &it: this->ongoing_tmp_commports) {
            S4U_CommPort::retireTemporaryCommPort(it.second);
        }
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
                                                                   const std::set<std::string>& mount_points,
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
        for (auto const &part: this->file_system->get_partitions()) {
            message = "  - mount point " + part->get_name() + ": " +
                      std::to_string(part->get_free_space()) + "/" +
                      std::to_string(part->get_size()) + " Bytes";
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

        // In case this was a restart!
        this->commport->reset();
        this->recv_commport->reset();

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

        WRENCH_INFO("BUFFERISZED WATOING ON COMMPORT: %s", this->commport->get_cname());
        try {
            message = this->commport->getMessage();
        } catch (ExecutionException &e) {
            WRENCH_INFO("Got a network error while getting some message... ignoring");
            return true;
        }

        WRENCH_DEBUG("Got a [%s] message", message->getName().c_str());

        if (auto msg = std::dynamic_pointer_cast<ServiceStopDaemonMessage>(message)) {
            return processStopDaemonRequest(msg->ack_commport);

        } else if (auto msg = std::dynamic_pointer_cast<StorageServiceFreeSpaceRequestMessage>(message)) {
            return processFreeSpaceRequest(msg->answer_commport, msg->path);

        } else if (auto msg = std::dynamic_pointer_cast<StorageServiceFileDeleteRequestMessage>(message)) {
            return processFileDeleteRequest(msg->location, msg->answer_commport);

        } else if (auto msg = std::dynamic_pointer_cast<StorageServiceFileLookupRequestMessage>(message)) {
            return processFileLookupRequest(msg->location, msg->answer_commport);

        } else if (auto msg = std::dynamic_pointer_cast<StorageServiceFileWriteRequestMessage>(message)) {
            return processFileWriteRequest(msg->location, msg->num_bytes_to_write, msg->answer_commport);

        } else if (auto msg = std::dynamic_pointer_cast<StorageServiceFileReadRequestMessage>(message)) {
            return processFileReadRequest(msg->location, msg->num_bytes_to_read, msg->answer_commport);

        } else if (auto msg = std::dynamic_pointer_cast<StorageServiceFileCopyRequestMessage>(message)) {
            return processFileCopyRequest(msg->src, msg->dst, msg->answer_commport);

        } else if (auto msg = std::dynamic_pointer_cast<FileTransferThreadNotificationMessage>(message)) {
            return processFileTransferThreadNotification(
                    msg->file_transfer_thread,
                    msg->src_commport,
                    msg->src_location,
                    msg->dst_commport,
                    msg->dst_location,
                    msg->success,
                    msg->failure_cause,
                    msg->answer_commport_if_read,
                    msg->answer_commport_if_write,
                    msg->answer_commport_if_copy);
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
     * @param answer_commport: the commport to which the reply should be sent
     * @return true if this process should keep running
     */
    bool SimpleStorageServiceBufferized::processFileWriteRequest(std::shared_ptr<FileLocation> &location,
                                                                 double num_bytes_to_write,
                                                                 S4U_CommPort *answer_commport) {


        std::shared_ptr<simgrid::fsmod::File> opened_file;
        auto failure_cause = validateFileWriteRequest(location, num_bytes_to_write, opened_file);


        if (failure_cause) {
            try {
                answer_commport->dputMessage(
                        new StorageServiceFileWriteAnswerMessage(
                                location,
                                false,
                                failure_cause,
                                {},
                                0,
                                this->getMessagePayloadValue(
                                        SimpleStorageServiceMessagePayload::FILE_WRITE_ANSWER_MESSAGE_PAYLOAD)));
            } catch (wrench::ExecutionException &e) {
                return true;
            }
            return true;
        }

//        // Create directory if need be
//        if (not this->file_system->directory_exists(location->getDirectoryPath())) {
//            this->file_system->create_directory(location->getDirectoryPath());
//        }

        // Generate a commport name on which to receive the file
        auto file_reception_commport = S4U_CommPort::getTemporaryCommPort();

        // Reply with a "go ahead, send me the file" message
        answer_commport->dputMessage(
                new StorageServiceFileWriteAnswerMessage(
                        location,
                        true,
                        nullptr,
                        {{file_reception_commport, location->getFile()->getSize()}},
                        this->buffer_size,
                        this->getMessagePayloadValue(
                                SimpleStorageServiceMessagePayload::FILE_WRITE_ANSWER_MESSAGE_PAYLOAD)));


        std::cerr << "BEFORE STARTING FT THREAD: " << this->file_system->get_partitions().at(0)->get_free_space() << "\n";


        // Create a FileTransferThread
        auto ftt = std::make_shared<FileTransferThread>(
                this->hostname,
                this->getSharedPtr<StorageService>(),
                location->getFile(),
                num_bytes_to_write,
                file_reception_commport,
                location,
                opened_file,
                nullptr,
                answer_commport,
                nullptr,
                this->buffer_size);
        ftt->setSimulation(this->simulation);

        // Add it to the Pool of pending data communications
        this->pending_file_transfer_threads.push_back(ftt);
        // Keep track of the commport as well
        this->ongoing_tmp_commports[ftt] = file_reception_commport;

        return true;
    }

    /**
     * @brief Handle a file read request
     * @param location: the file's location
     * @param num_bytes_to_read: the number of bytes to read
     * @param answer_commport: the commport to which the answer should be sent
     * @return
     */
    bool SimpleStorageServiceBufferized::processFileReadRequest(
            const std::shared_ptr<FileLocation> &location,
            double num_bytes_to_read,
            S4U_CommPort *answer_commport) {

        std::shared_ptr<simgrid::fsmod::File> opened_file;
        auto failure_cause = validateFileReadRequest(location, opened_file);

        bool success = (failure_cause == nullptr);

        S4U_CommPort *commport_to_receive_the_file_content = nullptr;
        if (success) {
            commport_to_receive_the_file_content = S4U_CommPort::getTemporaryCommPort();
        }

        // Send back the corresponding ack
        try {
            answer_commport->putMessage(
                    new StorageServiceFileReadAnswerMessage(
                            location,
                            success,
                            failure_cause,
                            commport_to_receive_the_file_content,
                            buffer_size,
                            1,
                            this->getMessagePayloadValue(StorageServiceMessagePayload::FILE_READ_ANSWER_MESSAGE_PAYLOAD)));
        } catch (ExecutionException &e) {
            return true;// oh well
        }

        // If success, then follow up with sending the file (ASYNCHRONOUSLY!)
        if (success) {
            // Create a FileTransferThread
            auto ftt = std::make_shared<FileTransferThread>(
                    this->hostname,
                    this->getSharedPtr<StorageService>(),
                    location->getFile(),
                    num_bytes_to_read,
                    location,
                    opened_file,
                    commport_to_receive_the_file_content,
                    answer_commport,
                    nullptr,
                    nullptr,
                    buffer_size);
            ftt->setSimulation(this->simulation);

            // Add it to the Pool of pending data communications
            this->pending_file_transfer_threads.push_front(ftt);
            // Keep track of the commport as well
            this->ongoing_tmp_commports[ftt] = commport_to_receive_the_file_content;
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
    bool SimpleStorageServiceBufferized::processFileCopyRequest(
            std::shared_ptr<FileLocation> &src_location,
            std::shared_ptr<FileLocation> &dst_location,
            S4U_CommPort *answer_commport) {

        std::shared_ptr<simgrid::fsmod::File> src_opened_file;
        std::shared_ptr<simgrid::fsmod::File> dst_opened_file;

        auto failure_cause = validateFileCopyRequest(src_location, dst_location, src_opened_file, dst_opened_file);

        if (failure_cause) {
            this->simulation->getOutput().addTimestampFileCopyFailure(Simulation::getCurrentSimulatedDate(), src_location->getFile(), src_location, dst_location);
            try {
                answer_commport->putMessage(
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


        WRENCH_INFO("Starting a thread to copy file %s from %s to %s",
                    src_location->getFile()->getID().c_str(),
                    src_location->toString().c_str(),
                    dst_location->toString().c_str());

        // Create a file transfer thread
        uint64_t transfer_size;
        transfer_size = (uint64_t) (src_location->getFile()->getSize());

        auto ftt = std::make_shared<FileTransferThread>(
                this->hostname,
                this->getSharedPtr<StorageService>(),
                src_location->getFile(),
                transfer_size,
                src_location,
                src_opened_file,
                dst_location,
                dst_opened_file,
                nullptr,
                nullptr,
                answer_commport,
                this->buffer_size);
        ftt->setSimulation(this->simulation);
        this->pending_file_transfer_threads.push_back(ftt);

//        src_location->getStorageService()->incrementNumRunningOperationsForLocation(src_location);

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
     * @param src_commport: the transfer's source commport (or "" if source was not a commport)
     * @param src_location: the transfer's source location (or nullptr if source was not a location)
     * @param dst_commport: the transfer's destination commport (or "" if source was not a commport)
     * @param dst_location: the transfer's destination location (or nullptr if destination was not a location)
     * @param success: whether the transfer succeeded or not
     * @param failure_cause: the failure cause (nullptr if success)
     * @param answer_commport_if_read: the commport to send a read notification ("" if not a copy)
     * @param answer_commport_if_write: the commport to send a write notification ("" if not a copy)
     * @param answer_commport_if_copy: the commport to send a copy notification ("" if not a copy)
     * @return false if the daemon should terminate
     */
    bool SimpleStorageServiceBufferized::processFileTransferThreadNotification(const std::shared_ptr<FileTransferThread> &ftt,
                                                                               S4U_CommPort *src_commport,
                                                                               const std::shared_ptr<FileLocation> &src_location,
                                                                               S4U_CommPort *dst_commport,
                                                                               const std::shared_ptr<FileLocation> &dst_location,
                                                                               bool success,
                                                                               std::shared_ptr<FailureCause> failure_cause,
                                                                               S4U_CommPort *answer_commport_if_read,
                                                                               S4U_CommPort *answer_commport_if_write,
                                                                               S4U_CommPort *answer_commport_if_copy) {

        // Remove the ftt from the list of running ftt
        if (this->running_file_transfer_threads.find(ftt) == this->running_file_transfer_threads.end()) {
            WRENCH_INFO(
                    "Got a notification from a non-existing File Transfer Thread. Perhaps this is from a former life... ignoring");
        } else {
            this->running_file_transfer_threads.erase(ftt);
        }

        // Retire the temporary comport associated to the ftt
        if (this->ongoing_tmp_commports.find(ftt) != this->ongoing_tmp_commports.end()) {
            S4U_CommPort::retireTemporaryCommPort(this->ongoing_tmp_commports[ftt]);
            this->ongoing_tmp_commports.erase(ftt);
        }

        // Retire the commports of the ftt! ( already done elsewhere)
        // S4U_CommPort::retireTemporaryCommPort(ftt->commport);
        // S4U_CommPort::retireTemporaryCommPort(ftt->recv_commport);

        // Deal with possibly open source file
        if (ftt->src_opened_file) {
            ftt->src_opened_file->close();
        }
        // Deal with possibly opened destination file
        if (ftt->dst_opened_file) {
            auto dst_file_system = ftt->dst_opened_file->get_file_system();
            auto dst_file_path = ftt->dst_opened_file->get_path();
            ftt->dst_opened_file->close();
            if (not dst_file_system->file_exists(ftt->dst_location->getFilePath())) {
                std::cerr << ftt->dst_location->getStorageService()->getName() << ": MOVING  " << dst_file_path << " TO " << ftt->dst_location->getFilePath() << "\n";
                dst_file_system->move_file(dst_file_path, ftt->dst_location->getFilePath());
            }
        }

#if 0
        if (success) {
//                WRENCH_INFO("File %s stored!", file->getID().c_str());
//                this->file_systems[dst_mount_point]->storeFileInDirectory(
//                        file, dst_path_at_mount_point);
            // Deal with time stamps, previously we could test whether a real timestamp was passed, now this.
            // Maybe no corresponding timestamp.
            try {
                this->simulation->getOutput().addTimestampFileCopyCompletion(Simulation::getCurrentSimulatedDate(),
                                                                             src_location->getFile(), src_location, dst_location);
            } catch (invalid_argument &ignore) {
            }
        }

        // Send back the relevant ack if this was a read
        if (answer_commport_if_read and success) {
            //            WRENCH_INFO(
            //                    "Sending back an ack since this was a file read and some client is waiting for me to say something");
            answer_commport_if_read->dputMessage(new StorageServiceAckMessage(src_location));
        }

        // Send back the relevant ack if this was a write operation
        if (answer_commport_if_write and success) {
            //            WRENCH_INFO(
            //                    "Sending back an ack since this was a file write and some client is waiting for me to say something");
            answer_commport_if_write->dputMessage(new StorageServiceAckMessage(dst_location));
        }

        // Send back the relevant ack if this was a copy
        if (answer_commport_if_copy) {
            //            WRENCH_INFO(
            //                    "Sending back an ack since this was a file copy and some client is waiting for me to say something");
            if ((src_location == nullptr) or (dst_location == nullptr)) {
                throw std::runtime_error("SimpleStorageServiceBufferized::processFileTransferThreadNotification(): "
                                         "src_location and dst_location must be non-null");
            }
            answer_commport_if_copy->dputMessage(
                    new StorageServiceFileCopyAnswerMessage(
                            src_location,
                            dst_location,
                            success,
                            std::move(failure_cause),
                            this->getMessagePayloadValue(
                                    SimpleStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD)));
        }
#endif

        // Send back the relevant ack if this was a read
        if (ftt->dst_location == nullptr) {
            //            WRENCH_INFO("Sending back an ack for a successful file read");
            ftt->answer_commport_if_read->dputMessage(new StorageServiceAckMessage(ftt->src_location));
        } else if (ftt->src_location == nullptr) {
//            StorageService::createFileAtLocation(ftt->dst_location);
            WRENCH_INFO("File %s stored", ftt->dst_location->getFile()->getID().c_str());
            // Deal with time stamps, previously we could test whether a real timestamp was passed, now this.
            // Maybe no corresponding timestamp.
            //            WRENCH_INFO("Sending back an ack for a successful file read");
            ftt->answer_commport_if_write->dputMessage(new StorageServiceAckMessage(ftt->dst_location));
        } else {
            if (ftt->dst_location->getStorageService() == shared_from_this()) {
//                this->createFile(ftt->dst_location);
                WRENCH_INFO("File %s stored", ftt->dst_location->getFile()->getID().c_str());
                try {
                    this->simulation->getOutput().addTimestampFileCopyCompletion(
                            Simulation::getCurrentSimulatedDate(), ftt->dst_location->getFile(), ftt->src_location, ftt->dst_location);
                } catch (invalid_argument &ignore) {
                }
            }
            std::cerr << "SENDING BACK FILE COPY\n";

            //            WRENCH_INFO("Sending back an ack for a file copy");
            ftt->answer_commport_if_copy->dputMessage(
                    new StorageServiceFileCopyAnswerMessage(
                            ftt->src_location,
                            ftt->dst_location,
                            true,
                            nullptr,
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
