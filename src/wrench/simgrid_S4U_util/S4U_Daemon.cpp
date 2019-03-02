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

//#include <xbt/ex.hpp>
#include <wrench/util/MessageManager.h>


#include <boost/algorithm/string.hpp>


XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_daemon, "Log category for S4U_Daemon");


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
        this->mailbox_name = mailbox_prefix + "_" + std::to_string(seq);
        this->process_name = process_name_prefix + "_" + std::to_string(seq);
        this->has_returned_from_main = false;
    }


#define CLEAN_UP_MAILBOX_TO_AVOID_MEMORY_LEAK 0

    S4U_Daemon::~S4U_Daemon() {
        WRENCH_DEBUG("IN DAEMON DESTRUCTOR (%s)'", this->getName().c_str());
        /** The code below was to avoid a memory leak on the actor! However, weirdly,
         *  it now causes problems due to SimGrid complaining that on_exit() functions
         *  shouldn't do blocking things.... So it's commented-out for now
         */
#if (CLEAN_UP_MAILBOX_TO_AVOID_MEMORY_LEAK == 1)
        simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::by_name(this->mailbox_name);
         mailbox->set_receiver(nullptr);
#endif

#ifdef ACTOR_TRACKING_OUTPUT
        num_actors[this->process_name_prefix]--;
#endif
    }

    /**
     * @brief Cleanup function called when the daemon terminates (for whatever reason)
     *
     * @param has_terminated_cleanly: whether the daemon returned from main() by itself
     * @param exit_code: the return value from main (if has_terminated_cleanly is true)
     */
    void S4U_Daemon::cleanup(bool has_terminated_cleanly, int return_value) {
//      WRENCH_INFO("Cleaning Up (default: nothing to do)");
    }

    /**
     * \cond
     */
    static int daemon_goodbye(int x, void *service_instance) {
        WRENCH_INFO("Terminating");
        if (service_instance == nullptr) {
            return 0;
        }
        auto service = reinterpret_cast<S4U_Daemon *>(service_instance);

        // Call cleanup
        service->cleanup(service->hasReturnedFromMain(), service->getReturnValue());

        // Free memory for the object unless the service is set to auto-restart
        if (not service->isSetToAutoRestart()) {
            delete service->life_saver;
        }
        return 0;
    }

    /**
     * \endcond
     */

    /**
     * @brief Start the daemon
     *
     * @param daemonized: whether the S4U actor should be daemonized
     * @param auto_restart: whether the S4U actor should automatically restart after a host reboot
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
        if (not simgrid::s4u::Host::by_name(hostname)->is_on()) {
            throw std::shared_ptr<HostError>(new HostError(hostname));
        }

        // Create the s4u_actor
        try {
            this->s4u_actor = simgrid::s4u::Actor::create(this->process_name.c_str(),
                                                          simgrid::s4u::Host::by_name(hostname),
                                                          S4U_DaemonActor(this));
        } catch (std::exception &e) {
            throw std::shared_ptr<FatalFailure>(new FatalFailure());
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
            if (daemonized) {
                this->s4u_actor->daemonize();
            }
            this->auto_restart = auto_restart;
            if (this->auto_restart) {
                this->s4u_actor->set_auto_restart(true);
            }
            this->s4u_actor->on_exit(daemon_goodbye, (void *) (this));
        }

        // Set the mailbox_name receiver (causes memory leak)
        simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::by_name(this->mailbox_name);
        mailbox->set_receiver(this->s4u_actor);
    }


/**
 * @brief Return the auto-restart status of the daemon
 * @return true or false
 */
    bool S4U_Daemon::isSetToAutoRestart() {
        return this->auto_restart;
    }

/**
 * @brief Method that run's the user-defined main method (that's called by the S4U actor class)
 */
    void S4U_Daemon::runMainMethod() {
        try {
            this->num_starts++;
            try {
                S4U_Simulation::computeZeroFlop();
                this->return_value = this->main();
                this->has_returned_from_main = true;
            } catch (std::shared_ptr<HostError> &e) {
                // In case the main() didn't to that catch
            } catch (simgrid::HostFailureException &e) {
                // In case the main() didn't to that catch
            }
//            WRENCH_INFO("SLEEPING FOR 0.0001 seconds (WHY?)");
            wrench::S4U_Simulation::sleep(0.001);
        } catch (std::exception &e) {
            throw;
        }

    }


/**
 * @brief Kill the daemon/actor.
 */
    void S4U_Daemon::killActor() {
        if ((this->s4u_actor != nullptr) && (not this->has_returned_from_main)) {
            try {
                // Sleeping a tiny bit to avoid the following behavior:
                // Actor A creates Actor B.
                // Actor C kills actor A at the same time
                // At that point, all references to Actor B are lost
                // (Actor A could have set a reference to B, and that reference
                // would be available on A's object, which then C can look at to
                // say "since I killed A, I should kill at its children as well"
//          S4U_Simulation::sleep(0.0001);
                this->s4u_actor->kill();

            } catch (xbt_ex &e) {
                throw std::shared_ptr<FatalFailure>(new FatalFailure());
            } catch (std::exception &e) {
                throw std::shared_ptr<FatalFailure>(new FatalFailure());
            }
            this->has_returned_from_main = true;
        }
    }

/**
* @brief Suspend the daemon/actor.
*/
    void S4U_Daemon::suspend() {
        if ((this->s4u_actor != nullptr) && (not this->has_returned_from_main)) {
            try {
                this->s4u_actor->suspend();
            } catch (xbt_ex &e) {
                throw std::shared_ptr<FatalFailure>(new FatalFailure());
            } catch (std::exception &e) {
                throw std::shared_ptr<FatalFailure>(new FatalFailure());
            }
        }
    }

/**
* @brief Resume the daemon/actor.
*/
    void S4U_Daemon::resume() {
        if ((this->s4u_actor != nullptr) && (not this->has_returned_from_main)) {
            try {
                this->s4u_actor->resume();
            } catch (xbt_ex &e) {
                throw std::shared_ptr<FatalFailure>(new FatalFailure());
            } catch (std::exception &e) {
                throw std::shared_ptr<FatalFailure>(new FatalFailure());
            }
        }
    }

/**
 * @brief Join (i.e., wait for) the daemon.
 *
 * @return true if the daemon terminated cleanly (i.e., main() returned), or false otherwise
 */
    std::pair<bool, int> S4U_Daemon::join() {
        if (this->hasReturnedFromMain()) {
            return std::make_pair(this->hasReturnedFromMain(), this->getReturnValue());
        }

        if (this->s4u_actor != nullptr) {
            try {
                this->s4u_actor->join();
            } catch (xbt_ex &e) {
                throw std::shared_ptr<FatalFailure>(new FatalFailure());
            } catch (std::exception &e) {
                throw std::shared_ptr<FatalFailure>(new FatalFailure());
            }
        }
        WRENCH_INFO("JOIN ON '%s' HAS RETURNED: %d %d",
                    this->getName().c_str(), this->hasReturnedFromMain(), this->getReturnValue());
        return std::make_pair(this->hasReturnedFromMain(), this->getReturnValue());
    }

/**
 * @brief Returns true if the daemon has returned from main() (i.e., not brutally killed)
 */
    bool S4U_Daemon::hasReturnedFromMain() {
        return this->has_returned_from_main;
    }

/**
 * @brief Returns the value returned by main() (if the daemon has returned from main)
 */
    int S4U_Daemon::getReturnValue() {
        return this->return_value;
    }

/**
 * @brief Retrieve the process name
 *
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
