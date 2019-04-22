
/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include "S4U_DaemonActor.h"
#include <xbt/log.h>

            XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_daemon_actor, "Log category for S4U_DaemonActor");

namespace wrench {

    void S4U_DaemonActor::setupOnExitFunction() {

        simgrid::s4u::this_actor::on_exit([this](bool failed) {
            // Set state to down
            this->daemon->state = S4U_Daemon::State::DOWN;
            // Call cleanup
            this->daemon->cleanup(daemon->hasReturnedFromMain(), this->daemon->getReturnValue());
            // Free memory for the object unless the service is set to auto-restart
            if (not this->daemon->isSetToAutoRestart()) {
                auto life_saver = this->daemon->life_saver;
                this->daemon->life_saver = nullptr;
                delete life_saver;
            }
            return 0;
        });
    }

};
