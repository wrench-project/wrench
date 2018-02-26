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
     * @param process_name_prefix: the prefix of the name of the simulated process/actor
     * @param mailbox_prefix: the prefix of the mailbox (to which a unique integer is appended)
     */
    S4U_Daemon::S4U_Daemon(std::string process_name_prefix, std::string mailbox_prefix) {
      unsigned long seq = S4U_Mailbox::generateUniqueSequenceNumber();
      this->mailbox_name = mailbox_prefix + "_" + std::to_string(seq);
      this->process_name = process_name_prefix + "_" + std::to_string(seq);
      this->terminated = false;
    }

    /**
     * @brief Constructor (daemon without a mailbox)
     *
     * @param process_name: the prefix of the name of the simulated process/actor
     */
    S4U_Daemon::S4U_Daemon(std::string process_name_prefix) {
      this->process_name = process_name_prefix + "_" + std::to_string(S4U_Mailbox::generateUniqueSequenceNumber());
      this->mailbox_name="";
      this->terminated = false;
    }

    S4U_Daemon::~S4U_Daemon() {
//        WRENCH_INFO("In the Daemon Destructor");
    }


    /**
     * \cond
     */
    static int daemon_goodbye(void *x, void *y) {
      WRENCH_INFO("Terminating");
      return 0;
    }

    /**
     * \endcond
     */

    /**
     * @brief Start the daemon
     *
     * @param hostname: the name of the host on which to start the daemon
     * @param daemonized: whether the S4U actor should be daemonized (untstart_ested)
     */
    void S4U_Daemon::start_daemon(std::string hostname, bool daemonized) {

      // Check that the host exists, and if not throw an exceptions
      if (simgrid::s4u::Host::by_name_or_null(hostname) == nullptr) {
        WRENCH_INFO("THROWING IN S$UDAEMON: '%s'", hostname.c_str());
        throw std::invalid_argument("S4U_DaemonWithMailbox::start_daemon(): Unknown host name '" + hostname + "'");
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

      // TODO: This wasn't working right last time Henri checked... but it's likely no big deal
      if (daemonized)
        this->s4u_actor->daemonize();

      this->s4u_actor->onExit(daemon_goodbye, (void *) (this->process_name.c_str()));


      // Set the mailbox receiver
      simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(this->mailbox_name);
      mailbox->setReceiver(this->s4u_actor);

      this->hostname = hostname;
    }

    /**
     * @brief Kill the daemon/actor.
     */
    void S4U_Daemon::kill_actor() {
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
    void S4U_Daemon::join_actor() {
      if ((this->s4u_actor != nullptr) && (not this->terminated)) {
        try {
          this->s4u_actor->join();
        } catch (xbt_ex &e) {
          throw std::shared_ptr<FatalFailure>(new FatalFailure());
        } catch (std::exception &e)
        {
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

};
