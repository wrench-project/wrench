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
     * @param start_timestamp: if this is a file copy, a start timestamp associated with it
     */
    FileTransferThread::FileTransferThread(std::string hostname,
                                           WorkflowFile *file,
                                           std::pair<LocationType, std::string> src,
                                           std::pair<LocationType, std::string> dst,
                                           std::string answer_mailbox_if_copy,
                                           std::string mailbox_to_notify,
                                           double local_copy_data_transfer_rate,
                                           SimulationTimestampFileCopyStart *start_timestamp) :
            Service(hostname, "file_transfer_thread", "file_transfer_thread"),
            file(file),
            src(src),
            dst(dst),
            answer_mailbox_if_copy(answer_mailbox_if_copy),
            mailbox_to_notify(mailbox_to_notify),
            local_copy_data_transfer_rate(local_copy_data_transfer_rate),
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
//        FileTransferThreadNotificationMessage *msg_to_send_back = nullptr;
        bool success = true;
        std::shared_ptr<NetworkError> failure_cause = nullptr;

        WRENCH_INFO("New DataCommunicationThread (file=%s, src=%s, dst=%s, answer_mailbox_if_copy=%s",
                    file->getID().c_str(),
                    src.second.c_str(),
                    dst.second.c_str(),
                    answer_mailbox_if_copy.c_str());


        if ((src.first == LocationType::LOCAL_PARTITION) && (dst.first == LocationType::MAILBOX)) {
            /** Sending a local file to the network **/
            try {
                S4U_Mailbox::putMessage(this->dst.second,
                                        new StorageServiceFileContentMessage(this->file));
            } catch (std::shared_ptr<NetworkError> &e) {
                WRENCH_INFO("DataCommunicationThread::main(): Network error (%s)", failure_cause->toString().c_str());
               success = false;
               failure_cause = e;
            }
        } else if ((src.first == LocationType::MAILBOX) && (dst.first == LocationType::LOCAL_PARTITION)) {
            /** Sending a local file to the network **/

            try {
                S4U_Mailbox::getMessage(this->src.second);
            } catch (std::shared_ptr<NetworkError> &e) {
                WRENCH_INFO("DataCommunicationThread::main(): Network error (%s)", failure_cause->toString().c_str());
                success = false;
                failure_cause = e;
            }
        } else if ((src.first == LocationType::LOCAL_PARTITION) && (dst.first == LocationType::LOCAL_PARTITION)) {

            Simulation::sleep(this->file->getSize() / this->local_copy_data_transfer_rate);
        }

        // Send report
        auto msg_to_send_back = new FileTransferThreadNotificationMessage(
                this->getSharedPtr<FileTransferThread>(),
                this->file,
                this->src,
                this->dst,
                this->answer_mailbox_if_copy,
                success, failure_cause,
                this->start_timestamp);
        S4U_Mailbox::dputMessage(this->mailbox_to_notify, msg_to_send_back);

        return 0;
    }

};