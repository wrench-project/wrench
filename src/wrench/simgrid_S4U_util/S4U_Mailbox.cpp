/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <iostream>
#include <memory>
#include <simgrid/s4u/Mailbox.hpp>
#include <simgrid/s4u.hpp>
#include <wrench/failure_causes/NetworkError.h>

#ifdef MESSAGE_MANAGER
#include <wrench/util/MessageManager.h>
#endif

#include <wrench/failure_causes/FailureCause.h>

#include <wrench/logging/TerminalOutput.h>
#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <wrench/simgrid_S4U_util/S4U_PendingCommunication.h>
#include <wrench/simulation/SimulationMessage.h>

WRENCH_LOG_CATEGORY(wrench_core_mailbox, "Mailbox");

namespace wrench {

    std::deque<simgrid::s4u::Mailbox *> S4U_Mailbox::free_mailboxes;
    std::set<simgrid::s4u::Mailbox *> S4U_Mailbox::used_mailboxes;

    class WorkflowTask;

    /**
     * @brief Synchronously receive a message from a mailbox
     *
     * @param mailbox: the mailbox
     * @return the message, or nullptr (in which case it's likely a brutal termination)
     *
     * @throw std::shared_ptr<NetworkError>
     *
     */
    std::unique_ptr<SimulationMessage> S4U_Mailbox::getMessage(simgrid::s4u::Mailbox *mailbox) {
        WRENCH_INFO("Getting a message from mailbox_name '%s'", mailbox->get_cname());
//        auto mailbox = simgrid::s4u::Mailbox::by_name(mailbox_name);
        SimulationMessage *msg;
        try {
//            msg = static_cast<SimulationMessage *>(mailbox->get());
            msg = mailbox->get<SimulationMessage>();
        } catch (simgrid::NetworkFailureException &e) {
            throw std::shared_ptr<NetworkError>(
                    new NetworkError(NetworkError::RECEIVING, NetworkError::FAILURE, mailbox->get_cname()));
        }

#ifdef MESSAGE_MANAGER
            MessageManager::removeReceivedMessage(mailbox_name, msg);
#endif

        WRENCH_INFO("Received a '%s' message from mailbox_name %s", msg->getName().c_str(), mailbox->get_cname());
        return std::unique_ptr<SimulationMessage>(msg);
    }

    /**
     * @brief Synchronously receive a message from a mailbox, with a timeout
     *
     * @param mailbox: the mailbox
     * @param timeout:  a timeout value in seconds (<0 means never timeout)
     * @return the message, or nullptr (in which case it's likely a brutal termination)
     *
     * @throw std::shared_ptr<NetworkError>
     */
    std::unique_ptr<SimulationMessage> S4U_Mailbox::getMessage(simgrid::s4u::Mailbox *mailbox, double timeout) {

        if (timeout < 0) {
            return S4U_Mailbox::getMessage(mailbox);
        }

        WRENCH_INFO("Getting a message from mailbox_name '%s' with timeout %lf sec", mailbox->get_cname(), timeout);
//        auto mailbox = simgrid::s4u::Mailbox::by_name(mailbox_name);
//        void *data = nullptr;
        wrench::SimulationMessage *msg;

        try {
//            data = mailbox->get(timeout);
            msg = mailbox->get<SimulationMessage>(timeout);
        } catch (simgrid::NetworkFailureException &e) {
            throw std::shared_ptr<NetworkError>(
                    new NetworkError(NetworkError::RECEIVING, NetworkError::FAILURE, mailbox->get_name()));
        } catch (simgrid::TimeoutException &e) {
            throw std::shared_ptr<NetworkError>(
                    new NetworkError(NetworkError::RECEIVING, NetworkError::TIMEOUT, mailbox->get_name()));
        }

//        auto msg = static_cast<SimulationMessage *>(data);


#ifdef MESSAGE_MANAGER
        MessageManager::removeReceivedMessage(mailbox_name, msg);
#endif

        WRENCH_INFO("Received a '%s' message from mailbox_name '%s'", msg->getName().c_str(), mailbox->get_cname());

        return std::unique_ptr<SimulationMessage>(msg);
    }

    /**
     * @brief Synchronously send a message to a mailbox
     *
     * @param mailbox: the mailbox
     * @param msg: the SimulationMessage
     *
     * @throw std::shared_ptr<NetworkError>
     */
    void S4U_Mailbox::putMessage(simgrid::s4u::Mailbox *mailbox, SimulationMessage *msg) {
        WRENCH_INFO("Putting a %s message (%.2lf bytes) to mailbox '%s'",
                     msg->getName().c_str(), msg->payload,
                     mailbox->get_cname());
//        simgrid::s4u::Mailbox *mailbox = simgrid::s4u::Mailbox::by_name(mailbox_name);
        try {
#ifdef MESSAGE_MANAGER
            MessageManager::manageMessage(mailbox->get_name(), msg);
#endif
            mailbox->put(msg, (uint64_t) msg->payload);
        } catch (simgrid::NetworkFailureException &e) {
            throw std::shared_ptr<NetworkError>(
                    new NetworkError(NetworkError::SENDING, NetworkError::FAILURE, mailbox->get_name()));
        } catch (simgrid::TimeoutException &e) {
            // Can happen if the other side is doing a timeout.... I think
            throw std::shared_ptr<NetworkError>(
                    new NetworkError(NetworkError::SENDING, NetworkError::TIMEOUT, mailbox->get_name()));
        }
    }

    /**
     * @brief Asynchronously send a message to a mailbox in a "fire and forget" fashion
     *
     * @param mailbox: the mailbox
     * @param msg: the SimulationMessage
     *
     */
    void S4U_Mailbox::dputMessage(simgrid::s4u::Mailbox *mailbox, SimulationMessage *msg) {

        WRENCH_INFO("Dputting a %s message (%.2lf bytes) to mailbox_name '%s'",
                     msg->getName().c_str(), msg->payload,
                     mailbox->get_cname());

        simgrid::s4u::CommPtr comm = nullptr;

//        auto mailbox = simgrid::s4u::Mailbox::by_name(mailbox_name);

//        try {
#ifdef MESSAGE_MANAGER
            MessageManager::manageMessage(mailbox_name, msg);
#endif
            mailbox->put_init(msg, (uint64_t) msg->payload)->detach();
//        } catch (simgrid::NetworkFailureException &e) {
//            throw std::shared_ptr<NetworkError>(
//                    new NetworkError(NetworkError::SENDING, NetworkError::FAILURE, mailbox_name));
//        } catch (simgrid::TimeoutException &e) {
//            throw std::shared_ptr<NetworkError>(
//                    new NetworkError(NetworkError::SENDING, NetworkError::TIMEOUT, mailbox_name));
//        }
    }

    /**
    * @brief Asynchronously send a message to a mailbox
    *
    * @param mailbox: the mailbox
    * @param msg: the SimulationMessage
    *
    * @return a pending communication handle
    *
    * @throw std::shared_ptr<NetworkError>
    */
    std::shared_ptr<S4U_PendingCommunication>
    S4U_Mailbox::iputMessage(simgrid::s4u::Mailbox *mailbox, SimulationMessage *msg) {

        WRENCH_INFO("Iputting a %s message (%.2lf bytes) to mailbox_name '%s'",
                     msg->getName().c_str(), msg->payload,
                     mailbox->get_cname());

        simgrid::s4u::CommPtr comm_ptr = nullptr;

//        auto mailbox = simgrid::s4u::Mailbox::by_name(mailbox_name);

        try {
#ifdef MESSAGE_MANAGER
            MessageManager::manageMessage(mailbox_name, msg);
#endif
            comm_ptr = mailbox->put_async(msg, (uint64_t) msg->payload);
        } catch (simgrid::NetworkFailureException &e) {
            throw std::shared_ptr<NetworkError>(
                    new NetworkError(NetworkError::SENDING, NetworkError::FAILURE, mailbox->get_name()));
        }

        auto pending_communication = std::shared_ptr<S4U_PendingCommunication>(
                new S4U_PendingCommunication(mailbox, S4U_PendingCommunication::OperationType::SENDING));
        pending_communication->comm_ptr = comm_ptr;
        return pending_communication;
    }

    /**
    * @brief Asynchronously receive a message from a mailbox
    *
    * @param mailbox: the mailbox
    *
    * @return a pending communication handle
    *
     * @throw std::shared_ptr<NetworkError>
    */
    std::shared_ptr<S4U_PendingCommunication> S4U_Mailbox::igetMessage(simgrid::s4u::Mailbox *mailbox) {

        simgrid::s4u::CommPtr comm_ptr = nullptr;

        WRENCH_INFO("Igetting a message from mailbox_name '%s'", mailbox->get_cname());

        std::shared_ptr<S4U_PendingCommunication> pending_communication = std::shared_ptr<S4U_PendingCommunication>(
                new S4U_PendingCommunication(mailbox, S4U_PendingCommunication::OperationType::RECEIVING));

//        auto mailbox = simgrid::s4u::Mailbox::by_name(mailbox_name);
        try {
//            comm_ptr = mailbox->get_async((void **) (&(pending_communication->simulation_message)));
            comm_ptr = mailbox->get_async<void>((void **) (&(pending_communication->simulation_message)));
        } catch (simgrid::NetworkFailureException &e) {
            throw std::shared_ptr<NetworkError>(
                    new NetworkError(NetworkError::RECEIVING, NetworkError::FAILURE, mailbox->get_name()));
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
     * @brief Get a temporary mailbox
     *
     * @return a temporary mailbox
     */
    simgrid::s4u::Mailbox *S4U_Mailbox::getTemporaryMailbox() {


        if (S4U_Mailbox::free_mailboxes.empty()) {
            throw std::runtime_error("S4U_Mailbox::getTemporaryMailbox(): Out of mailboxes! ");
        }
        auto mailbox = *(S4U_Mailbox::free_mailboxes.end() - 1);
        S4U_Mailbox::free_mailboxes.pop_back();
        std::cerr << simgrid::s4u::this_actor::get_pid() << " GOT TEMPORARY MAILBOX " << mailbox->get_name() << "\n";
        S4U_Mailbox::used_mailboxes.insert(mailbox);

        // Drain the mailbox of possible old messages (e.g., due to failures, etc.)
        while (not mailbox->empty()) {
            std::cerr << "***** DRAINING\n";
            auto msg = mailbox->get<SimulationMessage>();
            std::cerr << "***** DRAINED: " << msg->getName() << " from " << mailbox->get_name() << "\n";
//            throw std::runtime_error("HOLY CRAP!\n");
        }

        return mailbox;
    }


    /**
     * @brief Retire a temporary mailbox
     * @param mailbox: the mailbox to retire
     */
    void S4U_Mailbox::retireTemporaryMailbox(simgrid::s4u::Mailbox *mailbox) {
        std::cerr << simgrid::s4u::this_actor::get_pid() << " TRYING TO RETIRE MAILBOX " << mailbox->get_name() << "\n";
        if (S4U_Mailbox::used_mailboxes.find(mailbox) == S4U_Mailbox::used_mailboxes.end()) {
            return;
        }
        S4U_Mailbox::used_mailboxes.erase(mailbox);
        S4U_Mailbox::free_mailboxes.push_front(mailbox);
        std::cerr << simgrid::s4u::this_actor::get_pid() << " RETIRED MAILBOX " << mailbox->get_name() << "\n";
    }

    /**
     * @brief Create the pool of mailboxes to use
     * @param num_mailboxes: numb mailboxes in pool
     */
    void S4U_Mailbox::createMailboxPool(unsigned long num_mailboxes) {
        for (unsigned long i=0; i < num_mailboxes; i++) {
            S4U_Mailbox::free_mailboxes.push_back(simgrid::s4u::Mailbox::by_name("tmp" + std::to_string(S4U_Mailbox::generateUniqueSequenceNumber())));
        }
    }

    /**
     * @brief Generate a unique (non-temporary) mailbox
     * @param prefix: mailbox name prefix
     * @return
     */
    simgrid::s4u::Mailbox *S4U_Mailbox::generateUniqueMailbox(std::string prefix) {
        return simgrid::s4u::Mailbox::by_name(prefix + std::to_string(S4U_Mailbox::generateUniqueSequenceNumber()));
    }


};
