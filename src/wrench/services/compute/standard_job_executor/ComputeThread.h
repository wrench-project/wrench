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

#include "wrench/simgrid_S4U_util/S4U_Simulation.h"


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
        explicit ComputeThread(double flops) {
          this->flops = flops;
        }

        /**
         * @brief The S4U way of doing things
         */
        void operator()() {
          this->compute(this->flops);
        }


    private:
        double flops;

        void compute(double flops);

    };

    /***********************/
    /** \endcond           */
    /***********************/
};

#endif //WRENCH_COMPUTETHREAD_H
