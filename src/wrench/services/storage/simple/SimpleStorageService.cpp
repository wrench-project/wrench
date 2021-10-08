/**
 * Copyright (c) 2017-2020. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <climits>
#include <services/storage/storage_helper_classes/FileTransferThreadMessage.h>
#include <wrench/workflow/failure_causes/InvalidDirectoryPath.h>
#include <wrench/workflow/failure_causes/FileNotFound.h>
#include <wrench/workflow/failure_causes/StorageServiceNotEnoughSpace.h>
#include <wrench/workflow/failure_causes/NetworkError.h>

#include "wrench/services/storage/simple/SimpleStorageService.h"
#include "wrench/services/ServiceMessage.h"
#include "services/storage/StorageServiceMessage.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "wrench/workflow/WorkflowFile.h"
#include "wrench/exceptions/WorkflowExecutionException.h"
#include "wrench/simulation/SimulationTimestampTypes.h"
#include "wrench/services/storage/storage_helpers/FileLocation.h"
#include "wrench/services/memory/MemoryManager.h"

WRENCH_LOG_CATEGORY(wrench_core_simple_storage_service,
                    "Log category for Simple Storage Service");

namespace wrench {

    /**
    * @brief Generate a unique number
    *
    * @return a unique number
    */
    unsigned long SimpleStorageService::getNewUniqueNumber() {
        static unsigned long sequence_number = 0;
        return (sequence_number++);
    }

    /**
     * @brief Destructor
     */
    SimpleStorageService::~SimpleStorageService() {
        this->default_property_values.clear();
    }

    /**
     * @brief Cleanup method
     *
     * @param has_returned_from_main: whether main() returned
     * @param return_value: the return value (if main() returned)
     */
    void SimpleStorageService::cleanup(bool has_returned_from_main, int return_value) {
        this->pending_file_transfer_threads.clear();
        this->running_file_transfer_threads.clear();
        // Do nothing. It's fine to die and we'll just autorestart with our previous state
    }

    /**
     * @brief Public constructor
     *
     * @param hostname: the name of the host on which to start the service
     * @param mount_points: the set of mount points
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     */
    SimpleStorageService::SimpleStorageService(std::string hostname,
                                               std::set <std::string> mount_points,
                                               std::map <std::string, std::string> property_list,
                                               std::map<std::string, double> messagepayload_list) :
            SimpleStorageService(std::move(hostname), mount_points, property_list, messagepayload_list,
                                 "_" + std::to_string(getNewUniqueNumber())) {}

    /**
     * @brief Private constructor
     *
     * @param hostname: the name of the host on which to start the service
     * @param mount_points: the set of mount points
     * @param property_list: the property list
     * @param suffix: the suffix (for the service name)
     *
     * @throw std::invalid_argument
     */
    SimpleStorageService::SimpleStorageService(
            std::string hostname,
            std::set <std::string> mount_points,
            std::map <std::string, std::string> property_list,
            std::map<std::string, double> messagepayload_list,
            std::string suffix) :
            StorageService(std::move(hostname), mount_points, "simple_storage" + suffix, "simple_storage" + suffix) {
        this->setProperties(this->default_property_values, property_list);
        this->setMessagePayloads(this->default_messagepayload_values, messagepayload_list);
        this->validateProperties();

        this->num_concurrent_connections = this->getPropertyValueAsUnsignedLong(
                SimpleStorageServiceProperty::MAX_NUM_CONCURRENT_DATA_CONNECTIONS);
        this->buffer_size = this->getPropertyValueAsUnsignedLong(StorageServiceProperty::BUFFER_SIZE);
    }

    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int SimpleStorageService::main() {
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_CYAN);

        // "Start" all logical file systems
        for (auto const &fs : this->file_systems) {
            fs.second->init();
        }

        std::string message = "Simple Storage service " + this->getName() + "  starting on host " + this->getHostname();
        WRENCH_INFO("%s", message.c_str());
        for (auto const &fs : this->file_systems) {
            message = "  - mount point " + fs.first + ": " +
                      std::to_string(fs.second->getFreeSpace()) + "/" +
                      std::to_string(fs.second->getTotalCapacity()) + " Bytes";
            WRENCH_INFO("%s", message.c_str())
        }

        // If writeback device simulation is activated
        if (Simulation::isPageCachingEnabled()) {
            //  Find the "memory" disk (we know there is one)
            auto host = simgrid::s4u::Host::by_name(this->getHostname());
            simgrid::s4u::Disk *memory_disk = nullptr;
            for (auto const &d : host->get_disks()) {
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
            this->memory_manager = MemoryManager::initAndStart(this->simulation, memory_disk,0.4, 5, 30, this->hostname);
//            memory_manager_ptr->log();
        }

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
    bool SimpleStorageService::processNextMessage() {
        S4U_Simulation::computeZeroFlop();

        // Wait for a message
        std::shared_ptr <SimulationMessage> message = nullptr;

        try {
            message = S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr <NetworkError> &cause) {
            WRENCH_INFO("Got a network error while getting some message... ignoring");
            return true; // oh well
        }

        WRENCH_DEBUG("Got a [%s] message", message->getName().c_str());

        if (auto msg = dynamic_cast<ServiceStopDaemonMessage*>(message.get())) {
            try {
                S4U_Mailbox::putMessage(msg->ack_mailbox,
                                        new ServiceDaemonStoppedMessage(this->getMessagePayloadValue(
                                                SimpleStorageServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
            } catch (std::shared_ptr <NetworkError> &cause) {
                return false;
            }
            return false;

        } else if (auto msg = dynamic_cast<StorageServiceFreeSpaceRequestMessage*>(message.get())) {
            std::map<std::string, double> free_space;

            for (auto const &mp : this->file_systems) {
                free_space[mp.first] = mp.second->getFreeSpace();
            }

            S4U_Mailbox::dputMessage(
                    msg->answer_mailbox,
                    new StorageServiceFreeSpaceAnswerMessage(
                            free_space,
                            this->getMessagePayloadValue(
                                    SimpleStorageServiceMessagePayload::FREE_SPACE_ANSWER_MESSAGE_PAYLOAD)));
            return true;

        } else if (auto msg = dynamic_cast<StorageServiceFileDeleteRequestMessage*>(message.get())) {
            return processFileDeleteRequest(msg->file, msg->location, msg->answer_mailbox);

        } else if (auto msg = dynamic_cast<StorageServiceFileLookupRequestMessage*>(message.get())) {
            auto fs = this->file_systems[msg->location->getMountPoint()].get();
            bool file_found = fs->isFileInDirectory(msg->file, msg->location->getAbsolutePathAtMountPoint());

            S4U_Mailbox::dputMessage(
                    msg->answer_mailbox,
                    new StorageServiceFileLookupAnswerMessage(
                            msg->file, file_found,
                            this->getMessagePayloadValue(
                                    SimpleStorageServiceMessagePayload::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD)));
            return true;

        } else if (auto msg = dynamic_cast<StorageServiceFileWriteRequestMessage*>(message.get())) {
            return processFileWriteRequest(msg->file, msg->location, msg->answer_mailbox, msg->buffer_size);

        } else if (auto msg = dynamic_cast<StorageServiceFileReadRequestMessage*>(message.get())) {
            return processFileReadRequest(msg->file, msg->location, msg->answer_mailbox,
                                          msg->mailbox_to_receive_the_file_content, msg->buffer_size);

        } else if (auto msg = dynamic_cast<StorageServiceFileCopyRequestMessage*>(message.get())) {
            return processFileCopyRequest(msg->file, msg->src, msg->dst, msg->answer_mailbox);

        } else if (auto msg = dynamic_cast<FileTransferThreadNotificationMessage*>(message.get())) {
            return processFileTransferThreadNotification(
                    msg->file_transfer_thread,
                    msg->file,
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
                    "SimpleStorageService::processNextMessage(): Unexpected [" + message->getName() + "] message");
        }
    }

    /**
     * @brief Handle a file write request
     *
     * @param file: the file to write
     * @param location: the location to write the file to
     * @param answer_mailbox: the mailbox to which the reply should be sent
     * @param buffer_size: the buffer size to use
     * @return true if this process should keep running
     */
    bool SimpleStorageService::processFileWriteRequest(WorkflowFile *file, std::shared_ptr <FileLocation> location,
                                                       std::string answer_mailbox, unsigned long buffer_size) {
        // Figure out whether this succeeds or not
        std::shared_ptr <FailureCause> failure_cause = nullptr;

//        // Invalid mount point
//        if ((this->file_systems.find(location->getMountPoint()) == this->file_systems.end())) {
//
//            failure_cause = std::shared_ptr<FailureCause>(
//                    new InvalidDirectoryPath(
//                            this->getSharedPtr<SimpleStorageService>(),
//                            location->getMountPoint()
//                            + "/" +
//                            location->getAbsolutePathAtMountPoint()));
//        } else {

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
            auto fs = this->file_systems[location->getMountPoint()].get();

            if (not fs->doesDirectoryExist(location->getAbsolutePathAtMountPoint())) {
                fs->createDirectory(location->getAbsolutePathAtMountPoint());
            }

            // Update occupied space, in advance (will have to be decreased later in case of failure)
            fs->reserveSpace(file, location->getAbsolutePathAtMountPoint());

            // Generate a mailbox_name name on which to receive the file
            std::string file_reception_mailbox = S4U_Mailbox::generateUniqueMailboxName("file_reception");

            // Reply with a "go ahead, send me the file" message
            S4U_Mailbox::dputMessage(
                    answer_mailbox,
                    new StorageServiceFileWriteAnswerMessage(
                            file,
                            location,
                            true,
                            nullptr,
                            file_reception_mailbox,
                            this->getMessagePayloadValue(
                                    SimpleStorageServiceMessagePayload::FILE_WRITE_ANSWER_MESSAGE_PAYLOAD)));

            // Create a FileTransferThread
            auto ftt = std::shared_ptr<FileTransferThread>(
                    new FileTransferThread(this->hostname,
                                           this->getSharedPtr<StorageService>(),
                                           file,
                                           file_reception_mailbox,
                                           location,
                                           "",
                                           answer_mailbox,
                                           "",
                                           buffer_size));
            ftt->simulation = this->simulation;

            // Add it to the Pool of pending data communications
            this->pending_file_transfer_threads.push_back(ftt);

        } else {
            // Reply with a "failure" message
            S4U_Mailbox::dputMessage(
                    answer_mailbox,
                    new StorageServiceFileWriteAnswerMessage(
                            file,
                            location,
                            false,
                            failure_cause,
                            "",
                            this->getMessagePayloadValue(
                                    SimpleStorageServiceMessagePayload::FILE_WRITE_ANSWER_MESSAGE_PAYLOAD)));
        }

        return true;
    }

    /**
     * @brief Handle a file read request
     * @param file: the file
     * @param src_partition: the file partition to read the file from
     * @param answer_mailbox: the mailbox to which the answer should be sent
     * @param mailbox_to_receive_the_file_content: the mailbox to which the file will be sent
     * @param buffer_size: the buffer_size to use
     * @return
     */
    bool SimpleStorageService::processFileReadRequest(WorkflowFile *file,
                                                      std::shared_ptr <FileLocation> location,
                                                      std::string answer_mailbox,
                                                      std::string mailbox_to_receive_the_file_content,
                                                      unsigned long buffer_size) {

        // Figure out whether this succeeds or not
        std::shared_ptr <FailureCause> failure_cause = nullptr;

//        if ((this->file_systems.find(location->getMountPoint()) == this->file_systems.end()) or
        if (not this->file_systems[location->getMountPoint()]->doesDirectoryExist(
                location->getAbsolutePathAtMountPoint())) {
            failure_cause = std::shared_ptr<FailureCause>(
                    new InvalidDirectoryPath(
                            this->getSharedPtr<SimpleStorageService>(),
                            location->getMountPoint()
                            + "/" +
                            location->getAbsolutePathAtMountPoint()));
        } else {
            auto fs = this->file_systems[location->getMountPoint()].get();

            if (not fs->isFileInDirectory(file, location->getAbsolutePathAtMountPoint())) {
                WRENCH_INFO(
                        "Received a read request for a file I don't have (%s)", location->toString().c_str());
                failure_cause = std::shared_ptr<FailureCause>(new FileNotFound(file, location));
            }
        }

        bool success = (failure_cause == nullptr);;

        // Send back the corresponding ack, asynchronously and in a "fire and forget" fashion
        S4U_Mailbox::dputMessage(
                answer_mailbox,
                new StorageServiceFileReadAnswerMessage(
                        file,
                        location,
                        success,
                        failure_cause,
                        this->getMessagePayloadValue(
                                SimpleStorageServiceMessagePayload::FILE_READ_ANSWER_MESSAGE_PAYLOAD)));

        // If success, then follow up with sending the file (ASYNCHRONOUSLY!)
        if (success) {
            // Create a FileTransferThread
            auto ftt = std::shared_ptr<FileTransferThread>(
                    new FileTransferThread(this->hostname,
                                           this->getSharedPtr<StorageService>(),
                                           file,
                                           location,
                                           mailbox_to_receive_the_file_content,
                                           answer_mailbox,
                                           "",
                                           "",
                                           buffer_size));
            ftt->simulation = this->simulation;

            // Add it to the Pool of pending data communications
            this->pending_file_transfer_threads.push_front(ftt);
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
    bool SimpleStorageService::processFileCopyRequest(WorkflowFile *file,
                                                      std::shared_ptr <FileLocation> src_location,
                                                      std::shared_ptr <FileLocation> dst_location,
                                                      std::string answer_mailbox) {
//        // File System  and path at the destination exists?
//        if (this->file_systems.find(dst_location->getMountPoint()) == this->file_systems.end())  {
//
//            this->simulation->getOutput().addTimestamp<SimulationTimestampFileCopyFailure>(
//                    new SimulationTimestampFileCopyFailure(start_timestamp));
//
//            try {
//                S4U_Mailbox::putMessage(answer_mailbox,
//                                        new StorageServiceFileCopyAnswerMessage(
//                                                file,
//                                                src_location,
//                                                dst_location,
//                                                nullptr, false,
//                                                false,
//                                                std::shared_ptr<FailureCause>(
//                                                        new InvalidDirectoryPath(
//                                                                this->getSharedPtr<SimpleStorageService>(),
//                                                                dst_location->getMountPoint()
//                                                                + "/" +
//                                                                dst_location->getAbsolutePathAtMountPoint())),
//                                                this->getMessagePayloadValue(
//                                                        SimpleStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD)));
//
//            } catch (std::shared_ptr<NetworkError> &cause) {
//                return true;
//            }
//            return true;
//        }

        auto fs = this->file_systems[dst_location->getMountPoint()].get();

        // File is not already here
        if (not fs->isFileInDirectory(file, dst_location->getAbsolutePathAtMountPoint())) {
            if (not fs->hasEnoughFreeSpace(file->getSize())) {
                this->simulation->getOutput().addTimestampFileCopyFailure(file, src_location, dst_location);

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

                } catch (std::shared_ptr <NetworkError> &cause) {
                    return true;
                }
                return true;
            }
            fs->reserveSpace(file, dst_location->getAbsolutePathAtMountPoint());
        }

        WRENCH_INFO("Starting a thread to copy file %s from %s to %s",
                    file->getID().c_str(),
                    src_location->toString().c_str(),
                    dst_location->toString().c_str()
                    );

        // Create a file transfer thread
        auto ftt = std::shared_ptr<FileTransferThread>(
                new FileTransferThread(this->hostname,
                                       this->getSharedPtr<StorageService>(),
                                       file,
                                       src_location,
                                       dst_location,
                                       "",
                                       "",
                                       answer_mailbox,
                                       this->buffer_size));
        ftt->simulation = this->simulation;
        this->pending_file_transfer_threads.push_back(ftt);

        return true;
    }

    /**
     * @brief Start pending file transfer threads if any and if possible
     */
    void SimpleStorageService::startPendingFileTransferThread() {
        while ((not this->pending_file_transfer_threads.empty()) and
               (this->running_file_transfer_threads.size() < this->num_concurrent_connections)) {
            // Start a communications!
            auto ftt = this->pending_file_transfer_threads.at(0);
            this->pending_file_transfer_threads.pop_front();
            this->running_file_transfer_threads.insert(ftt);
            ftt->start(ftt, true, false); // Daemonize, non-auto-restart
        }
    }

    /**
     * @brief Process a notification received from a file transfer thread
     * @param ftt: the file transfer thread
     * @param file: the file
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
    bool SimpleStorageService::processFileTransferThreadNotification(std::shared_ptr <FileTransferThread> ftt,
                                                                     WorkflowFile *file,
                                                                     std::string src_mailbox,
                                                                     std::shared_ptr <FileLocation> src_location,
                                                                     std::string dst_mailbox,
                                                                     std::shared_ptr <FileLocation> dst_location,
                                                                     bool success,
                                                                     std::shared_ptr <FailureCause> failure_cause,
                                                                     std::string answer_mailbox_if_read,
                                                                     std::string answer_mailbox_if_write,
                                                                     std::string answer_mailbox_if_copy) {
        // Remove the ftt from the list of running ftt
        if (this->running_file_transfer_threads.find(ftt) == this->running_file_transfer_threads.end()) {
            WRENCH_INFO(
                    "Got a notification from a non-existing File Transfer Thread. Perhaps this is from a former life... ignoring");
        } else {
            this->running_file_transfer_threads.erase(ftt);
        }

        // Was the destination me?
        if (dst_location and (dst_location->getStorageService().get() == this)) {
            if (success) {
                WRENCH_INFO("File %s stored!", file->getID().c_str());
                this->file_systems[dst_location->getMountPoint()]->storeFileInDirectory(
                        file, dst_location->getAbsolutePathAtMountPoint());
                // Deal with time stamps, previously we could test whether a real timestamp was passed, now this.
                // May be no corresponding timestamp.
                try {
                    this->simulation->getOutput().addTimestampFileCopyCompletion(file, src_location, dst_location);
                } catch (invalid_argument &e) {
                }

            } else {
                // Process the failure, meaning, just un-decrease the free space
                this->file_systems[dst_location->getMountPoint()]->unreserveSpace(
                        file, dst_location->getAbsolutePathAtMountPoint());
            }
        }

        // Send back the relevant ack if this was a read
        if (not answer_mailbox_if_read.empty()) {
            WRENCH_DEBUG(
                    "Sending back an ack since this was a file read and some client is waiting for me to say something");
            S4U_Mailbox::dputMessage(answer_mailbox_if_read, new StorageServiceAckMessage());
        }

        // Send back the relevant ack if this was a write
        if (not answer_mailbox_if_write.empty()) {
            WRENCH_DEBUG(
                    "Sending back an ack since this was a file write and some client is waiting for me to say something");
            S4U_Mailbox::dputMessage(answer_mailbox_if_write, new StorageServiceAckMessage());
        }

        // Send back the relevant ack if this was a copy
        if (not answer_mailbox_if_copy.empty()) {
            WRENCH_DEBUG(
                    "Sending back an ack since this was a file copy and some client is waiting for me to say something");
            if ((src_location == nullptr) or (dst_location == nullptr)) {
                throw std::runtime_error("SimpleStorageService::processFileTransferThreadNotification(): "
                                         "src_location and dst_location must be non-null");
            }
            S4U_Mailbox::dputMessage(
                    answer_mailbox_if_copy,
                    new StorageServiceFileCopyAnswerMessage(
                            file,
                            src_location,
                            dst_location,
                            nullptr,
                            false,
                            success,
                            failure_cause,
                            this->getMessagePayloadValue(
                                    SimpleStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD)));
        }

        return true;
    }

    /**
     * @brief Process a file deletion request
     * @param file: the file to delete
     * @param dst_partition: the partition in which it is
     * @param answer_mailbox: the mailbox to which the notification should be sent
     * @return false if the daemon should terminate
     */
    bool SimpleStorageService::processFileDeleteRequest(WorkflowFile *file,
                                                        std::shared_ptr <FileLocation> location,
                                                        std::string answer_mailbox) {
        std::shared_ptr <FailureCause> failure_cause = nullptr;

        auto fs = this->file_systems[location->getMountPoint()].get();

        if ((not fs->doesDirectoryExist(location->getAbsolutePathAtMountPoint())) or
            (not fs->isFileInDirectory(file, location->getAbsolutePathAtMountPoint()))) {
            // If this is scratch, we don't care, perhaps it was taken care of elsewhere...
            if (not this->isScratch()) {
                failure_cause = std::shared_ptr<FailureCause>(
                        new FileNotFound(file, location));
            }
        } else {
            fs->removeFileFromDirectory(file, location->getAbsolutePathAtMountPoint());
        }

        S4U_Mailbox::dputMessage(
                answer_mailbox,
                new StorageServiceFileDeleteAnswerMessage(
                        file,
                        this->getSharedPtr<SimpleStorageService>(),
                        (failure_cause == nullptr),
                        failure_cause,
                        this->getMessagePayloadValue(
                                SimpleStorageServiceMessagePayload::FILE_DELETE_ANSWER_MESSAGE_PAYLOAD)));
        return true;
    }

    /**
     * @brief Helper method to validate propery values
     * throw std::invalid_argument
     */
    void SimpleStorageService::validateProperties() {
        this->getPropertyValueAsUnsignedLong(SimpleStorageServiceProperty::MAX_NUM_CONCURRENT_DATA_CONNECTIONS);
        this->getPropertyValueAsUnsignedLong(SimpleStorageServiceProperty::BUFFER_SIZE);
    }

};
