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

#ifndef WRENCH_SIM4U_DAEMONWITHMAILBOXACTOR_H
#define WRENCH_SIM4U_DAEMONWITHMAILBOXACTOR_H

#include <xbt.h>
#include <string>
#include <vector>
#include <iostream>

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    class S4U_DaemonWithMailbox;

    /**
     * @brief The actor for the S4U_DaemonWithMailbox abstraction
     */
    class S4U_DaemonWithMailboxActor {

    public:

        explicit S4U_DaemonWithMailboxActor(S4U_DaemonWithMailbox *d) {
          this->daemon = d;
        }

        void operator()() {
          this->daemon->main();
          this->daemon->setTerminated();
        }

    private:
        S4U_DaemonWithMailbox *daemon;

    };

    /***********************/
    /** \endcond           */
    /***********************/
};


#endif //WRENCH_SIM4U_DAEMONWITHMAILBOXACTOR_H
