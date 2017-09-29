/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_COMPUTETHREAD_H
#define WRENCH_COMPUTETHREAD_H

#include <simgrid_S4U_util/S4U_Mailbox.h>
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "StandardJobExecutorMessage.h"


namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief The actor for the S4U_DaemonWithMailbox abstraction
     */
    class ComputeThread {

    public:

        /**
         * @brief Constructor
         * @param flops: the number of flops to perform
         */
        explicit ComputeThread(double flops, std::string reply_mailbox) {
          this->flops = flops;
          this->reply_mailbox = reply_mailbox;
        }

        /**
         * @brief The S4U way of doing things
         */
        void operator()() {
          this->compute(this->flops);
          #ifndef S4U_KILL_JOIN_WORKS
          try {
            S4U_Mailbox::putMessage(this->reply_mailbox, new ComputeThreadDoneMessage());
          } catch (std::exception &e) {
            // Nothing, perhaps my parent has died

          }
          #endif
        }


    private:
        double flops;
        std::string reply_mailbox;

        void compute(double flops);

    };

    /***********************/
    /** \endcond           */
    /***********************/
};

#endif //WRENCH_COMPUTETHREAD_H
