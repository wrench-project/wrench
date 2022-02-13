/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <wrench/services/storage/StorageServiceMessage.h>
#include <wrench/services/storage/storage_helpers/FileTransferThread.h>
#include "wrench/services/storage/storage_helpers/FileTransferThreadMessage.h"

#include <wrench-dev.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <wrench/services/storage/storage_helpers/FileTransferThread.h>
#include <wrench/services/memory/MemoryManager.h>

WRENCH_LOG_CATEGORY(wrench_core_file_transfer_thread, "Log category for File Transfer Thread");


namespace wrench {
    /**
     * @brief Constructor
     * @param hostname: host on which to run
     * @param parent: the parent storage service
     * @param file: the file corresponding to the connection
     * @param num_bytes_to_transfer: number of bytes to transfer
     * @param src_mailbox: the a source mailbox to receive data from
     * @param dst_location: a location to write data to
     * @param answer_mailbox_if_read: the mailbox to send an answer to in case this was a file read ("" if none). This
     *        will simply be reported to the parent service, who may use it as needed
     * @param answer_mailbox_if_write: the mailbox to send an answer to in case this was a file write ("" if none). This
     *        will simply be reported to the parent service, who may use it as needed
     * @param answer_mailbox_if_copy: the mailbox to send an answer to in case this was a file copy ("" if none). This
     *        will simply be reported to the parent service, who may use it as needed
     * @param buffer_size: the buffer size to use
     */
    FileTransferThread::FileTransferThread(std::string hostname,
                                           std::shared_ptr<StorageService> parent,
                                           std::shared_ptr<DataFile>file,
                                           double num_bytes_to_transfer,
                                           simgrid::s4u::Mailbox *src_mailbox,
                                           std::shared_ptr<FileLocation> dst_location,
                                           simgrid::s4u::Mailbox *answer_mailbox_if_read,
                                           simgrid::s4u::Mailbox *answer_mailbox_if_write,
                                           simgrid::s4u::Mailbox *answer_mailbox_if_copy,
                                           unsigned long buffer_size) :
            Service(hostname, "file_transfer_thread"),
            parent(parent),
            file(file),
            num_bytes_to_transfer(num_bytes_to_transfer),
            answer_mailbox_if_read(answer_mailbox_if_read),
            answer_mailbox_if_write(answer_mailbox_if_write),
            answer_mailbox_if_copy(answer_mailbox_if_copy),
            buffer_size(buffer_size) {
        this->src_mailbox = src_mailbox;
        this->src_location = nullptr;
        this->dst_mailbox = nullptr;
        this->dst_location = dst_location;
    }

    /**
     * @brief Constructor
     * @param hostname: host on which to run
     * @param parent: the parent storage service
     * @param file: the file corresponding to the connection
     * @param src_location: a location to read data from
     * @param dst_mailbox: a mailbox to send data to
     * @param answer_mailbox_if_read: the mailbox to send an answer to in case this was a file read ("" if none). This
     *        will simply be reported to the parent service, who may use it as needed
     * @param answer_mailbox_if_write: the mailbox to send an answer to in case this was a file write ("" if none). This
     *        will simply be reported to the parent service, who may use it as needed
     * @param answer_mailbox_if_copy: the mailbox to send an answer to in case this was a file copy ("" if none). This
     *        will simply be reported to the parent service, who may use it as needed
     * @param buffer_size: the buffer size to use
     */
    FileTransferThread::FileTransferThread(std::string hostname,
                                           std::shared_ptr<StorageService> parent,
                                           std::shared_ptr<DataFile>file,
                                           double num_bytes_to_transfer,
                                           std::shared_ptr<FileLocation> src_location,
                                           simgrid::s4u::Mailbox *dst_mailbox,
                                           simgrid::s4u::Mailbox *answer_mailbox_if_read,
                                           simgrid::s4u::Mailbox *answer_mailbox_if_write,
                                           simgrid::s4u::Mailbox *answer_mailbox_if_copy,
                                           unsigned long buffer_size) :
            Service(hostname, "file_transfer_thread"),
            parent(parent),
            file(file),
            num_bytes_to_transfer(num_bytes_to_transfer),
            answer_mailbox_if_read(answer_mailbox_if_read),
            answer_mailbox_if_write(answer_mailbox_if_write),
            answer_mailbox_if_copy(answer_mailbox_if_copy),
            buffer_size(buffer_size) {
        this->src_mailbox = nullptr;
        this->src_location = src_location;
        this->dst_mailbox = dst_mailbox;
        this->dst_location = nullptr;
    }

    /**
     * @brief Constructor
     * @param hostname: host on which to run
     * @param parent: the parent storage service
     * @param file: the file corresponding to the connection
     * @param src_location: a location to read data from
     * @param dst_location: a location to send data to
     * @param answer_mailbox_if_read: the mailbox to send an answer to in case this was a file read (nullptr if none). This
     *        will simply be reported to the parent service, who may use it as needed
     * @param answer_mailbox_if_write: the mailbox to send an answer to in case this was a file write (nullptr if none). This
     *        will simply be reported to the parent service, who may use it as needed
     * @param answer_mailbox_if_copy: the mailbox to send an answer to in case this was a file copy (nullptr if none). This
     *        will simply be reported to the parent service, who may use it as needed
     * @param buffer_size: the buffer size to use
     */
    FileTransferThread::FileTransferThread(std::string hostname,
                                           std::shared_ptr<StorageService> parent,
                                           std::shared_ptr<DataFile>file,
                                           double num_bytes_to_transfer,
                                           std::shared_ptr<FileLocation> src_location,
                                           std::shared_ptr<FileLocation> dst_location,
                                           simgrid::s4u::Mailbox *answer_mailbox_if_read,
                                           simgrid::s4u::Mailbox *answer_mailbox_if_write,
                                           simgrid::s4u::Mailbox *answer_mailbox_if_copy,
                                           unsigned long buffer_size) :
            Service(hostname, "file_transfer_thread"),
            parent(parent),
            file(file),
            num_bytes_to_transfer(num_bytes_to_transfer),
            answer_mailbox_if_read(answer_mailbox_if_read),
            answer_mailbox_if_write(answer_mailbox_if_write),
            answer_mailbox_if_copy(answer_mailbox_if_copy),
            buffer_size(buffer_size) {
        this->src_mailbox = nullptr;
        this->src_location = src_location;
        this->dst_mailbox = nullptr;
        this->dst_location = dst_location;
    }

    /**
     * @brief Cleanup method
     *
     * @param has_returned_from_main: whether main() returned
     * @param return_value: the return value (if main() returned)
     */
    void FileTransferThread::cleanup(bool has_returned_from_main, int return_value) {
        // Do nothing. It's fine to just die
    }

    /**
     * @brief Main method
     * @return 0 on success, non-zero otherwise
     */
    int FileTransferThread::main() {
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_CYAN);

        FileTransferThreadNotificationMessage *msg_to_send_back = nullptr;
        std::shared_ptr<NetworkError> failure_cause = nullptr;

        WRENCH_INFO(
                "New FileTransferThread (file=%s, bytes_to_transfer=%.2lf, src_mailbox=%s; src_location=%s; dst_mailbox=%s; dst_location=%s; "
                "answer_mailbox_if_read=%s; answer_mailbox_if_write=%s; answer_mailbox_if_copy=%s; buffer size=%lu",
                file->getID().c_str(),
                this->num_bytes_to_transfer,
                ((src_mailbox == nullptr) ? "none" : src_mailbox->get_cname()),
                (src_location == nullptr ? "none" : src_location->toString().c_str()),
                ((dst_mailbox == nullptr) ? "none" : dst_mailbox->get_cname()),
                (dst_location == nullptr ? "none" : dst_location->toString().c_str()),
                ((answer_mailbox_if_read == nullptr) ? "none" : answer_mailbox_if_read->get_cname()),
                ((answer_mailbox_if_write == nullptr) ? "none" : answer_mailbox_if_write->get_cname()),
                ((answer_mailbox_if_copy == nullptr) ? "none" : answer_mailbox_if_copy->get_cname()),
                this->buffer_size
        );

        // Create a message to send back (some field of which may be overwritten below)
        msg_to_send_back = new FileTransferThreadNotificationMessage(
                this->getSharedPtr<FileTransferThread>(),
                this->file,
                this->src_mailbox,
                this->src_location,
                this->dst_mailbox,
                this->dst_location,
                this->answer_mailbox_if_read,
                this->answer_mailbox_if_write,
                this->answer_mailbox_if_copy,
                true, nullptr);

        if ((this->src_location) and (this->src_location->getStorageService() == this->parent) and
            (dst_mailbox)) {
            /** Sending a local file to the network **/
            try {
                sendLocalFileToNetwork(this->file, this->src_location, num_bytes_to_transfer, this->dst_mailbox);
            } catch (std::shared_ptr<NetworkError> &failure_cause) {

                WRENCH_INFO(
                        "FileTransferThread::main(): Network error (%s)", failure_cause->toString().c_str());
                msg_to_send_back->success = false;
                msg_to_send_back->failure_cause = failure_cause;
            }

        } else if ((this->src_mailbox) and (this->dst_location) and
                   (this->dst_location->getStorageService() == this->parent)) {
            /** Receiving a file from the network **/
            try {
                receiveFileFromNetwork(this->file, this->src_mailbox, this->dst_location);
            } catch (std::shared_ptr<NetworkError> &failure_cause) {

                WRENCH_INFO(
                        "FileTransferThread::main() Network error (%s)", failure_cause->toString().c_str());
                msg_to_send_back->success = false;
                msg_to_send_back->failure_cause = failure_cause;
            }

        } else if ((this->src_location) and (this->src_location->getStorageService() == this->parent) and
                   (this->dst_location) and (this->dst_location->getStorageService() == this->parent)) {
            /** Copying a local file */
            copyFileLocally(this->file, this->src_location, this->dst_location);

        } else if (((this->src_location) and (this->dst_location) and
                    (this->dst_location->getStorageService() == this->parent))) {
            /** Downloading a file from another storage service */
            try {
                downloadFileFromStorageService(this->file, this->src_location, this->dst_location);
            } catch (std::shared_ptr<NetworkError> &failure_cause) {
                msg_to_send_back->success = false;
                msg_to_send_back->failure_cause = failure_cause;
            } catch (std::shared_ptr<FailureCause> &failure_cause) {
                msg_to_send_back->success = false;
                msg_to_send_back->failure_cause = failure_cause;
            }
        } else {
            throw std::runtime_error("FileTransferThread::main(): Invalid src/dst combination");
        }

        // Call retire on all mailboxes passed, which is pretty brute force be should work
        // TODO: Figure out which ones to retire in the methods called above
//        if (answer_mailbox_if_read) S4U_Mailbox::retireTemporaryMailbox(answer_mailbox_if_read);
//        if (answer_mailbox_if_write) S4U_Mailbox::retireTemporaryMailbox(answer_mailbox_if_write);
//        if (answer_mailbox_if_copy) S4U_Mailbox::retireTemporaryMailbox(answer_mailbox_if_copy);
        if (this->dst_mailbox) {
//            std::cerr << "FTT: RETIRING MAILBOX " << this->dst_mailbox->get_name() << "\n";
            S4U_Mailbox::retireTemporaryMailbox(this->dst_mailbox);
        }
        if (this->src_mailbox) {
//            std::cerr << "FTT: RETIRING MAILBOX " << this->src_mailbox->get_name() << "\n";
            S4U_Mailbox::retireTemporaryMailbox(this->src_mailbox);
        }

        try {
            // Send report back to the service
            // (a dput() right before death is always dicey, so this is a put())
            S4U_Mailbox::putMessage(this->parent->mailbox, msg_to_send_back);
        } catch (std::shared_ptr<NetworkError> &e) {
            // oh well...
        }

        return 0;
    }

    /**
    * @brief Method to received a file from the network onto the local disk
    * @param file: the file
    * @param mailbox: the source mailbox
    * @param location: the destination location
    *
    * @throw shared_ptr<FailureCause>
    */
    void FileTransferThread::receiveFileFromNetwork(std::shared_ptr<DataFile>file,
                                                    simgrid::s4u::Mailbox *mailbox,
                                                    std::shared_ptr<FileLocation> location) {

        /** Ideal Fluid model buffer size */
        if (this->buffer_size == 0) {
            throw std::runtime_error(
                    "FileTransferThread::receiveFileFromNetwork(): Zero buffer size not implemented yet");

        } else {
            /** Non-zero buffer size */
            bool done = false;

            // Receive the first chunk
            auto msg = S4U_Mailbox::getMessage(mailbox);
            if (auto file_content_chunk_msg = dynamic_cast<StorageServiceFileContentChunkMessage *>(msg.get())) {
                done = file_content_chunk_msg->last_chunk;
            } else {
                throw std::runtime_error("FileTransferThread::receiveFileFromNetwork() : Received an unexpected [" +
                                         msg->getName() + "] message!");
            }

            try {
                if (Simulation::isPageCachingEnabled()) {
                    simulation->getMemoryManagerByHost(location->getStorageService()->hostname)->log();
//                    simulation->getMemoryManagerByHost(location->getStorageService()->hostname)->fincore();
                }

                // Receive chunks and write them to disk
                while (not done) {
                    // Issue the receive
                    auto req = S4U_Mailbox::igetMessage(mailbox);

                    // In NFS, write to cache only if the current host not the server host where the file is stored
                    // If the current host is file server, write to disk directly
                    if (Simulation::isPageCachingEnabled()) {

                        bool write_locally = location->getServerStorageService() == nullptr;

                        if (write_locally) {
                            simulation->writebackWithMemoryCache(file, msg->payload, location, true);
                        } else {
                            simulation->writeThroughWithMemoryCache(file, msg->payload, location);
                        }
                    } else {
                        // Write to disk
                        simulation->writeToDisk(msg->payload, location->getStorageService()->hostname,
                                                location->getMountPoint());
                    }

                    // Wait for the comm to finish
                    msg = req->wait();
                    if (auto file_content_chunk_msg =
                            dynamic_cast<StorageServiceFileContentChunkMessage *>(msg.get())) {
                        done = file_content_chunk_msg->last_chunk;
                    } else {
                        throw std::runtime_error(
                                "FileTransferThread::receiveFileFromNetwork() : Received an unexpected [" +
                                msg->getName() + "] message!");
                    }
                }

                // I/O for the last chunk
                if (Simulation::isPageCachingEnabled()) {
                    bool write_locally = location->getServerStorageService() == nullptr;

                    if (write_locally) {
                        simulation->writebackWithMemoryCache(file, msg->payload, location, true);
                    } else {
                        simulation->writeThroughWithMemoryCache(file, msg->payload, location);
                    }
                } else {
//                     Write to disk
                    simulation->writeToDisk(msg->payload, location->getStorageService()->hostname,
                                            location->getMountPoint());
                }

                if (Simulation::isPageCachingEnabled()) {
                    simulation->getMemoryManagerByHost(location->getStorageService()->hostname)->log();
//                    simulation->getMemoryManagerByHost(location->getStorageService()->hostname)->fincore();
                }

            } catch (std::shared_ptr<NetworkError> &e) {
                throw;
            }
        }
    }

    /**
     * @brief Method to send a file from the local disk to the network
     * @param file: the file
     * @param location: the source location
     * @param num_bytes_to_transfer: number of bytes to transfer
     * @param mailbox: the destination mailbox
     *
     * @throw shared_ptr<FailureCause>
     */
    void FileTransferThread::sendLocalFileToNetwork(std::shared_ptr<DataFile>file,
                                                    std::shared_ptr<FileLocation> location,
                                                    double num_bytes_to_transfer,
                                                    simgrid::s4u::Mailbox *mailbox) {
        /** Ideal Fluid model buffer size */
        if (this->buffer_size == 0) {
            throw std::runtime_error(
                    "FileTransferThread::sendLocalFileToNetwork(): Zero buffer size not implemented yet");

        } else {
            try {
                /** Non-zero buffer size */
                std::shared_ptr<S4U_PendingCommunication> req = nullptr;
                // Sending a zero-byte file is really sending a 1-byte file
                double remaining = std::max<double>(1, num_bytes_to_transfer);

                if (Simulation::isPageCachingEnabled()) {
                    simulation->getMemoryManagerByHost(location->getStorageService()->hostname)->log();
                }

                while (remaining > 0) {
                    double chunk_size = std::min<double>(this->buffer_size, remaining);

                    if (Simulation::isPageCachingEnabled()) {
                        simulation->readWithMemoryCache(file, chunk_size, location);
                    } else { WRENCH_INFO("Reading %s bytes from disk", std::to_string(chunk_size).c_str());
                        simulation->readFromDisk(chunk_size, location->getStorageService()->hostname,
                                                 location->getMountPoint());
                    }

                    remaining -= (double) (this->buffer_size);
                    if (req) {
                        req->wait();
//                        WRENCH_INFO("Bytes sent over the network were received");
                    }
//                    WRENCH_INFO("Asynchronously sending %s bytes over the network", std::to_string(chunk_size).c_str());
                    req = S4U_Mailbox::iputMessage(mailbox,
                                                   new StorageServiceFileContentChunkMessage(
                                                           this->file,
                                                           (unsigned long) chunk_size, (remaining <= 0)));
                }
                if (Simulation::isPageCachingEnabled()) {
                    simulation->getMemoryManagerByHost(location->getStorageService()->hostname)->log();
//                    simulation->getMemoryManagerByHost(location->getStorageService()->hostname)->fincore();
                }
                req->wait();
                WRENCH_INFO("Bytes sent over the network were received");
            } catch (std::shared_ptr<NetworkError> &e) {
                throw;
            }
        }
    }

    /**
     * @brief Method to copy a file localy
     * @param file: the file to copy
     * @param src_location: the source location
     * @param dst_location: the destination location
     */
    void FileTransferThread::copyFileLocally(std::shared_ptr<DataFile>file,
                                             std::shared_ptr<FileLocation> src_location,
                                             std::shared_ptr<FileLocation> dst_location) {
        double remaining = file->getSize();
        double to_send = std::min<double>(this->buffer_size, remaining);

        if ((src_location->getStorageService() == dst_location->getStorageService()) and
            (src_location->getFullAbsolutePath() == dst_location->getFullAbsolutePath())) { WRENCH_INFO(
                    "FileTransferThread::copyFileLocally(): Copying file %s onto itself at location %s... ignoring",
                    file->getID().c_str(), src_location->toString().c_str());
            return;
        }

        /** Ideal Fluid model buffer size */
        if (this->buffer_size == 0) {
            throw std::runtime_error(
                    "FileTransferThread::copyFileLocally(): Zero buffer size not implemented yet");

        } else {
            // Read the first chunk
            simulation->readFromDisk(to_send, src_location->getStorageService()->hostname,
                                     src_location->getMountPoint());
            // start the pipeline
            while (remaining > this->buffer_size) {

                simulation->readFromDiskAndWriteToDiskConcurrently(
                        this->buffer_size, this->buffer_size, src_location->getStorageService()->hostname,
                        src_location->getMountPoint(), dst_location->getMountPoint());

//
//                simulation->writeToDisk(this->buffer_size, dst_location->getStorageService()->hostname,
//                                            dst_location->getMountPoint());
//                simulation->readFromDisk(this->buffer_size, src_location->getStorageService()->hostname,
//                                             src_location->getMountPoint());

                remaining -= this->buffer_size;
            }
            // Write the last chunk
            simulation->writeToDisk(remaining, dst_location->getStorageService()->hostname,
                                    dst_location->getMountPoint());
        }
    }

    /**
     * @brief Download a file to a local partition/disk
     * @param file: the file to download
     * @param src_location: the source location
     * @param dst_location: the destination location
     * @param downloader_buffer_size: buffer size of the downloader (0 means use "ideal fluid model")
     */
    void FileTransferThread::downloadFileFromStorageService(std::shared_ptr<DataFile>file,
                                                            std::shared_ptr<FileLocation> src_location,
                                                            std::shared_ptr<FileLocation> dst_location) {
        if (file == nullptr) {
            throw std::invalid_argument("StorageService::downloadFile(): Invalid arguments");
        }

        WRENCH_INFO("Downloading file  %s from location %s",
                    file->getID().c_str(), src_location->toString().c_str());

        // Check that the buffer size is compatible
        if (((this->buffer_size == 0) && (src_location->getStorageService()->buffer_size != 0)) or
            ((this->buffer_size != 0) && (src_location->getStorageService()->buffer_size == 0))) {
            throw std::invalid_argument("FileTransferThread::downloadFileFromStorageService(): "
                                        "Incompatible buffer size specs (both must be zero, or both must be non-zero");
        }

        // Send a message to the source
        auto request_answer_mailbox = S4U_Daemon::getRunningActorRecvMailbox();
//        auto mailbox_that_should_receive_file_content = S4U_Mailbox::generateUniqueMailbox("works_by_itself");
        auto mailbox_that_should_receive_file_content = S4U_Mailbox::getTemporaryMailbox();

        try {
            S4U_Mailbox::putMessage(
                    src_location->getStorageService()->mailbox,
                    new StorageServiceFileReadRequestMessage(
                            request_answer_mailbox,
                            mailbox_that_should_receive_file_content,
                            file,
                            src_location,
                            file->getSize(),
                            std::min<unsigned long>(this->buffer_size,
                                                    this->buffer_size),
                            src_location->getStorageService()->getMessagePayloadValue(
                                    StorageServiceMessagePayload::FILE_READ_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            S4U_Mailbox::retireTemporaryMailbox(mailbox_that_should_receive_file_content);
            throw;
        }

        // Wait for a reply to the request
        std::unique_ptr<SimulationMessage> message = nullptr;

        try {
            message = S4U_Mailbox::getMessage(request_answer_mailbox, this->network_timeout);
        } catch (std::shared_ptr<NetworkError> &cause) {
            S4U_Mailbox::retireTemporaryMailbox(mailbox_that_should_receive_file_content);
            throw;
        }

        if (auto msg = dynamic_cast<StorageServiceFileReadAnswerMessage *>(message.get())) {
            // If it's not a success, throw an exception
            if (not msg->success) {
                S4U_Mailbox::retireTemporaryMailbox(mailbox_that_should_receive_file_content);
                throw msg->failure_cause;
            }
        } else {
            S4U_Mailbox::retireTemporaryMailbox(mailbox_that_should_receive_file_content);
            throw std::runtime_error("FileTransferThread::downloadFileFromStorageService(): Received an unexpected [" +
                                     message->getName() + "] message!");
        }

        WRENCH_INFO("Download request accepted (will receive file content on mailbox_name %s)",
                    mailbox_that_should_receive_file_content->get_cname());

        if (this->buffer_size == 0) {
            S4U_Mailbox::retireTemporaryMailbox(mailbox_that_should_receive_file_content);
            throw std::runtime_error(
                    "FileTransferThread::downloadFileFromStorageService(): Zero buffer size not implemented yet");

        } else {
            try {
                bool done = false;
                // Receive the first chunk
                auto msg = S4U_Mailbox::getMessage(mailbox_that_should_receive_file_content);
                if (auto file_content_chunk_msg =
                        dynamic_cast<StorageServiceFileContentChunkMessage *>(msg.get())) {
                    done = file_content_chunk_msg->last_chunk;
                } else {
                    S4U_Mailbox::retireTemporaryMailbox(mailbox_that_should_receive_file_content);
                    throw std::runtime_error(
                            "FileTransferThread::downloadFileFromStorageService(): Received an unexpected [" +
                            msg->getName() + "] message!");
                }

                // Receive chunks and write them to disk
                while (not done) {
                    // Issue the receive
                    auto req = S4U_Mailbox::igetMessage(mailbox_that_should_receive_file_content);
//                    WRENCH_INFO("Downloaded of %f of file  %s from location %s",
//                                msg->payload, file->getID().c_str(), src_location->toString().c_str());
                    // Do the I/O
                    if (Simulation::isPageCachingEnabled()) {
                        simulation->writebackWithMemoryCache(file, msg->payload, dst_location, false);
                    } else {
                        // Write to disk
                        simulation->writeToDisk(msg->payload,
                                                dst_location->getStorageService()->getHostname(),
                                                dst_location->getMountPoint());
                    }
                    // Wait for the comm to finish
//                    WRENCH_INFO("Wrote of %f of file  %s", msg->payload, file->getID().c_str());
                    msg = req->wait();
                    if (auto file_content_chunk_msg =
                            dynamic_cast<StorageServiceFileContentChunkMessage *>(msg.get())) {
                        done = file_content_chunk_msg->last_chunk;
                    } else {
                        S4U_Mailbox::retireTemporaryMailbox(mailbox_that_should_receive_file_content);
                        throw std::runtime_error(
                                "FileTransferThread::downloadFileFromStorageService(): Received an unexpected [" +
                                msg->getName() + "] message!");
                    }
                }
                S4U_Mailbox::retireTemporaryMailbox(mailbox_that_should_receive_file_content);

                // Do the I/O for the last chunk
                if (Simulation::isPageCachingEnabled()) {
                    simulation->writebackWithMemoryCache(file, msg->payload, dst_location, false);
                } else {
                    // Write to disk
                    simulation->writeToDisk(msg->payload,
                                            dst_location->getStorageService()->getHostname(),
                                            dst_location->getMountPoint());
                }
            } catch (std::shared_ptr<NetworkError> &e) {
                S4U_Mailbox::retireTemporaryMailbox(mailbox_that_should_receive_file_content);
                throw;
            }
        }
    }

}
