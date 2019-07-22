/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_FILETRANSFERTHREAD_H
#define WRENCH_FILETRANSFERTHREAD_H

#include <string>
#include <wrench/simgrid_S4U_util/S4U_Daemon.h>
#include <wrench/services/Service.h>

namespace wrench {

    class WorkflowFile;
    class SimulationTimestampFileCopyStart;

    /** @brief A help class that implements the concept of a communication
     *  thread that performs a file transfer
     */
    class FileTransferThread : public Service {

    public:

        /** @brief An enumerated type that denotes whether a src/dst
         * is local partition or a (remote) mailbox
         */
        enum LocationType {
            LOCAL_PARTITION,
            MAILBOX
        };


        FileTransferThread(std::string hostname,
                                WorkflowFile *file,
                                std::pair<LocationType, std::string> src,
                                std::pair<LocationType, std::string> dst,
                                std::string answer_mailbox_if_copy,
                                std::string mailbox_to_notify,
                                double local_copy_data_transfer_rate,
                                unsigned long copy_buffer_size,
                                SimulationTimestampFileCopyStart *start_timestamp = nullptr);

        int main() override;
        void cleanup(bool has_returned_from_main, int return_value) override;


    private:
        WorkflowFile *file;
        std::pair<LocationType, std::string> src;
        std::pair<LocationType, std::string> dst;
        std::string answer_mailbox_if_copy;
        std::string mailbox_to_notify;
        double local_copy_data_transfer_rate;
        unsigned long copy_buffer_size;
        SimulationTimestampFileCopyStart *start_timestamp;

        void receiveFileFromNetwork(WorkflowFile *file, std::string partition, std::string mailbox);
        void sendLocalFileToNetwork(WorkflowFile *file, std::string partition, std::string mailbox);

    };

}

#endif //WRENCH_FILETRANSFERTHREAD_H

