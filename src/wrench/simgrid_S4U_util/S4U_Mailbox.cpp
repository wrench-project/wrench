/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <iostream>
#include <xbt/ex.hpp>
#include <simgrid/s4u/Mailbox.hpp>
#include <simgrid/s4u.hpp>

#include "logging/TerminalOutput.h"
#include "simgrid_S4U_util/S4U_Mailbox.h"
#include "simulation/SimulationMessage.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(mailbox, "Mailbox");


namespace wrench {

    class WorkflowTask;

    // A data structure to keep track of pending asynchronous putMessage() operations
    // what will have to be waited on at some point
    std::map<simgrid::s4u::ActorPtr, std::set<simgrid::s4u::CommPtr>> S4U_Mailbox::dputs;




    /**
     * @brief A blocking method to receive a message from a mailbox
     *
     * @param mailbox_name: the mailbox name
     * @return the message, or nullptr (in which case it's likely a brutal termination)
     *
     * @throw std:runtime_error
     *      - "network_error" (e.g., other end has died)
     *
     */
    std::unique_ptr<SimulationMessage> S4U_Mailbox::getMessage(std::string mailbox_name) {
      WRENCH_DEBUG("IN GET from %s", mailbox_name.c_str());
      simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(mailbox_name);
      SimulationMessage *msg = nullptr;
      try {
        msg = static_cast<SimulationMessage *>(simgrid::s4u::this_actor::recv(mailbox));
      } catch (xbt_ex &e) {
        if (e.category == network_error) {
          WRENCH_INFO("Network error while doing a getMessage(). Likely the sender has died");
          throw std::runtime_error("network_error");
        }
      }
      // This is just because it seems that after something like a killAll() we get a nullptr
      if (msg == nullptr) {
        WRENCH_INFO("Network error while doing a getMessage(). Got a nullptr...");
        throw std::runtime_error("network_error");
      }

      WRENCH_DEBUG("GOT a '%s' message from %s", msg->getName().c_str(), mailbox_name.c_str());
      return std::unique_ptr<SimulationMessage>(msg);
    }

    /**
     * @brief A blocking method to receive a message from a mailbox, with a timeout
     *
     * @param mailbox_name: the mailbox name
     * @param timeout:  a timeout value in seconds
     * @return the message, or nullptr (in which case it's likely a brutal termination)
     *
     * @throw std::runtime_error:
     *        - "timeout"
     *        - "network_error" (e.g., other end has died)
     */
    std::unique_ptr<SimulationMessage> S4U_Mailbox::getMessage(std::string mailbox_name, double timeout) {
      WRENCH_DEBUG("IN GET WITH TIMEOUT (%lf) FROM MAILBOX %s", timeout, mailbox_name.c_str());
      simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(mailbox_name);
      void *data = nullptr;
      try {
        data = simgrid::s4u::this_actor::recv(mailbox, timeout);
      } catch (xbt_ex &e) {
        if (e.category == timeout_error) {
          throw std::runtime_error("timeout");
        }
        if (e.category == network_error) {
          WRENCH_INFO("Network error while doing a getMessage() with timeout. Likely the sender has died.");
          throw std::runtime_error("network_error");
        }
      }

//      try {
//        simgrid::s4u::CommPtr comm = simgrid::s4u::this_actor::irecv(mailbox, &data);
//        comm->wait(timeout);
//      } catch (xbt_ex &e) {
//        if (e.category == timeout_error) {
//          throw std::runtime_error("timeout");
//        }
//        if (e.category == network_error) {
//          WRENCH_INFO("Network error while doing a getMessage() with timeout. Likely the sender has died.");
//          throw std::runtime_error("network_error");
//        }
//      }

      // This is just because it seems that after something like a killAll() we get a nullptr
      if (data == nullptr) {
        WRENCH_INFO("Network error while doing a getMessage() with timeout (got a nullptr).");
        throw std::runtime_error("network_error");
      }

      SimulationMessage *msg = static_cast<SimulationMessage *>(data);

      WRENCH_DEBUG("GOT a '%s' message from %s", msg->getName().c_str(), mailbox_name.c_str());

      return std::unique_ptr<SimulationMessage>(msg);
    }

    /**
     * @brief A blocking method to send a message to a mailbox
     *
     * @param mailbox_name: the mailbox name
     * @param msg: the SimulationMessage
     *
     * @throw std::runtime_error:  ("network_error")
     */
    void S4U_Mailbox::putMessage(std::string mailbox_name, SimulationMessage *msg) {
      WRENCH_DEBUG("PUTTING to %s a %s message", mailbox_name.c_str(), msg->getName().c_str());
      simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(mailbox_name);
      try {
      simgrid::s4u::this_actor::send(mailbox, msg, (size_t) msg->payload);
      } catch (xbt_ex &e) {
        if (e.category == network_error) {
          WRENCH_INFO("Network error while doing a putMessage)");
          throw std::runtime_error("network_error");
        }
      }

      return;
    }

    /**
     * @brief A non-blocking method to send a message to a mailbox
     *
     * @param mailbox_name: the mailbox name
     * @param msg: the SimulationMessage
     *
     * @throw std::runtime_error:  ("network_error")
     */
    void S4U_Mailbox::dputMessage(std::string mailbox_name, SimulationMessage *msg) {

      WRENCH_DEBUG("Dputting to %s a %s message", mailbox_name.c_str(), msg->getName().c_str());

      simgrid::s4u::CommPtr comm = nullptr;

      simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(mailbox_name);
      try {
        comm = simgrid::s4u::Comm::send_async(mailbox, msg, (int) msg->payload);
      } catch (xbt_ex &e) {
        if (e.category == network_error) {
          WRENCH_INFO("Network error while doing a dputMessage()");
          throw std::runtime_error("network_error");
        }
      }

      // Insert the communication into the dputs map, so that it's not lost
      // and it can be "cleared" later
      S4U_Mailbox::dputs[simgrid::s4u::Actor::self()].insert(comm);
      return;
    }

    /**
     * @brief A method that checks on and clears previous asynchronous communications. This is
     * to avoid having the above levels deal with asynchronous communication stuff.
     */
    void S4U_Mailbox::clear_dputs() {
      std::set<simgrid::s4u::CommPtr> set = S4U_Mailbox::dputs[simgrid::s4u::Actor::self()];
      std::set<simgrid::s4u::CommPtr>::iterator it;
      for (it = set.begin(); it != set.end(); ++it) {
        // TODO: This is probably not great right now, but S4U asynchronous communication are
        // TODO: in a state of flux, and so this seems to work but for the memory leak
        // TODO: will have to talk to the S4U developers

//        XBT_INFO("Getting the state of a previous communication! (%s)", simgrid::s4u::Actor::self()->name().c_str());
        e_s4u_activity_state_t state = (*it)->getState();
        if (state == finished) {
//          XBT_INFO(
//                  "The communication is finished.... remove it from the pending list [TODO: delete memory??? call test()???]");
          set.erase(*it);
        } else {
//          XBT_INFO("State = %d (finished = %d)", state, finished);
        }
      }
      return;
    }

    /**
    * @brief A method to generate a unique sequence number
    *
    * @return a unique sequence number
    */
    unsigned long S4U_Mailbox::generateUniqueSequenceNumber() {
      static unsigned long sequence_number = 0;
      return sequence_number++;
    }


    /**
     * @brief A method to generate a unique mailbox name given a prefix (this method
     *        simply appends an increasing sequence number to the prefix)
     *
     * @param prefix: a prefix for the mailbox name
     * @return a unique mailbox name as a string
     */
    std::string S4U_Mailbox::generateUniqueMailboxName(std::string prefix) {
      return prefix + "_" + std::to_string(S4U_Mailbox::generateUniqueSequenceNumber());
    }


};
