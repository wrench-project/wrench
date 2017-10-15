
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
#include <xbt/ex.hpp>


XBT_LOG_NEW_DEFAULT_CATEGORY(compute_thread, "Log category for ComputeThread");

//#define S4U_KILL_JOIN_WORKS

namespace wrench {

    ComputeThread::~ComputeThread() {
//      WRENCH_INFO("In COMPUTETHREAD DESTRUCTOR");
    }

    /**
     * @brief Constructor
     * @param flops: the number of flops to perform
     */
    ComputeThread::ComputeThread(double flops, std::string reply_mailbox) :
            S4U_Daemon("compute_thread_" + std::to_string(S4U_Mailbox::generateUniqueSequenceNumber())) {
      this->flops = flops;
      this->reply_mailbox = reply_mailbox;

      // Start my daemon on the host
      this->hostname = S4U_Simulation::getHostName();
      this->start(this->hostname, false);
    }

    int ComputeThread::main() {
      try {
        WRENCH_INFO("New compute threads beginnin %.2f-flop computation", this->flops);
        S4U_Simulation::compute(this->flops);
      } catch (std::exception &e) {
        WRENCH_INFO("Probably got killed while I was computing");
        return 0;
      }
      #ifndef S4U_KILL_JOIN_WORKS
      try {
        S4U_Mailbox::putMessage(this->reply_mailbox, new ComputeThreadDoneMessage());
      } catch (std::shared_ptr<NetworkError> &e) {
        WRENCH_INFO("Couldn't report on my completion to my parent");
      } catch (std::shared_ptr<FatalFailure> &e) {
        WRENCH_INFO("Couldn't report on my completion to my parent");
      }
      #endif

      return 0;
    }

    /**
     * kill()
     */
    void ComputeThread::kill() {
      try {
        this->kill_actor();
      } catch (std::shared_ptr<FatalFailure> &e) {
        WRENCH_INFO("Failed to kill a compute thread.. .perhaps it's already dead... nevermind");
      }
    }


    /**
    * join()
    */
    void ComputeThread::join() {
      try {
        this->join_actor();
      } catch (std::shared_ptr<FatalFailure> &e) {
        throw;
      }
    }


};
