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
#include <wrench/services/storage/storage_helpers/FileTransferThreadMessage.h>
#include <wrench/failure_causes/InvalidDirectoryPath.h>
#include <wrench/failure_causes/FileNotFound.h>
#include <wrench/failure_causes/StorageServiceNotEnoughSpace.h>
#include <wrench/failure_causes/NetworkError.h>

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

//    /**
//    * @brief Generate a unique number
//    *
//    * @return a unique number
//    */
//    unsigned long SimpleStorageServiceNonBufferized::getNewUniqueNumber() {
//        static unsigned long sequence_number = 0;
//        return (sequence_number++);
//    }

//    /**
//     * @brief Destructor
//     */
//    SimpleStorageServiceNonBufferized::~SimpleStorageServiceNonBufferized() {
//        this->default_property_values.clear();
//    }

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
                                 "_" + std::to_string(SimpleStorageService::getNewUniqueNumber())) {}

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
        while (true) {
            S4U_Simulation::computeZeroFlop();

            this->startPendingTransactions();

            // Create an async recv
            simgrid::s4u::CommPtr comm_ptr;
            std::unique_ptr<SimulationMessage> simulation_message;
            try {
                comm_ptr = this->mailbox->get_async<void>((void **) (&(simulation_message)));
            } catch (simgrid::NetworkFailureException &e) {
                // oh well
                continue;
            }
            // Create all activities to wait on
            std::vector<simgrid::s4u::ActivityPtr> pending_activities;
            pending_activities.push_back(comm_ptr);
            for (auto const &stream : this->running_sg_iostreams) {
                pending_activities.push_back(stream);
            }
            // Wait for the first one to complete
            ssize_t activity_index = simgrid::s4u::Activity::wait_any(pending_activities);

            // It's a communication
            if (activity_index == 0) {
                processNextMessage(simulation_message.get());
            } else if (activity_index > 0) {
                throw std::runtime_error("TODO: REACT TO A PENDING STREAM COMPLETING");
            } else if (activity_index == -1) {
                throw std::runtime_error("TODO: REACT TO WAIT_ANY RETURNING -1");
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

//            // Generate a mailbox_name name on which to receive the file
//            auto file_reception_mailbox = S4U_Mailbox::getTemporaryMailbox();
//            //            auto file_reception_mailbox = S4U_Mailbox::generateUniqueMailbox("faa_does_not_work");

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
            simgrid::s4u::Disk *me_disk = nullptr;
            for (auto const &d: me_host->get_disks()) {
                if (d->get_property("mount") == location->getMountPoint()) {
                    me_disk = d;
                }
            }
            if (me_disk == nullptr) {
                throw std::runtime_error("SimpleStorageServiceNonBufferized::processFileWriteRequest(): disk not found - internal error");
            }


            WRENCH_INFO("Creating Streaming Activity for a write request");
            auto sg_iostream = simgrid::s4u::Io::streamto_init(requesting_host,
                                                               nullptr,
                                                               me_host,
                                                               me_disk)->set_size((uint64_t)file->getSize());

            // Create a Transaction
            auto transaction = std::make_shared<Transaction>(
                    sg_iostream,
                    file,
                    answer_mailbox);

//            // Create a FileTransferThread
//            auto ftt = std::shared_ptr<FileTransferThread>(
//                    new FileTransferThread(this->hostname,
//                                           this->getSharedPtr<StorageService>(),
//                                           file,
//                                           file->getSize(),
//                                           file_reception_mailbox,
//                                           location,
//                                           nullptr,
//                                           answer_mailbox,
//                                           nullptr,
//                                           buffer_size));
//            ftt->setSimulation(this->simulation);

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

        //        if ((this->file_systems.find(location->getMountPoint()) == this->file_systems.end()) or
        if (not this->file_systems[location->getMountPoint()]->doesDirectoryExist(
                location->getAbsolutePathAtMountPoint())) {
            failure_cause = std::shared_ptr<FailureCause>(
                    new InvalidDirectoryPath(
                            this->getSharedPtr<SimpleStorageService>(),
                            location->getMountPoint() + "/" +
                            location->getAbsolutePathAtMountPoint()));
        } else {
            auto fs = this->file_systems[location->getMountPoint()].get();

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
            simgrid::s4u::Disk *me_disk = nullptr;
            for (auto const &d: me_host->get_disks()) {
                if (d->get_property("mount") == location->getMountPoint()) {
                    me_disk = d;
                }
            }
            if (me_disk == nullptr) {
                throw std::runtime_error("SimpleStorageServiceNonBufferized::processFileReadRequest(): disk not found - internal error");
            }

            WRENCH_INFO("Creating Streaming Activity for a read request");
//            auto sg_iostream = simgrid::s4u::Io::streamto_init(me_host,
//                                                               me_disk,
//                                                               requesting_host,
//                                                               nullptr)->set_size((uint64_t)file->getSize());

//            std::cerr << me_host->get_cname() << "\n";
//            std::cerr << me_disk->get_cname() << "\n";
//            std::cerr << requesting_host->get_cname() << "\n";
            auto sg_iostream = simgrid::s4u::Io::streamto_init(me_host,
                                                               me_disk,
                                                               requesting_host,
                                                               nullptr)->set_size(666);

            WRENCH_INFO("SYnCHRONOUS!");
            simgrid::s4u::Io::streamto(me_host,
                                       me_disk,
                                       requesting_host,
                                       nullptr, 666 );

            WRENCH_INFO("STARTED IT NOW JUST TO TRY");

            // Create a Transaction
            auto transaction = std::make_shared<Transaction>(
                    sg_iostream,
                    file,
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

        auto fs = this->file_systems[dst_location->getMountPoint()].get();

        // File is not already here
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

        WRENCH_INFO("Starting an activity to copy file %s from %s to %s",
                    file->getID().c_str(),
                    src_location->toString().c_str(),
                    dst_location->toString().c_str());

        // Create the streaming activity
        std::cerr << src_location->getServerStorageService() << "\n";
        std::cerr << src_location->getServerStorageService()->getName() << "\n";
        std::cerr << src_location->getServerStorageService()->getHostname() << "\n";
        auto src_host = simgrid::s4u::Host::by_name(src_location->getServerStorageService()->getHostname());
        simgrid::s4u::Disk *src_disk = nullptr;
        for (auto const &d: src_host->get_disks()) {
            if (d->get_property("mount") == src_location->getMountPoint()) {
                src_disk = d;
            }
        }
        if (src_disk == nullptr) {
            throw std::runtime_error("SimpleStorageServiceNonBufferized::processFileCopyRequest(): disk not found - internal error");
        }
        auto dst_host = simgrid::s4u::Host::by_name(dst_location->getServerStorageService()->getHostname());
        simgrid::s4u::Disk *dst_disk = nullptr;
        for (auto const &d: dst_host->get_disks()) {
            if (d->get_property("mount") == dst_location->getMountPoint()) {
                dst_disk = d;
            }
        }
        if (dst_disk == nullptr) {
            throw std::runtime_error("SimpleStorageServiceNonBufferized::processFileCopyRequest(): disk not found - internal error");
        }

        auto sg_iostream = simgrid::s4u::Io::streamto_init(src_host,
                                                           src_disk,
                                                           dst_host,
                                                           dst_disk)->set_size((uint64_t)file->getSize());

        // Create a Transaction
        auto transaction = std::make_shared<Transaction>(
                sg_iostream,
                file,
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
        WRENCH_INFO("Starting pending transactions");
        while ((not this->pending_sg_iostreams.empty()) and
               (this->running_sg_iostreams.size() < this->num_concurrent_connections)) {
            auto sg_iostream = this->pending_sg_iostreams.at(0);
            this->pending_sg_iostreams.pop_front();
            this->running_sg_iostreams.insert(sg_iostream);
            WRENCH_INFO("Starting an IO stream!");
            sg_iostream->vetoable_start();
            WRENCH_INFO("IO stream started!");
        }
    }

//    /**
//     * @brief Process a notification received from a file transfer thread
//     * @param ftt: the file transfer thread
//     * @param file: the file
//     * @param src_mailbox: the transfer's source mailbox (or "" if source was not a mailbox)
//     * @param src_location: the transfer's source location (or nullptr if source was not a location)
//     * @param dst_mailbox: the transfer's destination mailbox (or "" if source was not a mailbox)
//     * @param dst_location: the transfer's destination location (or nullptr if destination was not a location)
//     * @param success: whether the transfer succeeded or not
//     * @param failure_cause: the failure cause (nullptr if success)
//     * @param answer_mailbox_if_read: the mailbox to send a read notification ("" if not a copy)
//     * @param answer_mailbox_if_write: the mailbox to send a write notification ("" if not a copy)
//     * @param answer_mailbox_if_copy: the mailbox to send a copy notification ("" if not a copy)
//     * @return false if the daemon should terminate
//     */
//    bool SimpleStorageServiceNonBufferized::processFileTransferThreadNotification(const std::shared_ptr<FileTransferThread> &ftt,
//                                                                                  const std::shared_ptr<DataFile> &file,
//                                                                                  simgrid::s4u::Mailbox *src_mailbox,
//                                                                                  const std::shared_ptr<FileLocation> &src_location,
//                                                                                  simgrid::s4u::Mailbox *dst_mailbox,
//                                                                                  const std::shared_ptr<FileLocation> &dst_location,
//                                                                                  bool success,
//                                                                                  std::shared_ptr<FailureCause> failure_cause,
//                                                                                  simgrid::s4u::Mailbox *answer_mailbox_if_read,
//                                                                                  simgrid::s4u::Mailbox *answer_mailbox_if_write,
//                                                                                  simgrid::s4u::Mailbox *answer_mailbox_if_copy) {
//        // Remove the ftt from the list of running ftt
//        if (this->running_file_transfer_threads.find(ftt) == this->running_file_transfer_threads.end()) {
//            WRENCH_INFO(
//                    "Got a notification from a non-existing File Transfer Thread. Perhaps this is from a former life... ignoring");
//        } else {
//            this->running_file_transfer_threads.erase(ftt);
//        }
//
//        // Was the destination me?
//        if (dst_location and (dst_location->getStorageService().get() == this)) {
//            if (success) {
//                WRENCH_INFO("File %s stored!", file->getID().c_str());
//                this->file_systems[dst_location->getMountPoint()]->storeFileInDirectory(
//                        file, dst_location->getAbsolutePathAtMountPoint());
//                // Deal with time stamps, previously we could test whether a real timestamp was passed, now this.
//                // May be no corresponding timestamp.
//                try {
//                    this->simulation->getOutput().addTimestampFileCopyCompletion(Simulation::getCurrentSimulatedDate(), file, src_location, dst_location);
//                } catch (invalid_argument &ignore) {
//                }
//
//            } else {
//                // Process the failure, meaning, just un-decrease the free space
//                this->file_systems[dst_location->getMountPoint()]->unreserveSpace(
//                        file, dst_location->getAbsolutePathAtMountPoint());
//            }
//        }
//
//        // Send back the relevant ack if this was a read
//        if (answer_mailbox_if_read and success) {
//            WRENCH_INFO(
//                    "Sending back an ack since this was a file read and some client is waiting for me to say something");
//            S4U_Mailbox::dputMessage(answer_mailbox_if_read, new StorageServiceAckMessage());
//        }
//
//        // Send back the relevant ack if this was a write
//        if (answer_mailbox_if_write and success) {
//            WRENCH_INFO(
//                    "Sending back an ack since this was a file write and some client is waiting for me to say something");
//            S4U_Mailbox::dputMessage(answer_mailbox_if_write, new StorageServiceAckMessage());
//        }
//
//        // Send back the relevant ack if this was a copy
//        if (answer_mailbox_if_copy) {
//            WRENCH_INFO(
//                    "Sending back an ack since this was a file copy and some client is waiting for me to say something");
//            if ((src_location == nullptr) or (dst_location == nullptr)) {
//                throw std::runtime_error("SimpleStorageServiceNonBufferized::processFileTransferThreadNotification(): "
//                                         "src_location and dst_location must be non-null");
//            }
//            S4U_Mailbox::dputMessage(
//                    answer_mailbox_if_copy,
//                    new StorageServiceFileCopyAnswerMessage(
//                            file,
//                            src_location,
//                            dst_location,
//                            nullptr,
//                            false,
//                            success,
//                            std::move(failure_cause),
//                            this->getMessagePayloadValue(
//                                    SimpleStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD)));
//        }
//
//        return true;
//    }

//    /**
//     * @brief Process a file deletion request
//     * @param file: the file to delete
//     * @param location: the file location
//     * @param answer_mailbox: the mailbox to which the notification should be sent
//     * @return false if the daemon should terminate
//     */
//    bool SimpleStorageServiceNonBufferized::processFileDeleteRequest(const std::shared_ptr<DataFile> &file,
//                                                                     const std::shared_ptr<FileLocation> &location,
//                                                                     simgrid::s4u::Mailbox *answer_mailbox) {
//        std::shared_ptr<FailureCause> failure_cause = nullptr;
//
//        auto fs = this->file_systems[location->getMountPoint()].get();
//
//        if ((not fs->doesDirectoryExist(location->getAbsolutePathAtMountPoint())) or
//            (not fs->isFileInDirectory(file, location->getAbsolutePathAtMountPoint()))) {
//            // If this is scratch, we don't care, perhaps it was taken care of elsewhere...
//            if (not this->isScratch()) {
//                failure_cause = std::shared_ptr<FailureCause>(
//                        new FileNotFound(file, location));
//            }
//        } else {
//            fs->removeFileFromDirectory(file, location->getAbsolutePathAtMountPoint());
//        }
//
//        S4U_Mailbox::dputMessage(
//                answer_mailbox,
//                new StorageServiceFileDeleteAnswerMessage(
//                        file,
//                        this->getSharedPtr<SimpleStorageService>(),
//                        (failure_cause == nullptr),
//                        failure_cause,
//                        this->getMessagePayloadValue(
//                                SimpleStorageServiceMessagePayload::FILE_DELETE_ANSWER_MESSAGE_PAYLOAD)));
//        return true;
//    }
//
//    /**
//     * @brief Helper method to validate property values
//     * throw std::invalid_argument
//     */
//    void SimpleStorageServiceNonBufferized::validateProperties() {
//        this->getPropertyValueAsUnsignedLong(SimpleStorageServiceProperty::MAX_NUM_CONCURRENT_DATA_CONNECTIONS);
//        this->getPropertyValueAsSizeInByte(SimpleStorageServiceProperty::BUFFER_SIZE);
//    }

//    /**
//     * @brief Get number of File Transfer Threads that are currently running or are pending
//     * @return The number of threads
//     */
//    double SimpleStorageServiceNonBufferized::countRunningFileTransferThreads() {
//        return this->running_file_transfer_threads.size() + this->pending_file_transfer_threads.size();
//    }

    /**
     * @brief Get the load (number of concurrent reads) on the storage service
     * @return the load on the service
     */
    double SimpleStorageServiceNonBufferized::getLoad() {
        // TODO: TO RE-IMPLE<MENT FOR NON-BUFFERIZED
        return 0.0;
    }

//    /**
//     * @brief Get a file's last write date at a location (in zero simulated time)
//     *
//     * @param file: the file
//     * @param location: the file location
//     *
//     * @return the file's last write date, or -1 if the file is not found
//     *
//     */
//    double SimpleStorageServiceNonBufferized::getFileLastWriteDate(const std::shared_ptr<DataFile> &file, const std::shared_ptr<FileLocation> &location) {
//        if ((file == nullptr) or (location == nullptr)) {
//            throw std::invalid_argument("SimpleStorageServiceNonBufferized::getFileLastWriteDate(): Invalid arguments");
//        }
//        auto fs = this->file_systems[location->getMountPoint()].get();
//        return fs->getFileLastWriteDate(file, location->getAbsolutePathAtMountPoint());
//    }

};// namespace wrench
