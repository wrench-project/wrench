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
#include "simgrid_S4U_util/S4U_Mailbox.h"

namespace wrench {

    /**
     * @brief Constructor (daemon with a mailbox)
     *
     * @param process_name: the name of the simulated process/actor
     * @param mailbox_prefix: the prefix of the mailbox (to which a unique integer is appended)
     */
    S4U_Daemon::S4U_Daemon(std::string process_name, std::string mailbox_prefix)
            : process_name(process_name),
              mailbox_name(S4U_Mailbox::generateUniqueMailboxName(mailbox_prefix)) {
      this->terminated = false;
    }

    /**
     * @brief Constructor (daemon without a mailbox)
     *
     * @param process_name: the name of the simulated process/actor
     */
    S4U_Daemon::S4U_Daemon(std::string process_name)
            : process_name(process_name),
              mailbox_name("") {
      this->terminated = false;
    }


    /**
     * @brief Start the daemon
     *
     * @param hostname: the name of the host on which to start the daemon
     */
    void S4U_Daemon::start(std::string hostname, bool daemonized) {

      // Check that the host exists, and if not throw an exceptions
      if (simgrid::s4u::Host::by_name_or_null(hostname) == nullptr) {
        throw std::invalid_argument("S4U_DaemonWithMailbox::start(): Unknown host name '" + hostname + "'");
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
        if(daemonized)
        this->s4u_actor->daemonize();

      // Set the mailbox receiver
      simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(this->mailbox_name);
      mailbox->setReceiver(this->s4u_actor);

      this->hostname = hostname;
    }

    /**
     * @brief Kill the daemon/actor.
     */
    void S4U_Daemon::kill_actor() {
      if (not this->terminated) {
        this->s4u_actor->kill();
        this->terminated = true;
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
