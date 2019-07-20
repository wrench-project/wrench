/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_DATACOMMUNICATIONTHREADMESSAGE_H
#define WRENCH_DATACOMMUNICATIONTHREADMESSAGE_H


#include <memory>

#include <wrench/services/ServiceMessage.h>
#include <wrench/workflow/execution_events/FailureCause.h>
#include <wrench/services/file_registry/FileRegistryService.h>
#include <wrench/simulation/SimulationTimestampTypes.h>
#include <wrench/simulation/Simulation.h>
#include <wrench/simulation/SimulationOutput.h>
#include "DataCommunicationThread.h"

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Top-level class for messages received/sent by a DataCommunicationThread
     */
    class DataCommunicationThreadMessage : public ServiceMessage {
    protected:
        DataCommunicationThreadMessage(std::string name, double payload) :
                ServiceMessage("DataCommunicationThreadMessage::" + name, payload) {}
    };


    /**
     * @brief A message sent to by a DataCommunicationThread to report on success/failure of the communication
     */
    class DataCommunicationThreadNotificationMessage : public DataCommunicationThreadMessage {
    public:
        DataCommunicationThreadNotificationMessage(std::shared_ptr<DataCommunicationThread> data_communication_thread,
                                                   WorkflowFile *file, std::string partition,
                                                   DataCommunicationThread::DataCommunicationType communication_type,
                                                   std::string answer_mailbox_if_copy,
                                                   bool success, std::shared_ptr<FailureCause> failure_cause,
                                                   SimulationTimestampFileCopyStart *start_time_stamp) :
                DataCommunicationThreadMessage("DataCommunicationThreadNotificationMessage", 0),
                data_communication_thread(data_communication_thread),
                file(file), partition(partition), communication_type(communication_type),
                answer_mailbox_if_copy(answer_mailbox_if_copy), success(success),
                failure_cause(failure_cause), start_time_stamp(start_time_stamp) {}

        /** @brief Data communiucation thread that sent this message */
        std::shared_ptr<DataCommunicationThread> data_communication_thread;
        /** @brief File that was being communicated */
        WorkflowFile *file;
        /** @brief Partition */
        std::string partition;
        /** @brief Communication type (SENDING or RECEIVING) */
        DataCommunicationThread::DataCommunicationType communication_type;
        /** @brief If this was a file copy, the mailbox to which an answer should be send */
        std::string answer_mailbox_if_copy;
        /** @brief Whether the communication succeeded or not */
        bool success;
        /** @brief The failure cause is case of a failure */
        std::shared_ptr<FailureCause> failure_cause;
        /** @brief A start time stamp */
        SimulationTimestampFileCopyStart *start_time_stamp;
    };


    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_DATACOMMUNICATIONTHREADMESSAGE_H
