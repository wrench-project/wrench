
/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/logging/TerminalOutput.h"
#include <wrench-dev.h>
#include "ComputeThread.h"

WRENCH_LOG_CATEGORY(wrench_core_compute_thread, "Log category for ComputeThread");

namespace wrench {


    /**
     * @brief Constructor
     * @param hostname: the host on which the compute thread should run
     * @param flops: the number of flops to perform
     * @param reply_mailbox: the mailbox to which the "done/failed" message should be sent
     */
    ComputeThread::ComputeThread(std::string hostname, double flops, std::string reply_mailbox)
            :
            Service(hostname, "compute_thread", "compute_thread") {
        this->flops = flops;
        this->reply_mailbox = reply_mailbox;
    }

    /**
     * @brief The main method of the compute thread
     * @return
     */
    int ComputeThread::main() {
        WRENCH_INFO("New compute thread (%.2f flops) on core (%.2f flop/sec) will report to %s)",
                    this->flops,
                    S4U_Simulation::getFlopRate(),
                    reply_mailbox.c_str());
        S4U_Simulation::compute(this->flops);
        try {
            S4U_Mailbox::putMessage(this->reply_mailbox, new ComputeThreadDoneMessage());
        } catch (std::shared_ptr<NetworkError> &e) {
            WRENCH_INFO("Couldn't report on my completion to my parent [ignoring and returning as if everything's ok]");
            return 0;
        }
        return 0;
    }

    /**
     * @brief Terminate (brutally) the compute thread
     */
    void ComputeThread::kill() {
        this->killActor();
    }

    /**
     * @brief Cleanup method that overrides the base method and does nothing as a compute thread
     *        does not need to implement any particular fault-tolerant behavior (it runs on the
     *        same how as a workunit executor, which is also dead anyway)
     * @param has_returned_from_main: whether the daemon has terminated cleanly (i.e., returned from main)
     * @param return_value: main's return value
     */
    void ComputeThread::cleanup(bool has_returned_from_main, int return_value) {
        return;
    }

};
