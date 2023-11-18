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
#include <wrench/simgrid_S4U_util/S4U_CommPort.h>
#include <wrench/simgrid_S4U_util/S4U_PendingCommunication.h>
#include <wrench/simulation/SimulationMessage.h>
#include "wrench/exceptions/ExecutionException.h"

WRENCH_LOG_CATEGORY(wrench_core_commport, "CommPort");

namespace wrench {

    S4U_CommPort *S4U_CommPort::NULL_COMMPORT;

    std::vector<std::unique_ptr<S4U_CommPort>> S4U_CommPort::all_commports;
    std::deque<S4U_CommPort *> S4U_CommPort::free_commports;
    std::set<S4U_CommPort *> S4U_CommPort::used_commports;
    std::deque<S4U_CommPort *> S4U_CommPort::commports_to_drain;
    unsigned long S4U_CommPort::commport_pool_size;
    double S4U_CommPort::default_control_message_size;



    class WorkflowTask;
    /**
     * @brief Helper method that avoids calling WRENCH_DEBUG from a .h file and do the logging for the templated getMessage() method.  
     * It also has the added bonus of checking for inheritance
     *
     * @param commport: the commport so we can get its name
     * @param type: a pointer to the message so we have its type
     * @param id: an integer id
     *
     */
    void S4U_CommPort::templateWaitingLog(const std::string& type, unsigned long long id) {

        WRENCH_DEBUG("Waiting for message of type <%s> from commport '%s'.  Request ID: %llu", type.c_str(), this->s4u_mb->get_cname(), id);
    }

    /**
     * @brief Helper method that avoids calling WRENCH_DEBUG from a .h file and do the logging for the templated getMessage() method.  
     * It also has the added bonus of checking for inheritance.
     *
     * @param commport: the commport so we can get its name
     * @param type: a pointer to the message so we have its type
     * @param id: an integer id
     *
     */
    void S4U_CommPort::templateWaitingLogUpdate(const std::string& type, unsigned long long id) {

        WRENCH_DEBUG("Received a message of type <%s> from commport '%s'.  Request ID: %llu", type.c_str(), this->s4u_mb->get_cname(), id);
    }

    /**
     * @brief Synchronously receive a message from a commport
     *
     * @param commport: the commport
     * @param log: should the log message be printed
     * @return the message, or nullptr (in which case it's likely a brutal termination)
     *
     * @throw std::shared_ptr<NetworkError>
     *
     */
    std::unique_ptr<SimulationMessage> S4U_CommPort::getMessage(bool log) {
        if (this == S4U_CommPort::NULL_COMMPORT) {
            throw std::invalid_argument("S4U_CommPort::getMessage(): Cannot be called with NULL_COMMPORT");
        }

        if (log) WRENCH_DEBUG("Getting a message from commport '%s'", this->s4u_mb->get_cname());
        SimulationMessage *msg;
        try {
            //            msg = static_cast<SimulationMessage *>(commport->get());
            msg = this->s4u_mb->get<SimulationMessage>();
        } catch (simgrid::NetworkFailureException &e) {
            throw ExecutionException(std::make_shared<NetworkError>(
                    NetworkError::RECEIVING, NetworkError::FAILURE, this->s4u_mb->get_cname()));
        }

#ifdef MESSAGE_MANAGER
        MessageManager::removeReceivedMessage(this, msg);
#endif

        WRENCH_DEBUG("Received a '%s' message from commport %s", msg->getName().c_str(), this->s4u_mb->get_cname());
        return std::unique_ptr<SimulationMessage>(msg);
    }

    /**
     * @brief Synchronously receive a message from a commport, with a timeout
     *
     * @param commport: the commport
     * @param timeout:  a timeout value in seconds (<0 means never timeout)
     * @param log: should the log message be printed
     * @return the message, or nullptr (in which case it's likely a brutal termination)
     *
     * @throw std::shared_ptr<NetworkError>
     */
    std::unique_ptr<SimulationMessage> S4U_CommPort::getMessage(double timeout, bool log) {
        if (this == S4U_CommPort::NULL_COMMPORT) {
            throw std::invalid_argument("S4U_CommPort::getMessage(): Cannot be called with NULL_COMMPORT");
        }

        if (timeout < 0) {
            return this->getMessage();
        }

        if (log) WRENCH_DEBUG("Getting a message from commport '%s' with timeout %lf sec", this->s4u_mb->get_cname(), timeout);
        wrench::SimulationMessage *msg;

        try {
            //            data = commport->get(timeout);
            msg = this->s4u_mb->get<SimulationMessage>(timeout);
        } catch (simgrid::NetworkFailureException &e) {
            throw ExecutionException(std::make_shared<NetworkError>(
                    NetworkError::RECEIVING, NetworkError::FAILURE, this->s4u_mb->get_name()));
        } catch (simgrid::TimeoutException &e) {
            throw ExecutionException(std::make_shared<NetworkError>(
                    NetworkError::RECEIVING, NetworkError::TIMEOUT, this->s4u_mb->get_name()));
        }

        //        auto msg = static_cast<SimulationMessage *>(data);


#ifdef MESSAGE_MANAGER
        MessageManager::removeReceivedMessage(this, msg);
#endif

        WRENCH_DEBUG("Received a '%s' message from commport '%s'", msg->getName().c_str(), this->s4u_mb->get_cname());

        return std::unique_ptr<SimulationMessage>(msg);
    }

    /**
     * @brief Synchronously send a message to a commport
     *
     * @param commport: the commport
     * @param msg: the SimulationMessage
     *
     * @throw std::shared_ptr<NetworkError>
     */
    void S4U_CommPort::putMessage(SimulationMessage *msg) {

        if (this == S4U_CommPort::NULL_COMMPORT) {
            return;
        }

        WRENCH_DEBUG("Putting a %s message (%.2lf bytes) to commport '%s'",
                     msg->getName().c_str(), msg->payload,
                     this->s4u_mb->get_cname());
        try {
#ifdef MESSAGE_MANAGER
            MessageManager::manageMessage(this, msg);
#endif
            this->s4u_mb->put(msg, (uint64_t) msg->payload);
        } catch (simgrid::NetworkFailureException &e) {
            throw ExecutionException(std::make_shared<NetworkError>(
                    NetworkError::SENDING, NetworkError::FAILURE, this->s4u_mb->get_name()));
        } catch (simgrid::TimeoutException &e) {
            // Can happen if the other side is doing a timeout.... I think
            throw ExecutionException(std::make_shared<NetworkError>(
                    NetworkError::SENDING, NetworkError::TIMEOUT, this->s4u_mb->get_name()));
        }
    }

    /**
     * @brief Asynchronously send a message to a commport in a "fire and forget" fashion
     *
     * @param commport: the commport
     * @param msg: the SimulationMessage
     *
     */
    void S4U_CommPort::dputMessage(SimulationMessage *msg) {

        if (this == S4U_CommPort::NULL_COMMPORT) {
            return;
        }

        WRENCH_DEBUG("Dputting a %s message (%.2lf bytes) to commport '%s'",
                     msg->getName().c_str(), msg->payload,
                     this->s4u_mb->get_cname());

#ifdef MESSAGE_MANAGER
        MessageManager::manageMessage(this, msg);
#endif
	//if (msg->payload)
        this->s4u_mb->put_init(msg, (uint64_t) msg->payload)->detach();
	//else
        //  commport->put(msg, 0);
    }

    /**
    * @brief Asynchronously send a message to a commport
    *
    * @param commport: the commport
    * @param msg: the SimulationMessage
    *
    * @return a pending communication handle
    *
    * @throw std::shared_ptr<NetworkError>
    */
    std::shared_ptr<S4U_PendingCommunication>
    S4U_CommPort::iputMessage(SimulationMessage *msg) {

        if (this == S4U_CommPort::NULL_COMMPORT) {
            return nullptr;
        }

        WRENCH_DEBUG("Iputting a %s message (%.2lf bytes) to commport '%s'",
                     msg->getName().c_str(), msg->payload,
                     this->s4u_mb->get_cname());

        simgrid::s4u::CommPtr comm_ptr = nullptr;

        try {
#ifdef MESSAGE_MANAGER
            MessageManager::manageMessage(this, msg);
#endif
            comm_ptr = this->s4u_mb->put_async(msg, (uint64_t) msg->payload);
        } catch (simgrid::NetworkFailureException &e) {
            throw ExecutionException(std::make_shared<NetworkError>(
                    NetworkError::SENDING, NetworkError::FAILURE, this->s4u_mb->get_name()));
        }

        auto pending_communication = std::make_shared<S4U_PendingCommunication>(
                this, S4U_PendingCommunication::OperationType::SENDING);
        pending_communication->comm_ptr = comm_ptr;
        return pending_communication;
    }


    /**
    * @brief Asynchronously receive a message from a commport
    *
    * @param commport: the commport
    *
    * @return a pending communication handle
    *
     * @throw std::shared_ptr<NetworkError>
    */
    std::shared_ptr<S4U_PendingCommunication> S4U_CommPort::igetMessage() {

        if (this == S4U_CommPort::NULL_COMMPORT) {
            throw std::invalid_argument("S4U_CommPort::igetMessage(): Cannot be called with NULL_COMMPORT");
        }

        simgrid::s4u::CommPtr comm_ptr = nullptr;

        WRENCH_DEBUG("Igetting a message from commport '%s'", this->s4u_mb->get_cname());

        std::shared_ptr<S4U_PendingCommunication> pending_communication = std::make_shared<S4U_PendingCommunication>(
                this, S4U_PendingCommunication::OperationType::RECEIVING);

        try {
            comm_ptr = this->s4u_mb->get_async<void>((void **) (&(pending_communication->simulation_message)));
        } catch (simgrid::NetworkFailureException &e) {
            throw ExecutionException(std::make_shared<NetworkError>(
                    NetworkError::RECEIVING, NetworkError::FAILURE, this->s4u_mb->get_name()));
        }
        pending_communication->comm_ptr = comm_ptr;
        return pending_communication;
    }


    /**
    * @brief Generate a unique sequence number
    *
    * @return a unique sequence number
    */
    unsigned long S4U_CommPort::generateUniqueSequenceNumber() {
        static unsigned long sequence_number = 0;
        return sequence_number++;
    }


    /**
     * @brief Get a temporary commport
     *
     * @return a temporary commport
     */
    S4U_CommPort *S4U_CommPort::getTemporaryCommPort() {
        if (S4U_CommPort::free_commports.empty()) {
            throw std::runtime_error("S4U_CommPort::getTemporaryCommPort(): Out of commportes! "
                                     "(Increase the commport pool size with the --wrench-commport-pool-size command-line argument (default is 5000))");
        }

        //        std::cerr << "FREE MAILBOX: " << S4U_CommPort::free_commports.size() << "\n";

        auto commport = *(S4U_CommPort::free_commports.end() - 1);
        S4U_CommPort::free_commports.pop_back();
        //        std::cerr << simgrid::s4u::this_actor::get_pid() << " GOT TEMPORARY MAILBOX " << commport->get_name() << "\n";

        if (not commport->s4u_mb->empty()) {
            //            std::cerr << "############### WASTING MAILBOX " << commport->get_name() << "\n";
            S4U_CommPort::commports_to_drain.push_front(commport);
            return S4U_CommPort::getTemporaryCommPort();// Recursive call!

            //            // Drain one commport
            //            if (not S4U_CommPort::commports_to_drain.empty()) {
            //                auto to_drain = *(S4U_CommPort::commports_to_drain.end() - 1);
            //                std::cerr << "############ UNWASTING MAILBOX " << to_drain->get_name() << "\n";
            //                S4U_CommPort::commports_to_drain.pop_back();
            //                while (not to_drain->empty()) {
            //                    to_drain->get<SimulationMessage>();
            //                }
            //            }
        }

        S4U_CommPort::used_commports.insert(commport);

        return commport;
    }


    /**
     * @brief Retire a temporary commport
     * @param commport: the commport to retire
     */
    void S4U_CommPort::retireTemporaryCommPort(S4U_CommPort *commport) {
        //        std::cerr << simgrid::s4u::this_actor::get_pid() << " TRYING TO RETIRE MAILBOX " << commport->get_name() << "\n";
        if (S4U_CommPort::used_commports.find(commport) == S4U_CommPort::used_commports.end()) {
            return;
        }
        S4U_CommPort::used_commports.erase(commport);
        S4U_CommPort::free_commports.push_front(commport);
        //        std::cerr << simgrid::s4u::this_actor::get_pid() << " RETIRED MAILBOX " << commport->get_name() << "\n";
    }

    /**
     * @brief Create the pool of commports to use
     * @param num_commports: numb commports in pool
     */
    void S4U_CommPort::createCommPortPool(unsigned long num_commports) {
        S4U_CommPort::all_commports.reserve(num_commports);
        for (unsigned long i = 0; i < num_commports; i++) {
            std::unique_ptr<S4U_CommPort> mb = std::make_unique<S4U_CommPort>();
            S4U_CommPort::free_commports.push_back(mb.get());
            S4U_CommPort::all_commports.push_back(std::move(mb));
        }
    }

    unsigned long long S4U_CommPort::messageCounter = 0;
}// namespace wrench
