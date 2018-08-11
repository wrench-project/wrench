/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *
 */

#ifndef WRENCH_SIM4U_DAEMONACTOR_H
#define WRENCH_SIM4U_DAEMONACTOR_H


#include <xbt.h>
#include <string>
#include <vector>
#include <iostream>
#include <wrench-dev.h>
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"


    /***********************/
    /** \cond INTERNAL     */
    /***********************/

namespace wrench {

    class S4U_Daemon;

    /**
     * @brief The S4U actor that's the foundation for the S4U_Daemon abstraction
     */
    class S4U_DaemonActor {

    public:

        /**
         * @brief Constructor
         * @param d: a daemon instance
         */
        explicit S4U_DaemonActor(S4U_Daemon *d) {
          this->daemon = d;
        }

        /**
         * @brief The S4U way of defining the actor's "main" method
         */
        void operator()() {
//          std::cerr << "ACTOR: RUNNING MAIN: " << this->daemon->getName() << "\n";
          try {
            S4U_Simulation::computeZeroFlop();
            this->daemon->main();
            wrench::S4U_Simulation::sleep(0.001);
          } catch (std::exception &e) {
            throw;
          }
          this->daemon->setTerminated();
        }

    private:
        S4U_Daemon *daemon;

    };

    /***********************/
    /** \endcond           */
    /***********************/
};


#endif //WRENCH_SIM4U_DAEMONWITHMAILBOXACTOR_H
