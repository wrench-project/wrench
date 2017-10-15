/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include "wrench/services/storage/simple/IncomingFile.h"

namespace wrench {

    /**
     * @brief Constructor
     * @param file: the file
     * @param send_file_copy_ack: whether to send a file copy ack
     * @param ack_mailbox: the mailbox to send the ack
     */
    IncomingFile::IncomingFile(WorkflowFile *file,
                               bool send_file_copy_ack, std::string ack_mailbox) :
      file(file), send_file_copy_ack(send_file_copy_ack), ack_mailbox(ack_mailbox){

    }
};