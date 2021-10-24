/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_COMPUTE_THREAD_H
#define WRENCH_COMPUTE_THREAD_H

#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "wrench/services/helper_services/standard_job_executor/StandardJobExecutorMessage.h"


namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    class Simulation;

    /**
     * @brief A one-shot service that simulates a CPU-bound thread that
     *        performs a given number of flops and then reports to
     *        some mailbox saying "I am done"
     */
    class ComputeThread : public Service {

    public:

        ComputeThread(std::string hostname, double flops, std::string reply_mailbox);

        int main() override;

        void cleanup(bool has_returned_from_main, int return_value) override;

        void kill();

    private:
        double flops;
        std::string reply_mailbox;

    };

    /***********************/
    /** \endcond           */
    /***********************/
};

#endif //WRENCH_COMPUTE_THREAD_H
