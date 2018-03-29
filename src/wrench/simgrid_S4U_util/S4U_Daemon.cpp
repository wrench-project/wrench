/**
 * Copyright (c) 2017. The WRENCH Team.
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


XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_daemon, "Log category for S4U_Daemon");


namespace wrench {

    /**
     * @brief Constructor (daemon with a mailbox)
     *
     * @param hostname: the name of the host on which the daemon will run
     * @param process_name_prefix: the prefix of the name of the simulated process/actor
     * @param mailbox_prefix: the prefix of the mailbox (to which a unique integer is appended)
     */
    S4U_Daemon::S4U_Daemon(std::string hostname, std::string process_name_prefix, std::string mailbox_prefix) {

      if (simgrid::s4u::Host::by_name_or_null(hostname) == nullptr) {
        throw std::invalid_argument("S4U_Daemon::S4U_Daemon(): Unknown host '" + hostname + "'");
      }

      this->hostname = hostname;
      this->simulation = nullptr;
      unsigned long seq = S4U_Mailbox::generateUniqueSequenceNumber();
      this->mailbox_name = mailbox_prefix + "_" + std::to_string(seq);
      this->process_name = process_name_prefix + "_" + std::to_string(seq);
      this->terminated = false;
    }

    /**
     * @brief Constructor (daemon without a mailbox)
     *
     * @param hostname: the name of the host on which the daemon will run
     * @param process_name_prefix: the prefix of the name of the simulated process/actor
     */
    S4U_Daemon::S4U_Daemon(std::string hostname, std::string process_name_prefix) {
      if (simgrid::s4u::Host::by_name_or_null(hostname) == nullptr) {
        throw std::invalid_argument("S4U_Daemon::S4U_Daemon(): Unknown host '" + hostname + "'");
      }

      this->hostname = hostname;
      this->process_name = process_name_prefix + "_" + std::to_string(S4U_Mailbox::generateUniqueSequenceNumber());
      this->mailbox_name="";
      this->terminated = false;
    }

    S4U_Daemon::~S4U_Daemon() {
//      std::cerr << "### DESTRUCTOR OF DAEMON " << this->getName() << "\n";
    }

    /**
     * @brief Cleanup function called when the daemon terminates (for whatever reason)
     */
    void S4U_Daemon::cleanup(){
//      WRENCH_INFO("Cleaning Up (default: nothing to do)");
    }


    /**
     * \cond
     */
    static int daemon_goodbye(void *x, void* service_instance) {
      WRENCH_INFO("Terminating");
      if (service_instance) {
        auto service = reinterpret_cast<S4U_Daemon *>(service_instance);
        service->cleanup();
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
     * @param daemonized: whether the S4U actor should be daemonized (untstart_ested)
     */
    void S4U_Daemon::startDaemon(bool daemonized) {

      // Check that there is a lifesaver
      if (not this->life_saver) {
        throw std::runtime_error("S4U_Daemon::startDaemon(): You must call createLifeSaver() before calling startDaemon()");
      }

      // Check that the simulation pointer is set
      if (not this->simulation) {
        throw std::runtime_error("S4U_Daemon::startDaemon(): You must set the simulation field before calling startDaemon() (" + this->getName() + ")");
      }

      // Create the s4u_actor
      try {
        this->s4u_actor = simgrid::s4u::Actor::createActor(this->process_name.c_str(),
                                                           simgrid::s4u::Host::by_name(hostname),
                                                           S4U_DaemonActor(this));
      } catch (std::exception &e) {
        // Some internal SimGrid exceptions...
        std::abort();
      }

      if (daemonized) {
        this->s4u_actor->daemonize();
      }
      this->s4u_actor->onExit(daemon_goodbye, (void *) (this));

      // Set the mailbox_name receiver
      simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(this->mailbox_name);
      mailbox->setReceiver(this->s4u_actor);
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
          S4U_Simulation::sleep(0.0001);
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
     * @brief Kill the daemon/actor.
     */
    void S4U_Daemon::joinActor() {
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
     * @brief creates a life saver for the daemon
     * @param reference
     */
    void S4U_Daemon::createLifeSaver(std::shared_ptr<S4U_Daemon> reference) {
      if (this->life_saver) {
        throw std::runtime_error("S4U_Daemon::createLifeSaver(): Lifesaver already created!");
      }
      this->life_saver = new S4U_Daemon::LifeSaver(reference);
    }

};
