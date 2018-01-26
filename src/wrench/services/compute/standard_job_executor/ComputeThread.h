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

#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "StandardJobExecutorMessage.h"



namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief A compute thread
     */
    class ComputeThread : public S4U_Daemon {

    public:

        ~ComputeThread();

        ComputeThread(double flops, std::string reply_mailbox);

        int main();

        void kill();
        void join();


    private:
        double flops;
        std::string reply_mailbox;

    };

    /***********************/
    /** \endcond           */
    /***********************/
};

#endif //WRENCH_COMPUTETHREAD_H
