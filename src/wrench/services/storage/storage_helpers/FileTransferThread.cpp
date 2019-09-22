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

WRENCH_LOG_NEW_DEFAULT_CATEGORY(file_transfer_thread, "Log category for File Transfer Thread");


namespace wrench {


    /**
     * @brief Constructor
     * @param hostname: host on which to run
     * @param parent: the parent storage service
     * @param file: the file corresponding to the connection
     * @param src: the transfer source
     * @param dst: the transfer destination
     * @param answer_mailbox_if_copy: the mailbox to send an answer to in case this was a file copy ("" if none). This
     *        will simply be reported to the parent service, who may use it as needed
     * @param buffer_size: the buffer size to use
     * @param start_timestamp: if this is a file copy, a start timestamp associated with it
     */
    FileTransferThread::FileTransferThread(std::string hostname,
                                           std::shared_ptr<StorageService> parent,
                                           WorkflowFile *file,
                                           std::pair<LocationType, std::string> src,
                                           std::pair<LocationType, std::string> dst,
                                           std::string answer_mailbox_if_copy,
                                           unsigned long buffer_size,
                                           SimulationTimestampFileCopyStart *start_timestamp) :
            Service(hostname, "file_transfer_thread", "file_transfer_thread"),
            parent(parent),
            file(file),
            src(src),
            dst(dst),
            answer_mailbox_if_copy(answer_mailbox_if_copy),
            buffer_size(buffer_size),
            start_timestamp(start_timestamp)
    {

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

        WRENCH_INFO("New FileTransferThread (file=%s, src=%s, dst=%s, answer_mailbox_if_copy=%s",
                    file->getID().c_str(),
                    src.second.c_str(),
                    dst.second.c_str(),
                    answer_mailbox_if_copy.c_str());

        // Create a message to send back (some field of which may be overwritten below)
        msg_to_send_back = new FileTransferThreadNotificationMessage(
                this->getSharedPtr<FileTransferThread>(),
                this->file,
                this->src,
                this->dst,
                this->answer_mailbox_if_copy,
                true, nullptr,
                this->start_timestamp);

        if ((src.first == LocationType::LOCAL_PARTITION) && (dst.first == LocationType::MAILBOX)) {
            /** Sending a local file to the network **/
            try {
                sendLocalFileToNetwork(this->file, this->src.second, this->dst.second);
            } catch (std::shared_ptr<NetworkError> &failure_cause) {
                WRENCH_INFO("FileTransferThread::main(): Network error (%s)", failure_cause->toString().c_str());
                msg_to_send_back->success = false;
                msg_to_send_back->failure_cause = failure_cause;
            }
        } else if ((src.first == LocationType::MAILBOX) && (dst.first == LocationType::LOCAL_PARTITION)) {
            /** Receiving a file from the network **/

            try {
                receiveFileFromNetwork(this->file, this->src.second, this->dst.second);
            } catch (std::shared_ptr<NetworkError> &failure_cause) {
                WRENCH_INFO("FileTransferThread::main() Network error (%s)", failure_cause->toString().c_str());
                msg_to_send_back->success = false;
                msg_to_send_back->failure_cause = failure_cause;
            }

        } else if ((src.first == LocationType::LOCAL_PARTITION) && (dst.first == LocationType::LOCAL_PARTITION)) {
            /** Copying a file local file */
            copyFileLocally(this->file, src.second, dst.second);

        } else if ((src.first == LocationType::STORAGE_SERVICE) && (dst.first == LocationType::LOCAL_PARTITION)) {
            /** Downloading a file from another storage service */
            try {
                downloadFileFromStorageService(this->file, this->dst.second, this->src.second);
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
    * @param partition: the source mailbox
    * @param mailbox: the destination partition
    *
    * @throw shared_ptr<FailureCause>
    */
    void FileTransferThread::receiveFileFromNetwork(WorkflowFile *file, std::string mailbox, std::string partition) {

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
                    S4U_Simulation::writeToDisk(msg->payload, partition);
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
                // Sleep to simulate I/O for the last chunk
                S4U_Simulation::writeToDisk(msg->payload, partition);
            } catch (std::shared_ptr<NetworkError> &e) {
                throw;
            }
        }

    }


    /**
     * @brief Method to send a file from the local disk to the network
     * @param file: the file
     * @param partition: the source local partition
     * @param mailbox: the destination network mailbox
     *
     * @throw shared_ptr<FailureCause>
     */
    void FileTransferThread::sendLocalFileToNetwork(WorkflowFile *file, std::string partition, std::string mailbox) {

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
                    S4U_Simulation::readFromDisk(chunk_size, partition);
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
     * @brief Method to download a file from a remote storage service
     * @param file: the file to download
     * @param partition: the partition to write the file to
     * @param storage_service_and_partition: the source storage service and partition  (format: "s:p")
     */
    void FileTransferThread::downloadFileFromStorageService(WorkflowFile *file, std::string partition, std::string storage_service_and_partition) {
        // Get the service by name and the src_partition name
        std::vector<std::string> tokens;
        boost::split(tokens, storage_service_and_partition, boost::is_any_of(":"));
        std::string storage_service_name = tokens.at(0);
        std::string src_partition = tokens.at(1);

        std::shared_ptr<StorageService> ss = Service::getServiceByName<StorageService>(storage_service_name);
        if (ss == nullptr) {
            throw std::runtime_error("FileTransferThread::downloadFileFromStorageService(): Cannot find service " + storage_service_name + " by name! Fatal error");
        }

        // Download the file
        try {
            ss->downloadFile(file, src_partition, partition, this->buffer_size);
        } catch (WorkflowExecutionException &e) {
            throw e.getCause();
        }
    }

    /**
     * @brief Method to copy a file localy
     * @param file: the file to copy
     * @param src_partition: the source partition
     * @param dst_partition: the destination partition
     */
    void FileTransferThread::copyFileLocally(WorkflowFile *file, std::string src_partition, std::string dst_partition) {
        double remaining = file->getSize();
        double to_send = std::min<double>(this->buffer_size, remaining);

        /** Ideal Fluid model buffer size */
        if (this->buffer_size == 0) {

            throw std::runtime_error(
                    "FileTransferThread::copyFileLocally(): Zero buffer size not implemented yet");

        } else {
            // Read the first chunk
            S4U_Simulation::readFromDisk(to_send, src_partition);
            // start the pipeline
            while (remaining > this->buffer_size) {
                // Write to disk. TODO: Make this asynchronous!
                S4U_Simulation::writeToDisk(this->buffer_size, dst_partition);
                S4U_Simulation::readFromDisk(this->buffer_size, src_partition);

                remaining -= this->buffer_size;
            }
            // Write the last chunk
            S4U_Simulation::writeToDisk(remaining, dst_partition);
        }

    }


    /**
     * @brief Download a file to a local partition/disk
     * @param file: the file to download
     * @param src_mountpoint: the source mount point
     * @param dst_mountpoint: the local mount point
     * @param downloader_buffer_size: buffer size of the downloader (0 means use "ideal fluid model")
     */
    void StorageService::downloadFile(WorkflowFile *file, std::string src_mountpoint, std::string dst_mountpoint, unsigned long downloader_buffer_size) {

        if (file == nullptr) {
            throw std::invalid_argument("StorageService::downloadFile(): Invalid arguments");
        }

        WRENCH_INFO("Initiating a file read operation for file %s on storage service %s",
                    file->getID().c_str(), this->getName().c_str());

        // Check that the service is up
        assertServiceIsUp();

        // Check that the buffer size is compatible
        if (((downloader_buffer_size == 0) && (this->buffer_size != 0)) or
            ((downloader_buffer_size != 0) && (this->buffer_size == 0))) {
            throw std::invalid_argument("StorageService::downloadFile(): Incompatible buffer size specs (both must be zero, or both must be non-zero");
        }

        // Empty partition means "/"
        if (src_mountpoint.empty()) {
            src_mountpoint = "/";
        }
        if (dst_mountpoint.empty()) {
            dst_mountpoint = "/";
        }

        // Send a message to the daemon
        std::string request_answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("read_file_request");
        std::string mailbox_that_should_receive_file_content = S4U_Mailbox::generateUniqueMailboxName("read_file_chunks");

        try {
            S4U_Mailbox::putMessage(this->mailbox_name,
                                    new StorageServiceFileReadRequestMessage(request_answer_mailbox,
                                                                             mailbox_that_should_receive_file_content,
                                                                             file,
                                                                             src_mountpoint,
                                                                             std::min<unsigned long>(this->buffer_size, downloader_buffer_size),
                                                                             this->getMessagePayloadValue(
                                                                                     StorageServiceMessagePayload::FILE_READ_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        // Wait for a reply to the request
        std::shared_ptr<SimulationMessage> message = nullptr;

        try {
            message = S4U_Mailbox::getMessage(request_answer_mailbox, this->network_timeout);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        if (auto msg = std::dynamic_pointer_cast<StorageServiceFileReadAnswerMessage>(message)) {
            // If it's not a success, throw an exception
            if (not msg->success) {
                throw WorkflowExecutionException(msg->failure_cause);
            }
        } else {
            throw std::runtime_error("StorageService::downloadFile(): Received an unexpected [" +
                                     message->getName() + "] message!");
        }

        WRENCH_INFO("File read request accepted (will receive file content on mailbox_name %s)",
                    mailbox_that_should_receive_file_content.c_str());

        if (this->buffer_size == 0) {

            throw std::runtime_error("downloadFile::writeFile(): Zero buffer size not implemented yet");

        } else {
            try {
                bool done = false;
                // Receive the first chunk
                auto msg = S4U_Mailbox::getMessage(mailbox_that_should_receive_file_content);
                if (auto file_content_chunk_msg =
                        std::dynamic_pointer_cast<StorageServiceFileContentChunkMessage>(msg)) {
                    done = file_content_chunk_msg->last_chunk;
                } else {
                    throw std::runtime_error("FileTransferThread::downloadFile() : Received an unexpected [" +
                                             msg->getName() + "] message!");
                }

                // Receive chunks and write them to disk
                while (not done) {
                    // Issue the receive
                    auto req = S4U_Mailbox::igetMessage(mailbox_that_should_receive_file_content);
                    // Do the I/O
                    S4U_Simulation::writeToDisk(msg->payload, dst_mountpoint);

                    // Wait for the comm to finish
                    msg = req->wait();
                    if (auto file_content_chunk_msg =
                            std::dynamic_pointer_cast<StorageServiceFileContentChunkMessage>(msg)) {
                        done = file_content_chunk_msg->last_chunk;
                    } else {
                        throw std::runtime_error("FileTransferThread::downloadFile() : Received an unexpected [" +
                                                 msg->getName() + "] message!");
                    }
                }
                // Do the I/O for the last chunk
                S4U_Simulation::writeToDisk(msg->payload, dst_mountpoint);
            } catch (std::shared_ptr<NetworkError> &e) {
                throw WorkflowExecutionException(e);
            }
        }

    }

};
