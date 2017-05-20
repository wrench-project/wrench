/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include "S4U_DaemonWithMailbox.h"
#include "S4U_DaemonWithMailboxActor.h"
#include "S4U_Mailbox.h"

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param process_name: the name of the simulated process/actor
     * @param mailbox_prefix: the prefix of the mailbox (to which a unique integer is appended)
     */
    S4U_DaemonWithMailbox::S4U_DaemonWithMailbox(std::string process_name, std::string mailbox_prefix) {
      this->process_name = process_name;
      this->mailbox_name = S4U_Mailbox::generateUniqueMailboxName(mailbox_prefix);
    }

    /**
     * @brief Start the daemon
     * @param hostname: the name of the host on which to start the daemon
     */
    void S4U_DaemonWithMailbox::start(std::string hostname) {

      // Check that the host exists, and if not throw an exceptions
      if (simgrid::s4u::Host::by_name_or_null(hostname) == nullptr) {
        throw std::invalid_argument("Unknown host name '" + hostname + "'");
      }

      // Create the s4u_actor
      try {
        this->s4u_actor = simgrid::s4u::Actor::createActor(this->process_name.c_str(),
                                                           simgrid::s4u::Host::by_name(hostname),
                                                           S4U_DaemonWithMailboxActor(this));
      } catch (std::exception e) {
        // Some internal SimGrid exceptions...
        std::abort();
      }

      // Set the mailbox receiver
      simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(this->mailbox_name);
      mailbox->setReceiver(this->s4u_actor);

      this->hostname = hostname;

    }

    /**
     * @brief Kill the daemon/actor.
     */
    void S4U_DaemonWithMailbox::kill_actor() {
      if (!(this->terminated)) {
        this->s4u_actor->kill();
        this->terminated = true;
      } else {
      }
    }

    /**
     * @brief Set the terminated status of the daemon/actor
     */
    void S4U_DaemonWithMailbox::setTerminated() {
      this->terminated = true;
    }


    /**
     * @brief Retrieve the process name
     * @return the name
     */
    std::string S4U_DaemonWithMailbox::getName() {
      return this->process_name;
    }

};