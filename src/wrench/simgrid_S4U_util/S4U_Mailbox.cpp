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
        WRENCH_DEBUG("Getting a message from mailbox_name '%s'", mailbox->get_cname());
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

        WRENCH_DEBUG("Received a '%s' message from mailbox_name %s", msg->getName().c_str(), mailbox->get_cname());
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

        WRENCH_DEBUG("Getting a message from mailbox_name '%s' with timeout %lf sec", mailbox->get_cname(), timeout);
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

        WRENCH_DEBUG("Received a '%s' message from mailbox_name '%s'", msg->getName().c_str(), mailbox->get_cname());

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
        WRENCH_DEBUG("Putting a %s message (%.2lf bytes) to mailbox '%s'",
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

        WRENCH_DEBUG("Dputting a %s message (%.2lf bytes) to mailbox_name '%s'",
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

        WRENCH_DEBUG("Iputting a %s message (%.2lf bytes) to mailbox_name '%s'",
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

        WRENCH_DEBUG("Igetting a message from mailbox_name '%s'", mailbox->get_cname());

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
