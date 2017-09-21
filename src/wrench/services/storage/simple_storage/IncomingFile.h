/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_INCOMINGFILE_H
#define WRENCH_INCOMINGFILE_H


#include <string>

namespace wrench {

    class S4U_PendingCommunication;
    class WorkflowFile;

    /**
     * \cond INTERNAL
     */
    class IncomingFile {
    public:

        IncomingFile(WorkflowFile *file, bool send_file_copy_ack, std::string ack_mailbox);

        WorkflowFile *file;
        bool send_file_copy_ack;
        std::string ack_mailbox;
    };

    /**
     * \endcond
     */

};


#endif //WRENCH_INCOMINGFILE_H
