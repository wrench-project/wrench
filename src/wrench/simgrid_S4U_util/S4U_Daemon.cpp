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

#include <xbt/ex.hpp>
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
      this->terminated = false;
    }

//     NOT NEEDED?
//    /**
//     * @brief Constructor (daemon without a mailbox)
//     *
//     * @param hostname: the name of the host on which the daemon will run
//     * @param process_name_prefix: the prefix of the name of the simulated process/actor
//     */
//    S4U_Daemon::S4U_Daemon(std::string hostname, std::string process_name_prefix) {
//      if (simgrid::s4u::Host::by_name_or_null(hostname) == nullptr) {
//        throw std::invalid_argument("S4U_Daemon::S4U_Daemon(): Unknown host '" + hostname + "'");
//      }
//
//      this->hostname = hostname;
//      this->process_name = process_name_prefix + "_" + std::to_string(S4U_Mailbox::generateUniqueSequenceNumber());
//      this->mailbox_name = "";
//      this->terminated = false;
//    }

    S4U_Daemon::~S4U_Daemon() {
      #ifdef ACTOR_TRACKING_OUTPUT
      num_actors[this->process_name_prefix]--;
      #endif
//      std::cerr << "### DESTRUCTOR OF DAEMON " << this->getName() << "\n";
    }

    /**
     * @brief Cleanup function called when the daemon terminates (for whatever reason)
     */
    void S4U_Daemon::cleanup() {
//      WRENCH_INFO("Cleaning Up (default: nothing to do)");
    }

    /**
     * \cond
     */
    static int daemon_goodbye(int x, void *service_instance) {
      WRENCH_INFO("Terminating");
      if (service_instance) {
        auto service = reinterpret_cast<S4U_Daemon *>(service_instance);
        service->cleanup();
        delete service->life_saver;
        service->life_saver = nullptr;
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
     */
    void S4U_Daemon::startDaemon(bool daemonized) {

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

      // Create the s4u_actor
      try {
        this->s4u_actor = simgrid::s4u::Actor::create(this->process_name.c_str(),
                                                      simgrid::s4u::Host::by_name(hostname),
                                                      S4U_DaemonActor(this));
      } catch (std::exception &e) {
        // Some internal SimGrid exceptions...
        std::abort();
      }

      // This test here is critical. It's possible that the created actor above returns
      // right away, in which case calling daemonize() on it cases the calling actor to
      // terminate immediately. This is a weird simgrid::s4u behavior/bug, that may be
      // fixed at some point, but this test saves us for now.
      if (not this->terminated) {
        if (daemonized) {
          this->s4u_actor->daemonize();
        }
        this->s4u_actor->on_exit(daemon_goodbye, (void *) (this));

        // Set the mailbox_name receiver
        simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::by_name(this->mailbox_name);
        mailbox->set_receiver(this->s4u_actor);
      }
    }

    /**
     * @brief Kill the daemon/actor.
     */
    void S4U_Daemon::killActor() {
      if ((this->s4u_actor != nullptr) && (not this->terminated)) {
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
        this->terminated = true;
      }
    }

    /**
    * @brief Suspend the daemon/actor.
    */
    void S4U_Daemon::suspend() {
      if ((this->s4u_actor != nullptr) && (not this->terminated)) {
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
      if ((this->s4u_actor != nullptr) && (not this->terminated)) {
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
     */
    void S4U_Daemon::join() {
      if ((this->s4u_actor != nullptr) && (not this->terminated)) {
        try {
          this->s4u_actor->join();
        } catch (xbt_ex &e) {
          throw std::shared_ptr<FatalFailure>(new FatalFailure());
        } catch (std::exception &e) {
          throw std::shared_ptr<FatalFailure>(new FatalFailure());
        }
      }
    }

    /**
     * @brief Set the terminated status of the daemon/actor
     */
    void S4U_Daemon::setTerminated() {
      this->terminated = true;
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
