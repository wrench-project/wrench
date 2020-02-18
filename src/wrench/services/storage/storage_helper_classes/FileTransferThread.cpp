/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <services/storage/StorageServiceMessage.h>
#include "wrench/services/storage/storage_helpers/FileTransferThread.h"
#include "FileTransferThreadMessage.h"

#include <wrench-dev.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <wrench/services/storage/storage_helpers/FileTransferThread.h>

WRENCH_LOG_NEW_DEFAULT_CATEGORY(file_transfer_thread, "Log category for File Transfer Thread");


namespace wrench {


    /**
     * @brief Constructor
     * @param hostname: host on which to run
     * @param parent: the parent storage service
     * @param file: the file corresponding to the connection
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
                                           WorkflowFile *file,
                                           std::string src_mailbox,
                                           std::shared_ptr<FileLocation> dst_location,
                                           std::string answer_mailbox_if_read,
                                           std::string answer_mailbox_if_write,
                                           std::string answer_mailbox_if_copy,
                                           unsigned long buffer_size) :
            Service(hostname, "file_transfer_thread", "file_transfer_thread"),
            parent(parent),
            file(file),
            answer_mailbox_if_read(answer_mailbox_if_read),
            answer_mailbox_if_write(answer_mailbox_if_write),
            answer_mailbox_if_copy(answer_mailbox_if_copy),
            buffer_size(buffer_size)
    {
        this->src_mailbox = src_mailbox;
        this->src_location = nullptr;
        this->dst_mailbox = "";
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
                                           WorkflowFile *file,
                                           std::shared_ptr<FileLocation> src_location,
                                           std::string dst_mailbox,
                                           std::string answer_mailbox_if_read,
                                           std::string answer_mailbox_if_write,
                                           std::string answer_mailbox_if_copy,
                                           unsigned long buffer_size) :
            Service(hostname, "file_transfer_thread", "file_transfer_thread"),
            parent(parent),
            file(file),
            answer_mailbox_if_read(answer_mailbox_if_read),
            answer_mailbox_if_write(answer_mailbox_if_write),
            answer_mailbox_if_copy(answer_mailbox_if_copy),
            buffer_size(buffer_size)
    {
        this->src_mailbox = "";
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
                                           WorkflowFile *file,
                                           std::shared_ptr<FileLocation> src_location,
                                           std::shared_ptr<FileLocation> dst_location,
                                           std::string answer_mailbox_if_read,
                                           std::string answer_mailbox_if_write,
                                           std::string answer_mailbox_if_copy,
                                           unsigned long buffer_size) :
            Service(hostname, "file_transfer_thread", "file_transfer_thread"),
            parent(parent),
            file(file),
            answer_mailbox_if_read(answer_mailbox_if_read),
            answer_mailbox_if_write(answer_mailbox_if_write),
            answer_mailbox_if_copy(answer_mailbox_if_copy),
            buffer_size(buffer_size)
    {
        this->src_mailbox = "";
        this->src_location = src_location;
        this->dst_mailbox = "";
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

        FileTransferThreadNotificationMessage *msg_to_send_back = nullptr;
        std::shared_ptr<NetworkError> failure_cause = nullptr;

        WRENCH_INFO("New FileTransferThread (file=%s, src_mailbox=%s; src_location=%s; dst_mailbox=%s; dst_location=%s; "
                    "answer_mailbox_if_copy=%s; answer_mailbox_if_copy=%s; answer_mailbox_if_copy=%s",
                    file->getID().c_str(),
                    (src_mailbox.empty() ? "none" : src_mailbox.c_str()),
                    (src_location == nullptr ? "none" : src_location->toString().c_str()),
                    (dst_mailbox.empty() ? "none" : dst_mailbox.c_str()),
                    (dst_location == nullptr ? "none" : dst_location->toString().c_str()),
                    (answer_mailbox_if_read.empty() ? "none" : answer_mailbox_if_read.c_str()),
                    (answer_mailbox_if_write.empty() ? "none" : answer_mailbox_if_write.c_str()),
                    (answer_mailbox_if_copy.empty() ? "none" : answer_mailbox_if_copy.c_str())
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
            (not dst_mailbox.empty())) {
            /** Sending a local file to the network **/
            try {
                sendLocalFileToNetwork(this->file, this->src_location, this->dst_mailbox);
            } catch (std::shared_ptr<NetworkError> &failure_cause) {
                WRENCH_INFO("FileTransferThread::main(): Network error (%s)", failure_cause->toString().c_str());
                msg_to_send_back->success = false;
                msg_to_send_back->failure_cause = failure_cause;
            }

        } else if ((not this->src_mailbox.empty()) and (this->dst_location) and
                   (this->dst_location->getStorageService() == this->parent)) {
            /** Receiving a file from the network **/
            try {
                receiveFileFromNetwork(this->file, this->src_mailbox, this->dst_location);
            } catch (std::shared_ptr<NetworkError> &failure_cause) {
                WRENCH_INFO("FileTransferThread::main() Network error (%s)", failure_cause->toString().c_str());
                msg_to_send_back->success = false;
                msg_to_send_back->failure_cause = failure_cause;
            }

        } else if ((this->src_location) and (this->src_location->getStorageService() == this->parent) and
                   (this->dst_location) and (this->dst_location->getStorageService() == this->parent)) {
            /** Copying a file local file */
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


        try {
            // Send report back to the service
            // (a dput() right before death is always dicey, so this is a put())
            S4U_Mailbox::putMessage(this->parent->mailbox_name, msg_to_send_back);
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
    void FileTransferThread::receiveFileFromNetwork(WorkflowFile *file, std::string mailbox, std::shared_ptr<FileLocation> location) {

        /** Ideal Fluid model buffer size */
        if (this->buffer_size == 0) {

            throw std::runtime_error(
                    "FileTransferThread::receiveFileFromNetwork(): Zero buffer size not implemented yet");

        } else {
            /** Non-zero buffer size */

            bool done = false;

            // Receive the first chunk
            auto msg = S4U_Mailbox::getMessage(mailbox);
            if (auto file_content_chunk_msg =
                    std::dynamic_pointer_cast<StorageServiceFileContentChunkMessage>(msg)) {
                done = file_content_chunk_msg->last_chunk;
            } else {
                throw std::runtime_error("FileTransferThread::receiveFileFromNetwork() : Received an unexpected [" +
                                         msg->getName() + "] message!");
            }

            try {

                // Receive chunks and write them to disk
                while (not done) {
                    // Issue the receive
                    auto req = S4U_Mailbox::igetMessage(mailbox);
                    // Write to disk
                    S4U_Simulation::writeToDisk(msg->payload, location->getStorageService()->hostname,
                                                location->getMountPoint());
                    // Wait for the comm to finish
                    msg = req->wait();
                    if (auto file_content_chunk_msg =
                            std::dynamic_pointer_cast<StorageServiceFileContentChunkMessage>(msg)) {
                        done = file_content_chunk_msg->last_chunk;
                    } else {
                        throw std::runtime_error(
                                "FileTransferThread::receiveFileFromNetwork() : Received an unexpected [" +
                                msg->getName() + "] message!");
                    }
                }
                // I/O for the last chunk
                S4U_Simulation::writeToDisk(msg->payload, location->getStorageService()->hostname,
                                            location->getMountPoint());
            } catch (std::shared_ptr<NetworkError> &e) {
                throw;
            }
        }

    }


    /**
     * @brief Method to send a file from the local disk to the network
     * @param file: the file
     * @param location: the source location
     * @param mailbox: the destination mailbox
     *
     * @throw shared_ptr<FailureCause>
     */
    void FileTransferThread::sendLocalFileToNetwork(WorkflowFile *file, std::shared_ptr<FileLocation> location, std::string mailbox) {

        /** Ideal Fluid model buffer size */
        if (this->buffer_size == 0) {

            throw std::runtime_error(
                    "FileTransferThread::sendLocalFileToNetwork(): Zero buffer size not implemented yet");

        } else {

            try {
                /** Non-zero buffer size */
                std::shared_ptr<S4U_PendingCommunication> req = nullptr;
                // Sending a zero-byte file is really sending a 1-byte file
                double remaining = std::max<double>(1, file->getSize());

                while (remaining > 0) {
                    double chunk_size = std::min<double>(this->buffer_size, remaining);
                    S4U_Simulation::readFromDisk(chunk_size, location->getStorageService()->hostname,
                                                 location->getMountPoint());
                    remaining -= this->buffer_size;
                    if (req) {
                        req->wait();
                    }
                    req = S4U_Mailbox::iputMessage(mailbox,
                                                   new StorageServiceFileContentChunkMessage(
                                                           this->file,
                                                           chunk_size, (remaining <= 0)));
                }
                req->wait();
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
    void FileTransferThread::copyFileLocally(WorkflowFile *file,
                                             std::shared_ptr<FileLocation> src_location,
                                             std::shared_ptr<FileLocation> dst_location) {
        double remaining = file->getSize();
        double to_send = std::min<double>(this->buffer_size, remaining);

        if ((src_location->getStorageService() == dst_location->getStorageService()) and
                (src_location->getFullAbsolutePath() == dst_location->getFullAbsolutePath())) {
            WRENCH_INFO("FileTransferThread::copyFileLocally(): Copying file %s onto itself at location %s... ignoring",
                    file->getID().c_str(), src_location->toString().c_str());
            return;
        }

        /** Ideal Fluid model buffer size */
        if (this->buffer_size == 0) {

            throw std::runtime_error(
                    "FileTransferThread::copyFileLocally(): Zero buffer size not implemented yet");

        } else {
            // Read the first chunk
            S4U_Simulation::readFromDisk(to_send, src_location->getStorageService()->hostname,
                                         src_location->getMountPoint());
            // start the pipeline
            while (remaining > this->buffer_size) {

                S4U_Simulation::readFromDiskAndWriteToDiskConcurrently(
                        this->buffer_size, this->buffer_size, src_location->getStorageService()->hostname,
                        src_location->getMountPoint(), dst_location->getMountPoint());

//
//                S4U_Simulation::writeToDisk(this->buffer_size, dst_location->getStorageService()->hostname,
//                                            dst_location->getMountPoint());
//                S4U_Simulation::readFromDisk(this->buffer_size, src_location->getStorageService()->hostname,
//                                             src_location->getMountPoint());

                remaining -= this->buffer_size;
            }
            // Write the last chunk
            S4U_Simulation::writeToDisk(remaining, dst_location->getStorageService()->hostname,
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
    void FileTransferThread::downloadFileFromStorageService(WorkflowFile *file,
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
        std::string request_answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("read_file_request");
        std::string mailbox_that_should_receive_file_content = S4U_Mailbox::generateUniqueMailboxName("read_file_chunks");

        try {
            S4U_Mailbox::putMessage(src_location->getStorageService()->mailbox_name,
                                    new StorageServiceFileReadRequestMessage(request_answer_mailbox,
                                                                             mailbox_that_should_receive_file_content,
                                                                             file,
                                                                             src_location,
                                                                             std::min<unsigned long>(this->buffer_size, this->buffer_size),
                                                                             src_location->getStorageService()->getMessagePayloadValue(
                                                                                     StorageServiceMessagePayload::FILE_READ_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw;
        }

        // Wait for a reply to the request
        std::shared_ptr<SimulationMessage> message = nullptr;

        try {
            message = S4U_Mailbox::getMessage(request_answer_mailbox, this->network_timeout);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw;
        }


        if (auto msg = std::dynamic_pointer_cast<StorageServiceFileReadAnswerMessage>(message)) {
            // If it's not a success, throw an exception
            if (not msg->success) {
                throw msg->failure_cause;
            }
        } else {
            throw std::runtime_error("FileTransferThread::downloadFileFromStorageService(): Received an unexpected [" +
                                     message->getName() + "] message!");
        }

        WRENCH_INFO("Download request accepted (will receive file content on mailbox_name %s)",
                    mailbox_that_should_receive_file_content.c_str());

        if (this->buffer_size == 0) {

            throw std::runtime_error("FileTransferThread::downloadFileFromStorageService(): Zero buffer size not implemented yet");

        } else {

            try {
                bool done = false;
                // Receive the first chunk
                auto msg = S4U_Mailbox::getMessage(mailbox_that_should_receive_file_content);
                if (auto file_content_chunk_msg =
                        std::dynamic_pointer_cast<StorageServiceFileContentChunkMessage>(msg)) {
                    done = file_content_chunk_msg->last_chunk;
                } else {
                    throw std::runtime_error("FileTransferThread::downloadFileFromStorageService(): Received an unexpected [" +
                                             msg->getName() + "] message!");
                }

                // Receive chunks and write them to disk
                while (not done) {
                    // Issue the receive
                    auto req = S4U_Mailbox::igetMessage(mailbox_that_should_receive_file_content);

                    // Do the I/O
                    S4U_Simulation::writeToDisk(msg->payload,
                                                dst_location->getStorageService()->getHostname(),
                                                dst_location->getMountPoint());

                    // Wait for the comm to finish
                    msg = req->wait();
                    if (auto file_content_chunk_msg =
                            std::dynamic_pointer_cast<StorageServiceFileContentChunkMessage>(msg)) {
                        done = file_content_chunk_msg->last_chunk;
                    } else {
                        throw std::runtime_error("FileTransferThread::downloadFileFromStorageService(): Received an unexpected [" +
                                                 msg->getName() + "] message!");
                    }
                }
                // Do the I/O for the last chunk
                S4U_Simulation::writeToDisk(msg->payload,
                                            dst_location->getStorageService()->getHostname(),
                                            dst_location->getMountPoint());
            } catch (std::shared_ptr<NetworkError> &e) {
                throw;
            }
        }

    }

};
