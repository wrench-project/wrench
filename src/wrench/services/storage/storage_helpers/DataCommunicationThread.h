/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_DATACOMMUNICATIONTHREAD_H
#define WRENCH_DATACOMMUNICATIONTHREAD_H

#include <string>
#include <wrench/simgrid_S4U_util/S4U_Daemon.h>
#include <wrench/services/Service.h>

namespace wrench {

    class WorkflowFile;

    /** @brief A help class that implements the concept of a communication
     *  thread that performs a data communication
     */
    class DataCommunicationThread : public Service {

    public:
        /** @brief An enumerated type that denotes whether the communication thread is
         * sending or receiving data
         */
        enum DataCommunicationType {
            RECEIVING,
            SENDING,
        };

        DataCommunicationThread(std::string hostname,
                                WorkflowFile *file,
                                std::string partition,
                                DataCommunicationType communication_type,
                                std::string data_mailbox,
                                std::string answer_mailbox_if_copy,
                                std::string mailbox_to_notify,
                                SimulationTimestampFileCopyStart *start_timestamp = nullptr);

        int main() override;
        void cleanup(bool has_returned_from_main, int return_value) override;


            private:
        WorkflowFile *file;
        std::string partition;
        DataCommunicationType communication_type;
        std::string data_mailbox;
        std::string answer_mailbox_if_copy;
        std::string mailbox_to_notify;
        SimulationTimestampFileCopyStart *start_timestamp;
    };

}

#endif //WRENCH_DATACOMMUNICATIONTHREAD_H

