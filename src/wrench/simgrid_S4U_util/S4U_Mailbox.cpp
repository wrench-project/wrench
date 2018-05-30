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
#include <wrench/util/MessageManager.h>

#include "wrench/exceptions/WorkflowExecutionException.h"
#include "wrench/workflow/execution_events/FailureCause.h"

#include "wrench/logging/TerminalOutput.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/simgrid_S4U_util/S4U_PendingCommunication.h"
#include "wrench/simulation/SimulationMessage.h"

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
     * @throw std::shared_ptr<FatalFailure>
     *
     */
    std::unique_ptr<SimulationMessage> S4U_Mailbox::getMessage(std::string mailbox_name) {
      WRENCH_DEBUG("Getting a message from mailbox_name '%s'", mailbox_name.c_str());
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
      } catch (std::exception &e) {
        throw std::shared_ptr<FatalFailure>(new FatalFailure());
      }
      // This is just because it seems that after something like a killAll() we get a nullptr
      if (msg == nullptr) {
        throw std::shared_ptr<FatalFailure>(new FatalFailure());
      }

      //Remove this message from the message manager list
      MessageManager::removeReceivedMessages(mailbox_name,msg);
      WRENCH_DEBUG("Received a '%s' message from mailbox_name %s", msg->getName().c_str(), mailbox_name.c_str());
      return std::unique_ptr<SimulationMessage>(msg);
    }

    /**
     * @brief Synchronously receive a message from a mailbox, with a timeout
     *
     * @param mailbox_name: the mailbox name
     * @param timeout:  a timeout value in seconds (<0 means never timeout)
     * @return the message, or nullptr (in which case it's likely a brutal termination)
     *
     * @throw std::shared_ptr<NetworkError>
     * @throw std::shared_ptr<NetworkTimeout>
     * @throw std::shared_ptr<FatalFailure>
     */
    std::unique_ptr<SimulationMessage> S4U_Mailbox::getMessage(std::string mailbox_name, double timeout) {

      if (timeout < 0) {
        return S4U_Mailbox::getMessage(mailbox_name);
      }

      WRENCH_DEBUG("Getting a message from mailbox_name '%s' with timeout %lf sec", mailbox_name.c_str(), timeout);
      simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(mailbox_name);
      void *data = nullptr;

      try {
        data = mailbox->get(timeout);
      } catch (xbt_ex &e) {
        if (e.category == timeout_error) {
          throw std::shared_ptr<NetworkTimeout>(new NetworkTimeout(NetworkTimeout::RECEIVING, mailbox_name));
        }
        if (e.category == network_error) {
          throw std::shared_ptr<NetworkError>(new NetworkError(NetworkError::RECEIVING, mailbox_name));
        } else {
          throw std::runtime_error("S4U_Mailbox::getMessage(): Unexpected xbt_ex exception (" + std::to_string(e.category) + ")");
        }
      } catch (std::exception &e) {
        throw std::shared_ptr<FatalFailure>(new FatalFailure());
      }

      // This is just because it seems that after something like a killAll() we get a nullptr
      if (data == nullptr) {

        throw std::shared_ptr<FatalFailure>(new FatalFailure());
      }

      SimulationMessage *msg = static_cast<SimulationMessage *>(data);

      //Remove this message from the message manager list
      MessageManager::removeReceivedMessages(mailbox_name,msg);

      WRENCH_INFO("Received a '%s' message from mailbox_name '%s'", msg->getName().c_str(), mailbox_name.c_str());

      return std::unique_ptr<SimulationMessage>(msg);
    }

    /**
     * @brief Synchronously send a message to a mailbox
     *
     * @param mailbox_name: the mailbox name
     * @param msg: the SimulationMessage
     *
     * @throw std::shared_ptr<NetworkError>
     * @throw std::shared_ptr<FatalFailure>
     */
    void S4U_Mailbox::putMessage(std::string mailbox_name, SimulationMessage *msg) {
      WRENCH_DEBUG("Putting a %s message (%.2lf bytes) to mailbox_name '%s'",
                   msg->getName().c_str(), msg->payload,
                   mailbox_name.c_str());
      simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(mailbox_name);
      try {
        //also let the MessageManager manage this message
        MessageManager::manageMessage(mailbox_name,msg);
        mailbox->put(msg, (uint64_t) msg->payload);
      } catch (xbt_ex &e) {
        if ((e.category == network_error) || (e.category == timeout_error)) {
          throw std::shared_ptr<NetworkError>(new NetworkError(NetworkError::SENDING, mailbox_name));
        } else {
          throw std::runtime_error("S4U_Mailbox::putMessage(): Unexpected xbt_ex exception (" + std::to_string(e.category) + ")");
        }
      } catch (std::exception &e) {
        throw std::shared_ptr<FatalFailure>(new FatalFailure());
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
     * @throw std::shared_ptr<FatalFailure>
     */
    void S4U_Mailbox::dputMessage(std::string mailbox_name, SimulationMessage *msg) {

      WRENCH_DEBUG("Dputting a %s message (%.2lf bytes) to mailbox_name '%s'",
                   msg->getName().c_str(), msg->payload,
                   mailbox_name.c_str());

      simgrid::s4u::CommPtr comm = nullptr;

      simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(mailbox_name);

      try {
        mailbox->put_init(msg, (uint64_t) msg->payload)->detach();
      } catch (xbt_ex &e) {
        if ((e.category == network_error) || (e.category == timeout_error)) {
          WRENCH_INFO("Network error while doing a dputMessage()");
          throw std::shared_ptr<NetworkError>(new NetworkError(NetworkError::SENDING, mailbox_name));
        } else {
          throw std::runtime_error("S4U_Mailbox::dputMessage(): Unexpected xbt_ex exception (" + std::to_string(e.category) + ")");
        }
      } catch (std::exception &e) {
        WRENCH_INFO("dputMessage(): Got a std::exception");
        throw std::shared_ptr<FatalFailure>(new FatalFailure());
      }

      return;
    }

    /**
    * @brief Asynchronously send a message to a mailbox
    *
    * @param mailbox_name: the mailbox name
    * @param msg: the SimulationMessage
    *
    * @return a pending communication handle
    *
    * @throw std::shared_ptr<NetworkError>
    * @throw std::shared_ptr<FatalFailure>
    */
    std::unique_ptr<S4U_PendingCommunication> S4U_Mailbox::iputMessage(std::string mailbox_name, SimulationMessage *msg) {

      WRENCH_DEBUG("Iputting a %s message (%.2lf bytes) to mailbox_name '%s'",
                   msg->getName().c_str(), msg->payload,
                   mailbox_name.c_str());

      simgrid::s4u::CommPtr comm_ptr = nullptr;

      simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(mailbox_name);
      try {
        comm_ptr = mailbox->put_async(msg, (uint64_t) msg->payload);
      } catch (xbt_ex &e) {
        if (e.category == network_error) {
          throw std::shared_ptr<NetworkError>(new NetworkError(NetworkError::SENDING, mailbox_name));
        } else {
          throw std::runtime_error("S4U_Mailbox::iputMessage(): Unexpected xbt_ex exception (" + std::to_string(e.category) + ")");
        }
      } catch (std::exception &e) {
        throw std::shared_ptr<FatalFailure>(new FatalFailure());
      }
      std::unique_ptr<S4U_PendingCommunication> pending_communication = std::unique_ptr<S4U_PendingCommunication>(new S4U_PendingCommunication(mailbox_name));
      pending_communication->comm_ptr = comm_ptr;
      return pending_communication;
    }

    /**
    * @brief Asynchronously receive a message from a mailbox
    *
    * @param mailbox_name: the mailbox name
    *
    * @return a pending communication handle
    *
     * @throw std::shared_ptr<NetworkError>
    */
    std::unique_ptr<S4U_PendingCommunication> S4U_Mailbox::igetMessage(std::string mailbox_name) {

      simgrid::s4u::CommPtr comm_ptr = nullptr;

      WRENCH_DEBUG("Igetting a message from mailbox_name '%s'", mailbox_name.c_str());

      std::unique_ptr<S4U_PendingCommunication> pending_communication = std::unique_ptr<S4U_PendingCommunication>(new S4U_PendingCommunication(mailbox_name));

      simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(mailbox_name);
      try {
        comm_ptr = mailbox->get_async((void**)(&(pending_communication->simulation_message)));
      } catch (xbt_ex &e) {
        if (e.category == network_error) {
          throw std::shared_ptr<NetworkError>(new NetworkError(NetworkError::RECEIVING, mailbox_name));
        } else {
          throw std::runtime_error("S4U_Mailbox::igetMessage(): Unexpected xbt_ex exception (" + std::to_string(e.category) + ")");
        }
      } catch (std::exception &e) {
        throw std::shared_ptr<FatalFailure>(new FatalFailure());
      }
      pending_communication->comm_ptr = comm_ptr;
      return pending_communication;
    }


    /**
    * @brief Generate a unique sequence number
    *
    * @return a unique sequence number
    */
    unsigned long S4U_Mailbox::generateUniqueSequenceNumber() {
      static unsigned long sequence_number = 0;
      return sequence_number++;
    }


    /**
     * @brief Generate a unique mailbox name given a prefix (this method
     *        simply appends an increasing sequence number to the prefix)
     *
     * @param prefix: a prefix for the mailbox name
     * @return a unique mailbox name as a string
     */
    std::string S4U_Mailbox::generateUniqueMailboxName(std::string prefix) {
      return prefix + "_" + std::to_string(S4U_Mailbox::generateUniqueSequenceNumber());
    }

};
