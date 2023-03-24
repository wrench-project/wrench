/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/logging/TerminalOutput.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/simulation/SimulationMessage.h"
#include "wrench/services/storage/StorageService.h"
#include "wrench/exceptions/ExecutionException.h"
#include "wrench/managers/data_movement_manager/FileWriterThread.h"
#include "wrench/managers/data_movement_manager/DataMovementManagerMessage.h"

#include <memory>

WRENCH_LOG_CATEGORY(wrench_core_data_movement_manager_file_writer_thread, "Log category for Data Movement Manager FileWriterThread");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param hostname: the hostname on which the data movement manager is to run
     * @param creator_mailbox: the mailbox of the manager's creator
     * @param location: the write location
     */
    FileWriterThread::FileWriterThread(std::string hostname,
                                       simgrid::s4u::Mailbox *creator_mailbox,
                                       std::shared_ptr<FileLocation> location) : Service(hostname, "file_writer_thread") {
        this->creator_mailbox = creator_mailbox;
        this->location = location;
    }

    /**
     * @brief Main method of the file reader thread
     * @return 0 on successful termination, non-zero otherwise
     */
    int FileWriterThread::main() {
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_YELLOW);

        SimulationMessage *msg;
        try {
            StorageService::writeFileAtLocation(this->location);
            msg = new DataMovementManagerFileWriterThreadMessage(this->location, true, nullptr);
        } catch (ExecutionException &e) {
            msg = new DataMovementManagerFileWriterThreadMessage(this->location, false, e.getCause());
        }
        S4U_Mailbox::putMessage(this->creator_mailbox, msg);

        return 0;
    }


}// namespace wrench
