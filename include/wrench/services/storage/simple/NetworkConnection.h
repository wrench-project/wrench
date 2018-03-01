/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_NETWORKCONNECTION_H
#define WRENCH_NETWORKCONNECTION_H


#include <string>
#include <wrench/simgrid_S4U_util/S4U_PendingCommunication.h>
#include <wrench/workflow/execution_events/FailureCause.h>

namespace wrench {

    class WorkflowFile;

    class NetworkConnection {

    public:

        static constexpr unsigned char INCOMING_DATA = 0;
        static constexpr unsigned char OUTGOING_DATA = 1;
        static constexpr unsigned char INCOMING_CONTROL = 2;

        NetworkConnection(int type, WorkflowFile* file, std::string mailbox, std::string ack_mailbox);
        bool start();
        bool hasFailed();
        std::unique_ptr<SimulationMessage> getMessage();


        int type;
        WorkflowFile *file;
        std::string mailbox;
        std::string ack_mailbox;
        std::unique_ptr<S4U_PendingCommunication> comm;
        std::shared_ptr<FailureCause> failure_cause;
    };

};


#endif //WRENCH_NETWORKCONNECTION_H
