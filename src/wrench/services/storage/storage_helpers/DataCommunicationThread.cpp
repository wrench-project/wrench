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
#include "DataCommunicationThread.h"
#include "DataCommunicationThreadMessage.h"

#include <wrench-dev.h>

WRENCH_LOG_NEW_DEFAULT_CATEGORY(data_communication_thread, "Log category for Data Communication Thread");


namespace wrench {


    /**
     * @brief Constructor
     * @param hostname: host on which to run
     * @param file: the file corresponding to the connection
     * @param partition: the src/dst partition
     * @param communication_type: SENDING or RECEIVING
     * @param data_mailbox: the mailbox for the communication
     * @param answer_mailbox_if_copy: the mailbox to send an answer to in case this was a file copy ("" if none)
     * @param mailbox_to_notify: the mailbox to notify one communication is completed or has failed
     * @param start_timestamp: if this is a file copy, a start timestamp associated with it
     */
    DataCommunicationThread::DataCommunicationThread(std::string hostname,
                                                     WorkflowFile *file,
                                                     std::string partition,
                                                     DataCommunicationThread::DataCommunicationType communication_type,
                                                     std::string data_mailbox,
                                                     std::string answer_mailbox_if_copy,
                                                     std::string mailbox_to_notify,
                                                     SimulationTimestampFileCopyStart *start_timestamp)  :
            Service(hostname, "data_communication_thread", "data_communication_thread"),
            file(file),
            partition(partition),
            communication_type(communication_type),
            data_mailbox(data_mailbox),
            answer_mailbox_if_copy(answer_mailbox_if_copy),
            mailbox_to_notify(mailbox_to_notify),
            start_timestamp(start_timestamp)
    { }

    void DataCommunicationThread::cleanup(bool has_returned_from_main, int return_value) {
        // Do nothing. It's fine to just die
    }

    /**
     * @brief Main method
     * @return 0 on success, non-zero otherwise
     */
    int DataCommunicationThread::main() {

        // Perform the communication
        DataCommunicationThreadNotificationMessage *msg_to_send_back = nullptr;
        std::shared_ptr<NetworkError> failure_cause = nullptr;

        /*
        WRENCH_INFO("New DataCommunicationThread (file=%s, partition=%s, type=%s, data_mailbox=%s, answer_mailbox_if_copy=%s",
                    file->getID().c_str(),
                    partition.c_str(),
                    ((communication_type == DataCommunicationThread::DataCommunicationType::SENDING) ? "sending" : "receiving"),
                    data_mailbox.c_str(),
                    answer_mailbox_if_copy.c_str());
        */

        try {
            if (this->communication_type == DataCommunicationType::SENDING) {
                S4U_Mailbox::putMessage(this->data_mailbox,
                                        new StorageServiceFileContentMessage(this->file));
            } else {
                S4U_Mailbox::getMessage(this->data_mailbox);
            }
            msg_to_send_back = new DataCommunicationThreadNotificationMessage(
                    this->getSharedPtr<DataCommunicationThread>(),
                    this->file,
                    this->partition,
                    this->communication_type,
                    this->answer_mailbox_if_copy,
                    true, nullptr,
                    this->start_timestamp);
        } catch (std::shared_ptr<NetworkError> &failure_cause) {
            WRENCH_INFO("DataCommunicationThread::main(): Network error (%s)", failure_cause->toString().c_str());
            msg_to_send_back = new DataCommunicationThreadNotificationMessage(
                    this->getSharedPtr<DataCommunicationThread>(),
                    this->file,
                    this->partition,
                    this->communication_type,
                    this->answer_mailbox_if_copy,
                    false, failure_cause,
                    this->start_timestamp);
        }

        // Send report
        S4U_Mailbox::dputMessage(this->mailbox_to_notify, msg_to_send_back);

        return 0;
    }

};