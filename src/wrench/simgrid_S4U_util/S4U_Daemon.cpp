/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include "wrench/simgrid_S4U_util/S4U_Daemon.h"
#include "simgrid_S4U_util/S4U_DaemonActor.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include <wrench/logging/TerminalOutput.h>
#include <boost/algorithm/string.hpp>
#include <wrench/workflow/failure_causes/HostError.h>

#ifdef MESSAGE_MANAGER
#include <wrench/util/MessageManager.h>
#endif


WRENCH_LOG_CATEGORY(wrench_core_s4u_daemon, "Log category for S4U_Daemon");

#ifdef ACTOR_TRACKING_OUTPUT
std::map<std::string, unsigned long> num_actors;
#endif

namespace wrench {

    /**
     * @brief Constructor (daemon with a mailbox)
     *
     * @param hostname: the name of the host on which the daemon will run
     * @param process_name_prefix: the prefix of the name of the simulated process/actor
     * @param mailbox_prefix: the prefix of the mailbox (to which a unique integer is appended)
     */
    S4U_Daemon::S4U_Daemon(std::string hostname, std::string process_name_prefix, std::string mailbox_prefix) {

        if (not simgrid::s4u::Engine::is_initialized()) {
            throw std::runtime_error("Simulation must be initialized before services can be created");
        }

        if (simgrid::s4u::Host::by_name_or_null(hostname) == nullptr) {
            throw std::invalid_argument("S4U_Daemon::S4U_Daemon(): Unknown host '" + hostname + "'");
        }

#ifdef ACTOR_TRACKING_OUTPUT
        this->process_name_prefix = "";
      std::vector<std::string> tokens;
      boost::split(tokens, process_name_prefix, boost::is_any_of("_"));
      for (auto t : tokens) {
        if (t.at(0) < '0' or t.at(0) > '9') {
          this->process_name_prefix += t + "_";
        }
      }

      if (num_actors.find(this->process_name_prefix) == num_actors.end()) {
        num_actors.insert(std::make_pair(this->process_name_prefix, 1));
      } else {
        num_actors[this->process_name_prefix]++;
      }

      for (auto a : num_actors) {
        std::cerr << a.first << ":" << a.second << "\n";
      }
      std::cerr << "---------------\n";
#endif


        this->daemon_lock = simgrid::s4u::Mutex::create();
        this->hostname = hostname;
        this->simulation = nullptr;
        unsigned long seq = S4U_Mailbox::generateUniqueSequenceNumber();
        this->initial_mailbox_name = mailbox_prefix + "_" + std::to_string(seq);
        this->mailbox_name = this->initial_mailbox_name + "_#" + std::to_string(this->num_starts);
        this->process_name = process_name_prefix + "_" + std::to_string(seq);
        this->has_returned_from_main = false;
    }

#define CLEAN_UP_MAILBOX_TO_AVOID_MEMORY_LEAK 0

    S4U_Daemon::~S4U_Daemon() {

//        WRENCH_INFO("IN DAEMON DESTRUCTOR (%s)'", this->getName().c_str());

        /** The code below was to avoid a memory leak on the actor! However, weirdly,
         *  it now causes problems due to SimGrid complaining that on_exit() functions
         *  shouldn't do blocking things.... So it's commented-out for now
         */
#if (CLEAN_UP_MAILBOX_TO_AVOID_MEMORY_LEAK == 1)
         auto mailbox = simgrid::s4u::Mailbox::by_name(this->mailbox_name);
         mailbox->set_receiver(nullptr);
#endif

#ifdef ACTOR_TRACKING_OUTPUT
        num_actors[this->process_name_prefix]--;
#endif

    }

    /**
     * @brief Cleanup function called when the daemon terminates (for whatever reason). The
     *        default behavior is to throw an exception if the host is off. This method
     *        should be overriden in a daemons implements some fault-tolerant behavior, or
     *        is naturally tolerant.
     *
     * @param has_returned_from_main: whether the daemon returned from main() by itself
     * @param return_value: the return value from main (if has_terminated_cleanly is true)
     *
     * @throw std::runtime_error
     */
    void S4U_Daemon::cleanup(bool has_returned_from_main, int return_value) {
        // Default behavior is to throw in case of any problem
        if ((not has_returned_from_main) and (not S4U_Simulation::isHostOn(hostname))) {
            throw std::runtime_error(
                    "S4U_Daemon::cleanup(): This daemon has died due to a failure of its host, but does not override cleanup() "
                    "(so that is can implement fault-tolerance or explicitly ignore fault) ");
        }
    }


    /**
     * @brief Get the daemon's state
     * @return a state
     */
    S4U_Daemon::State S4U_Daemon::getState() {
        return this->state;
    }

    /**
     * @brief Start the daemon
     *
     * @param daemonized: whether the S4U actor should be daemonized
     * @param auto_restart: whether the S4U actor should automatically restart after a host reboot
     *
     * @throw std::runtime_error
     * @throw std::shared_ptr<HostError>
     */
    void S4U_Daemon::startDaemon(bool daemonized, bool auto_restart) {

        // Check that there is a lifesaver
        if (not this->life_saver) {
            throw std::runtime_error(
                    "S4U_Daemon::startDaemon(): You must call createLifeSaver() before calling startDaemon()");
        }

        // Check that the simulation pointer is set
        if (not this->simulation) {
            throw std::runtime_error(
                    "S4U_Daemon::startDaemon(): You must set the simulation field before calling startDaemon() (" +
                    this->getName() + ")");
        }

        // Check that the host is up!
        if (not S4U_Simulation::isHostOn(hostname)) {
            throw std::shared_ptr<HostError>(new HostError(hostname));
        }

        this->daemonized = daemonized;
        this->auto_restart = auto_restart;
        this->has_returned_from_main = false;
        this->mailbox_name = this->initial_mailbox_name + "_#" + std::to_string(this->num_starts);
        // Create the s4u_actor
        try {
            this->s4u_actor = simgrid::s4u::Actor::create(this->process_name.c_str(),
                                                          simgrid::s4u::Host::by_name(hostname),
                                                          S4U_DaemonActor(this));
        } catch (simgrid::Exception &e) {
            throw std::runtime_error("S4U_Daemon::startDaemon(): SimGrid actor creation failed... shouldn't happen.");
        }

        // nullptr is returned if the host is off (not the current behavior in SimGrid... just paranoid here)
        if (this->s4u_actor == nullptr) {
            throw std::shared_ptr<HostError>(new HostError(hostname));
        }

        // This test here is critical. It's possible that the created actor above returns
        // right away, in which case calling daemonize() on it cases the calling actor to
        // terminate immediately. This is a weird simgrid::s4u behavior/bug, that may be
        // fixed at some point, but this test saves us for now.
        if (not this->has_returned_from_main) {

            this->setupOnExitFunction();

            if (this->daemonized) {
                this->s4u_actor->daemonize();
            }

            if (this->auto_restart) {
                this->s4u_actor->set_auto_restart(true);
            }

        }

        // Set the mailbox_name receiver (causes memory leak)
        // Causes Mailbox::put() to no longer implement a rendez-vous communication.
        simgrid::s4u::Mailbox::by_name(this->mailbox_name)->set_receiver(this->s4u_actor);


    }

    /**
     * @brief Sets up the on_exit functionf for the actor
     */
    void S4U_Daemon::setupOnExitFunction() {

        this->s4u_actor->on_exit([this](bool failed) {
            // Set state to down
            this->state = S4U_Daemon::State::DOWN;
            // Call cleanup
            this->cleanup(this->hasReturnedFromMain(), this->getReturnValue());
            // Free memory for the object unless the service is set to auto-restart
            if (not this->isSetToAutoRestart()) {
                auto life_saver = this->life_saver;
                this->life_saver = nullptr;
                Service::increaseNumCompletedServicesCount();
#ifdef MESSAGE_MANAGER
                MessageManager::cleanUpMessages(this->mailbox_name);
#endif
                delete life_saver;
            }
            return 0;
        });
    }


/**
 * @brief Return the auto-restart status of the daemon
 * @return true or false
 */
    bool S4U_Daemon::isSetToAutoRestart() {
        return this->auto_restart;
    }

    /**
 * @brief Return the daemonized status of the daemon
 * @return true or false
 */
    bool S4U_Daemon::isDaemonized() {
        return this->daemonized;
    }

/**
 * @brief Method that run's the user-defined main method (that's called by the S4U actor class)
 */
    void S4U_Daemon::runMainMethod() {
        this->num_starts++;
        // Compute zero flop so that nothing happens
        // until the host has a >0 pstate
        S4U_Simulation::computeZeroFlop();
        this->state = State::UP;
        this->return_value = this->main();
        this->has_returned_from_main = true;
        this->state = State::DOWN;
    }


/**
 * @brief Kill the daemon/actor (does nothing if already dead)
 *
 * @throw std::shared_ptr<FatalFailure>
 */
    void S4U_Daemon::killActor() {

        // Do the kill only if valid actor and not already done
        if ((this->s4u_actor != nullptr) && (not this->has_returned_from_main)) {
            try {
                this->s4u_actor->kill();
            } catch (simgrid::Exception &) {
                throw std::runtime_error("simgrid::s4u::Actor::kill() failed... this shouldn't have happened");
            }
            this->has_returned_from_main = true;
        }
    }

/**
 * @brief Suspend the daemon/actor.
 */
    void S4U_Daemon::suspendActor() {
        if ((this->s4u_actor != nullptr) && (not this->has_returned_from_main)) {
            this->s4u_actor->suspend();
        }
    }

/**
 * @brief Resume the daemon/actor.
 */
    void S4U_Daemon::resumeActor() {
        if ((this->s4u_actor != nullptr) && (not this->has_returned_from_main)) {
            this->s4u_actor->resume();
        }
    }

/**
 * @brief Join (i.e., wait for) the daemon.
 *
 * @return a pair <A,B> where A is boolean (true if the daemon terminated cleanly (i.e., main() returned), or false otherwise)
 *          and B is the int returned from main() (if main returned).
 */
    std::pair<bool, int> S4U_Daemon::join() {

        if (this->s4u_actor != nullptr) {
            this->s4u_actor->join();
        } else {
            throw std::runtime_error("S4U_Daemon::join(): Fatal error: this->s4u_actor shouldn't be nullptr");
        }
//        WRENCH_INFO("JOIN ON '%s' HAS RETURNED: %d %d", this->getName().c_str(), this->hasReturnedFromMain(), this->getReturnValue());
        return std::make_pair(this->hasReturnedFromMain(), this->getReturnValue());
    }

/**
 * @brief Returns true if the daemon has returned from main() (i.e., not brutally killed)
 * @return The true or false
 */
    bool S4U_Daemon::hasReturnedFromMain() {
        return this->has_returned_from_main;
    }

/**
 * @brief Returns the value returned by main() (if the daemon has returned from main)
 * @return The return value
 */
    int S4U_Daemon::getReturnValue() {
        return this->return_value;
    }

/**
 * @brief Retrieve the process name
 * @return the name
 */
    std::string S4U_Daemon::getName() {
        return this->process_name;
    }

/**
 * @brief Create a life saver for the daemon
 * @param reference
 */
    void S4U_Daemon::createLifeSaver(std::shared_ptr<S4U_Daemon> reference) {
        if (this->life_saver) {
            throw std::runtime_error("S4U_Daemon::createLifeSaver(): Lifesaver already created!");
        }
        this->life_saver = new S4U_Daemon::LifeSaver(reference);
    }

/**
 * @brief Lock the daemon's lock
 */
    void S4U_Daemon::acquireDaemonLock() {
        this->daemon_lock->lock();
    }

/**
 * @brief Unlock the daemon's lock
 */
    void S4U_Daemon::releaseDaemonLock() {
        this->daemon_lock->unlock();
    }
};
