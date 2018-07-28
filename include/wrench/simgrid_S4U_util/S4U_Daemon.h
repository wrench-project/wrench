/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SIM4U_DAEMON_H
#define WRENCH_SIM4U_DAEMON_H

#include <string>

#include <simgrid/s4u.hpp>
#include <iostream>

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    class Simulation;

    /**
     * @brief A generic "running daemon" abstraction that serves as a basis for all simulated processes
     */
    class S4U_Daemon {

        class LifeSaver {
        public:
            explicit LifeSaver(std::shared_ptr<S4U_Daemon> &reference) : reference(reference) {}

            std::shared_ptr<S4U_Daemon> reference;
        };

    public:
        /** @brief The name of the daemon */
        std::string process_name;
        /** @brief The name of the daemon's mailbox */
        std::string mailbox_name;
        /** @brief The name of the host on which the daemon is running */
        std::string hostname;

        S4U_Daemon(std::string hostname, std::string process_name_prefix, std::string mailbox_prefix);

        // Daemon without a mailbox (not needed?)
//        S4U_Daemon(std::string hostname, std::string process_name_prefix);

        virtual ~S4U_Daemon();

        void startDaemon(bool daemonized);

        void createLifeSaver(std::shared_ptr<S4U_Daemon> reference);

        virtual void cleanup();

        /**
         * @brief The daemon's main method, to be overridden
         * @return 0 on success
         */
        virtual int main() = 0;

        void setTerminated();

        void join();
        void suspend();
        void resume();

        std::string getName();

        /** @brief The daemon's life saved */
        LifeSaver *life_saver = nullptr;

        /** @brief a pointer to the simulation object */
        Simulation *simulation;
    protected:

        void killActor();
        void acquireDaemonLock();
        void releaseDaemonLock();

    private:
        // Lock use typically to prevent kill() from killing the actor
        // while it's in the middle of doing something critical
        simgrid::s4u::MutexPtr daemon_lock;
        bool terminated;
        simgrid::s4u::ActorPtr s4u_actor;

    };

    /***********************/
    /** \endcond           */
    /***********************/
};


#endif //WRENCH_SIM4U_DAEMONWITHMAILBOX_H
