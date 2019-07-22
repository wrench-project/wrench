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
#include "FileTransferThread.h"
#include "FileTransferThreadMessage.h"

#include <wrench-dev.h>

WRENCH_LOG_NEW_DEFAULT_CATEGORY(file_transfer_thread, "Log category for File Transfer Thread");


namespace wrench {


    /**
     * @brief Constructor
     * @param hostname: host on which to run
     * @param file: the file corresponding to the connection
     * @param src: the transfer source
     * @param dst: the transfer destination
     * @param answer_mailbox_if_copy: the mailbox to send an answer to in case this was a file copy ("" if none)
     * @param mailbox_to_notify: the mailbox to notify once transfer is completed or has failed
     * @param local_copy_data_transfer_rate: local data copy transfer rate
     * @param copy_buffer_size: the copy buffer size
     * @param start_timestamp: if this is a file copy, a start timestamp associated with it
     */
    FileTransferThread::FileTransferThread(std::string hostname,
                                           WorkflowFile *file,
                                           std::pair<LocationType, std::string> src,
                                           std::pair<LocationType, std::string> dst,
                                           std::string answer_mailbox_if_copy,
                                           std::string mailbox_to_notify,
                                           double local_copy_data_transfer_rate,
                                           unsigned long copy_buffer_size,
                                           SimulationTimestampFileCopyStart *start_timestamp) :
            Service(hostname, "file_transfer_thread", "file_transfer_thread"),
            file(file),
            src(src),
            dst(dst),
            answer_mailbox_if_copy(answer_mailbox_if_copy),
            mailbox_to_notify(mailbox_to_notify),
            local_copy_data_transfer_rate(local_copy_data_transfer_rate),
            copy_buffer_size(copy_buffer_size),
            start_timestamp(start_timestamp)
    {
        if ((src.first == FileTransferThread::LocationType::MAILBOX) and
            (dst.first == FileTransferThread::LocationType::MAILBOX)) {
            throw std::invalid_argument("DataCommunicationThread::DataCommunicationThread(): the source and the destination cannot both be of type MAILBOX");
        }

    }

    void FileTransferThread::cleanup(bool has_returned_from_main, int return_value) {
        // Do nothing. It's fine to just die
    }

    /**
     * @brief Main method
     * @return 0 on success, non-zero otherwise
     */
    int FileTransferThread::main() {

        // Perform the transfer
        FileTransferThreadNotificationMessage *msg_to_send_back = nullptr;
        std::shared_ptr<NetworkError> failure_cause = nullptr;

        WRENCH_INFO("New DataCommunicationThread (file=%s, src=%s, dst=%s, answer_mailbox_if_copy=%s",
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
                WRENCH_INFO("DataCommunicationThread::main(): Network error (%s)", failure_cause->toString().c_str());
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
            if (this->local_copy_data_transfer_rate != DBL_MAX) {
                Simulation::sleep(this->file->getSize() / this->local_copy_data_transfer_rate);
            }
        }


        try {
            // Send report back to the service
            // (TODO: making this a dput causes a problem... perhaps a dput right before death bug in SimGrid (again?))
            S4U_Mailbox::putMessage(this->mailbox_to_notify, msg_to_send_back);
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

        // Receive chunks and write them to disk
        while (not done) {
            // Issue the receive
            auto req = S4U_Mailbox::igetMessage(mailbox);
            // Sleep to simulate I/O
            if (this->local_copy_data_transfer_rate != DBL_MAX) {
                Simulation::sleep(msg->payload / this->local_copy_data_transfer_rate);
            }
            // Wait for the comm to finish
            msg = req->wait();
            if (auto file_content_chunk_msg =
                    std::dynamic_pointer_cast<StorageServiceFileContentChunkMessage>(msg)) {
                done = file_content_chunk_msg->last_chunk;
            } else {
                throw std::runtime_error("FileTransferThread::receiveFileFromNetwork() : Received an unexpected [" +
                                         msg->getName() + "] message!");
            }
        }
        // Sleep to simulate I/O for the last chunk
        if (this->local_copy_data_transfer_rate != DBL_MAX) {
            Simulation::sleep(msg->payload / this->local_copy_data_transfer_rate);
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

        /** Infinite buffer size */
        if (this->copy_buffer_size == 0) {

            throw std::runtime_error("FileTransferThread::sendLocalFileToNetwork(): Zero buffer size not implemented yet");

        } else {

            /** Finite, non-zero buffer size */
            double remaining = file->getSize();
            double to_send = std::min<double>(this->copy_buffer_size, remaining);

            // Read the first chunk
            if (this->local_copy_data_transfer_rate != DBL_MAX) {
                Simulation::sleep(to_send / this->local_copy_data_transfer_rate);
            }
            // start the pipeline
            while (remaining > this->copy_buffer_size) {
                // Issue asynchronous comm
                auto req = S4U_Mailbox::iputMessage(mailbox,
                                                    new StorageServiceFileContentChunkMessage(this->file,
                                                                                              this->copy_buffer_size,
                                                                                              false));
                // Sleep to simulate I/O
                if (this->local_copy_data_transfer_rate != DBL_MAX) {
                    Simulation::sleep(to_send / this->local_copy_data_transfer_rate);
                }
                // Wait for the comm to finish
                req->wait();
                remaining -= this->copy_buffer_size;
            }
            // Send the last chunk
            S4U_Mailbox::putMessage(mailbox, new StorageServiceFileContentChunkMessage(this->file, remaining, true));
        }

    }


};