/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/logging/TerminalOutput.h"
#include "wrench/simgrid_S4U_util/S4U_CommPort.h"
#include "wrench/simulation/SimulationMessage.h"
#include "wrench/services/storage/StorageService.h"
#include "wrench/exceptions/ExecutionException.h"
#include "wrench/managers/data_movement_manager/FileWriterThread.h"
#include "wrench/managers/data_movement_manager/DataMovementManagerMessage.h"

#include <memory>
#include <utility>

WRENCH_LOG_CATEGORY(wrench_core_data_movement_manager_file_writer_thread, "Log category for Data Movement Manager FileWriterThread");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param hostname: the hostname on which the data movement manager is to run
     * @param creator_commport: the commport of the manager's creator
     * @param location: the write location
     */
    FileWriterThread::FileWriterThread(const std::string& hostname,
                                       S4U_CommPort *creator_commport,
                                       std::shared_ptr<FileLocation> location) : Service(hostname, "file_writer_thread") {
        this->creator_commport = creator_commport;
        this->location = std::move(location);
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
        this->creator_commport->putMessage(msg);

        return 0;
    }


}// namespace wrench
