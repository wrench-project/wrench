/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <iostream>
#include <memory>
#include <xbt/ex.hpp>
#include <simgrid/s4u/Mailbox.hpp>
#include <simgrid/s4u.hpp>

#include "exceptions/WorkflowExecutionException.h"
#include "workflow_execution_events/FailureCause.h"

#include "logging/TerminalOutput.h"
#include "simgrid_S4U_util/S4U_Mailbox.h"
#include "simgrid_S4U_util/S4U_PendingCommunication.h"
#include "simulation/SimulationMessage.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(mailbox, "Mailbox");


namespace wrench {

    class WorkflowTask;

    /**
     * @brief Synchronously receive a message from a mailbox
     *
     * @param mailbox_name: the mailbox name
     * @return the message, or nullptr (in which case it's likely a brutal termination)
     *
     * @throw std::shared_ptr<NetworkError>
     *
     */
    std::unique_ptr<SimulationMessage> S4U_Mailbox::getMessage(std::string mailbox_name) {
      WRENCH_DEBUG("IN GET from %s", mailbox_name.c_str());
      simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(mailbox_name);
      SimulationMessage *msg = nullptr;
      try {
        msg = static_cast<SimulationMessage *>(mailbox->get());
      } catch (xbt_ex &e) {
        if (e.category == network_error) {
          throw std::shared_ptr<NetworkError>(new NetworkError(NetworkError::RECEIVING, mailbox_name));
        } else {
          throw std::runtime_error("S4U_Mailbox::getMessage(): Unexpected xbt_ex exception (" + std::to_string(e.category) + ")");
        }
      }
      // This is just because it seems that after something like a killAll() we get a nullptr
      if (msg == nullptr) {
        WRENCH_INFO("Network error while doing a getMessage(). Got a nullptr...");
        throw std::shared_ptr<NetworkError>(new NetworkError(NetworkError::RECEIVING, mailbox_name));
      }

      WRENCH_DEBUG("GOT a '%s' message from %s", msg->getName().c_str(), mailbox_name.c_str());
      return std::unique_ptr<SimulationMessage>(msg);
    }

    /**
     * @brief Synrhonously receive a message from a mailbox, with a timeout
     *
     * @param mailbox_name: the mailbox name
     * @param timeout:  a timeout value in seconds
     * @return the message, or nullptr (in which case it's likely a brutal termination)
     *
     * @throw std::shared_ptr<NetworkError>
     * @throw std::shared_ptr<NetworkTimeout>
     */
    std::unique_ptr<SimulationMessage> S4U_Mailbox::getMessage(std::string mailbox_name, double timeout) {
      WRENCH_DEBUG("IN GET WITH TIMEOUT (%lf) FROM MAILBOX %s", timeout, mailbox_name.c_str());
      simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(mailbox_name);
      void *data = nullptr;
      try {
        data = mailbox->get(timeout);
      } catch (xbt_ex &e) {
        if (e.category == timeout_error) {
          throw std::shared_ptr<NetworkTimeout>(new NetworkTimeout(NetworkTimeout::RECEIVING, mailbox_name));
        }
        if (e.category == network_error) {
          WRENCH_INFO("Network error while doing a getMessage() with timeout. Likely the sender has died.");
          throw std::shared_ptr<NetworkError>(new NetworkError(NetworkError::RECEIVING, mailbox_name));
        } else {
          throw std::runtime_error("S4U_Mailbox::getMessage(): Unexpected xbt_ex exception (" + std::to_string(e.category) + ")");
        }
      }

      // This is just because it seems that after something like a killAll() we get a nullptr
      if (data == nullptr) {
        WRENCH_INFO("Network error while doing a getMessage() with timeout (got a nullptr).");
        throw std::shared_ptr<NetworkError>(new NetworkError(NetworkError::RECEIVING, mailbox_name));
      }

      SimulationMessage *msg = static_cast<SimulationMessage *>(data);

      WRENCH_INFO("GOT a '%s' message from %s", msg->getName().c_str(), mailbox_name.c_str());

      return std::unique_ptr<SimulationMessage>(msg);
    }

    /**
     * @brief Synchronously send a message to a mailbox
     *
     * @param mailbox_name: the mailbox name
     * @param msg: the SimulationMessage
     *
     * @throw std::shared_ptr<NetworkError>
     */
    void S4U_Mailbox::putMessage(std::string mailbox_name, SimulationMessage *msg) {
      WRENCH_DEBUG("PUTTING to %s a %s message (%lf bytes)", mailbox_name.c_str(), msg->getName().c_str(), msg->payload);
      simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(mailbox_name);
      try {
        mailbox->put(msg, (size_t) msg->payload);
      } catch (xbt_ex &e) {
        if (e.category == network_error) {
          WRENCH_INFO("Network error while doing a putMessage)");
          throw std::shared_ptr<NetworkError>(new NetworkError(NetworkError::SENDING, mailbox_name));
        } else {
          throw std::runtime_error("S4U_Mailbox::putMessage(): Unexpected xbt_ex exception (" + std::to_string(e.category) + ")");
        }
      } catch (std::exception &e) {
        throw;
      }

      return;
    }

    /**
     * @brief Asynchronously send a message to a mailbox in a "fire and forget" fashion
     *
     * @param mailbox_name: the mailbox name
     * @param msg: the SimulationMessage
     *
     * @throw std::shared_ptr<NetworkError>
     */
    void S4U_Mailbox::dputMessage(std::string mailbox_name, SimulationMessage *msg) {

      WRENCH_DEBUG("Dputting to %s a %s message", mailbox_name.c_str(), msg->getName().c_str());

      simgrid::s4u::CommPtr comm = nullptr;

      simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(mailbox_name);

      try {
        mailbox->put_init(msg, msg->payload)->detach();
      } catch (xbt_ex &e) {
        if (e.category == network_error) {
          WRENCH_INFO("Network error while doing a dputMessage()");
          throw std::shared_ptr<NetworkError>(new NetworkError(NetworkError::SENDING, mailbox_name));
        } else {
          throw std::runtime_error("S4U_Mailbox::dputMessage(): Unexpected xbt_ex exception (" + std::to_string(e.category) + ")");
        }
      }

        return;
    }

    /**
    * @brief Asynchronously send a message to a mailbox
    *
    * @param mailbox_name: the mailbox name
    * @param msg: the SimulationMessage
    *
    * @return: a pending communication handle
    *
     * @throw std::shared_ptr<NetworkError>
    */
    std::unique_ptr<S4U_PendingCommunication> S4U_Mailbox::iputMessage(std::string mailbox_name, SimulationMessage *msg) {


      WRENCH_DEBUG("iPUTTING to mailbox %s", mailbox_name.c_str());

      simgrid::s4u::CommPtr comm_ptr = nullptr;

      simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(mailbox_name);
      try {
        comm_ptr = mailbox->put_async(msg, (size_t) msg->payload);
      } catch (xbt_ex &e) {
        if (e.category == network_error) {
          WRENCH_INFO("Network error while doing a iputMessage() with timeout. Likely the sender has died.");
          throw std::shared_ptr<NetworkError>(new NetworkError(NetworkError::SENDING, mailbox_name));
        } else {
          throw std::runtime_error("S4U_Mailbox::iputMessage(): Unexpected xbt_ex exception (" + std::to_string(e.category) + ")");
        }
      }
      std::unique_ptr<S4U_PendingCommunication> pending_communication = std::unique_ptr<S4U_PendingCommunication>(new S4U_PendingCommunication());
      pending_communication->comm_ptr = comm_ptr;
      return pending_communication;
    }

    /**
    * @brief Asynchronously receive a message from a mailbox
    *
    * @param mailbox_name: the mailbox name
    * @param msg: the SimulationMessage
    *
    * @return: a pending communication handle
    *
     * @throw std::shared_ptr<NetworkError>
    */
    std::unique_ptr<S4U_PendingCommunication> S4U_Mailbox::igetMessage(std::string mailbox_name) {

      simgrid::s4u::CommPtr comm_ptr = nullptr;

      WRENCH_DEBUG("iGETTING from mailbox %s", mailbox_name.c_str());

      std::unique_ptr<S4U_PendingCommunication> pending_communication = std::unique_ptr<S4U_PendingCommunication>(new S4U_PendingCommunication());

      simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(mailbox_name);
      try {
        comm_ptr = mailbox->get_async((void**)(&(pending_communication->simulation_message)));
      } catch (xbt_ex &e) {
        if (e.category == network_error) {
          WRENCH_INFO("Network error while doing a igetMessage(). Likely the sender has died.");
          throw std::shared_ptr<NetworkError>(new NetworkError(NetworkError::RECEIVING, mailbox_name));
        } else {
          throw std::runtime_error("S4U_Mailbox::igetMessage(): Unexpected xbt_ex exception (" + std::to_string(e.category) + ")");
        }
      }
      pending_communication->comm_ptr = comm_ptr;
      return pending_communication;
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
