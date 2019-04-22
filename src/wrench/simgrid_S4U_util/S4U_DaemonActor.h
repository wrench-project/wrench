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
            // TODO: Setting up the on_exit function below to circumvent a SimGrid bug. This could go
            // TODO: back into the S4U_Daemon class in the future (when SimGrid doesn't "forget"
            // TODO: on_exit() calls when auto-restarting an actor).
            this->setupOnExitFunction();
            this->daemon->runMainMethod();
        }

    private:

        S4U_Daemon *daemon;

        void setupOnExitFunction();

        };

    /***********************/
    /** \endcond           */
    /***********************/
};


#endif //WRENCH_SIM4U_DAEMONWITHMAILBOXACTOR_H
