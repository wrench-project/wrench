/**
 * Copyright (c) 2017-2018. The WRENCH Team.
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

//#define ACTOR_TRACKING_OUTPUT yes


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
        /** @brief The initial name of the daemon's mailbox */
        std::string initial_mailbox_name;
        /** @brief The current name of the daemon's mailbox */
        std::string mailbox_name;
        /** @brief The name of the host on which the daemon is running */
        std::string hostname;



        S4U_Daemon(std::string hostname, std::string process_name_prefix, std::string mailbox_prefix);

        // Daemon without a mailbox (not needed?)
//        S4U_Daemon(std::string hostname, std::string process_name_prefix);

        virtual ~S4U_Daemon();

        void startDaemon(bool _daemonized, bool _auto_restart);

        void createLifeSaver(std::shared_ptr<S4U_Daemon> reference);

        virtual void cleanup(bool has_returned_from_main, int return_value);

        /**
         * @brief The daemon's main method, to be overridden
         * @return 0 on success, non-0 on failure!
         */
        virtual int main() = 0;

        bool hasReturnedFromMain();
        int getReturnValue();
        bool isDaemonized();
        bool isSetToAutoRestart();
        void setupOnExitFunction();

        std::pair<bool, int> join();

        void suspendActor();

        void resumeActor();


        std::string getName();

        /** @brief Daemon states */
        enum State {
            /** @brief UP state: the daemon has been started and is still running */
                    UP,
            /** @brief DOWN state: the daemon has been shutdown and/or has terminated */
                    DOWN,
            /** @brief SUSPENDED state: the daemon has been suspended (and hopefully will be resumed0 */
                    SUSPENDED,
        };

        S4U_Daemon::State getState();

        /** @brief The daemon's life saver */
        LifeSaver *life_saver = nullptr;

        /** @brief a pointer to the simulation object */
        Simulation *simulation;

        /** @brief Method to acquire the daemon's lock */
        void acquireDaemonLock();

        /** @brief Method to release the daemon's lock */
        void releaseDaemonLock();

    protected:

        /** @brief The service's state */
        State state;

        friend class S4U_DaemonActor;
        void runMainMethod();

        bool killActor();



        /** @brief The number of time that this daemon has started (i.e., 1 + number of restarts) */
        unsigned int num_starts = 0;

    private:


        // Lock used typically to prevent kill() from killing the actor
        // while it's in the middle of doing something critical
        simgrid::s4u::MutexPtr daemon_lock;

        simgrid::s4u::ActorPtr s4u_actor;

        bool has_returned_from_main = false; // Set to true after main returns
        int return_value = 0; // Set to the value returned by main
        bool daemonized; // Set to true if daemon is daemonized
        bool auto_restart; // Set to true if daemon is supposed to auto-restart


#ifdef ACTOR_TRACKING_OUTPUT
        std::string process_name_prefix;
#endif

    };

    /***********************/
    /** \endcond           */
    /***********************/
};


#endif //WRENCH_SIM4U_DAEMONWITHMAILBOX_H
