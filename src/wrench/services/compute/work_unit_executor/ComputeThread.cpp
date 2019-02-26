
/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <xbt.h>
#include <wrench-dev.h>
#include "ComputeThread.h"


XBT_LOG_NEW_DEFAULT_CATEGORY(compute_thread, "Log category for ComputeThread");


namespace wrench {


    /**
     * @brief Constructor
     * @param simulation: a pointer to the simulation object
     * @param hostname: the host on which the compute thread should run
     * @param flops: the number of flops to perform
     * @param reply_mailbox: the mailbox to which the "done/failed" message should be sent
     */
    ComputeThread::ComputeThread(Simulation *simulation, std::string hostname, double flops, std::string reply_mailbox) :
            Service(hostname, "compute_thread", "compute_thread") {
        this->simulation = simulation;
        this->flops = flops;
        this->reply_mailbox = reply_mailbox;
    }

    /**
     * @brief The main method of the compute thread
     * @return
     */
    int ComputeThread::main() {
        try {
            WRENCH_INFO("New compute thread (%.2f flops, will report to %s)", this->flops, reply_mailbox.c_str());
            S4U_Simulation::compute(this->flops);
        } catch (std::exception &e) {
            WRENCH_INFO("Probably got killed while I was computing");
            return 0;
        } catch (std::shared_ptr<HostError> &e) {
            WRENCH_INFO("Probably got killed while I was computing");
            return 0;
        }
        try {
            S4U_Mailbox::putMessage(this->reply_mailbox, new ComputeThreadDoneMessage());
        } catch (std::shared_ptr<NetworkError> &e) {
            WRENCH_INFO("Couldn't report on my completion to my parent");
        }

        return 0;
    }

    /**
     * @brief Terminate (brutally) the compute thread
     */
    void ComputeThread::kill() {
        try {
            this->killActor();
        } catch (std::shared_ptr<FatalFailure> &e) {
            WRENCH_INFO("Failed to kill a compute thread.. .perhaps it's already dead... nevermind");
        }
    }

};
