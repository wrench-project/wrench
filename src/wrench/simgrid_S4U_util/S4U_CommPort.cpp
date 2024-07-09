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
#include "wrench/failure_causes/FatalFailure.h"

WRENCH_LOG_CATEGORY(wrench_core_commport, "CommPort");

namespace wrench {

    S4U_CommPort *S4U_CommPort::NULL_COMMPORT;

    std::vector<std::unique_ptr<S4U_CommPort>> S4U_CommPort::all_commports;
    std::deque<S4U_CommPort *> S4U_CommPort::free_commports;
    std::set<S4U_CommPort *> S4U_CommPort::used_commports;
    std::deque<S4U_CommPort *> S4U_CommPort::commports_to_drain;
    unsigned long S4U_CommPort::commport_pool_size = 5000;
    double S4U_CommPort::default_control_message_size;

    /**
     * @brief Constructor
     */
    S4U_CommPort::S4U_CommPort() {
        auto number = std::to_string(S4U_CommPort::generateUniqueSequenceNumber());
        this->s4u_mb = simgrid::s4u::Mailbox::by_name("mb_" + number);
        this->s4u_mq = simgrid::s4u::MessageQueue::by_name("mq_" + number);
        this->mb_comm = nullptr;
        this->mq_comm = nullptr;
        this->mb_comm_posted = false;
        this->mq_comm_posted = false;
        this->name = "cp_" + number;
    }

    /**
     * @brief Destructor
     */
    S4U_CommPort::~S4U_CommPort() {
        //        std::cerr << "IN COMMPORT DESTRUCTOR " << this->name << "\n";
        //        std::cerr << "  - mb_comm_posted " << mb_comm_posted << "\n";
        //        std::cerr << "  - mb_comm " << mb_comm << "\n";
        //        if (mb_comm_posted) {
        //        std::cerr << "  - RECV: " << mb_comm->get_receiver() << "\n";
        //        std::cerr << "  - SENDER: " << mb_comm->get_sender() << "\n";
        //        }
        //        std::cerr << "  - mq_comm_posted " << mq_comm_posted << "\n";
        //        std::cerr << "  - mq_comm " << mq_comm << "\n";
        //        if (mq_comm_posted) {
        //            std::cerr << "  - RECV: " << mq_comm->get_receiver() << "\n";
        //            std::cerr << "  - SENDER: " << mq_comm->get_sender() << "\n";
        //        }
    }

    /**
     * @brief Reset all communication
     */
    void S4U_CommPort::reset() {
        this->mq_comm = nullptr;
        this->mq_comm_posted = false;
        this->mb_comm = nullptr;
        this->mb_comm_posted = false;
    }


    /**
     * @brief Helper method that avoids calling WRENCH_DEBUG from a .h file and do the logging for the templated getMessage() method.
     * It also has the added bonus of checking for inheritance
     *
     * @param type: a pointer to the message so we have its type
     * @param id: an integer id
     *
     */
    void S4U_CommPort::templateWaitingLog(const std::string &type, unsigned long long id) {

        WRENCH_DEBUG("Waiting for message of type <%s> from commport '%s'.  Request ID: %llu", type.c_str(), this->get_cname(), id);
    }

    /**
     * @brief Helper method that avoids calling WRENCH_DEBUG from a .h file and do the logging for the templated getMessage() method.
     * It also has the added bonus of checking for inheritance.
     *
     * @param type: a pointer to the message so we have its type
     * @param id: an integer id
     *
     */
    void S4U_CommPort::templateWaitingLogUpdate(const std::string &type, unsigned long long id) {

        WRENCH_DEBUG("Received a message of type <%s> from commport '%s'.  Request ID: %llu", type.c_str(), this->get_cname(), id);
    }

    /**
     * @brief Synchronously receive a message from a commport
     *
     * @param log: should the log message be printed
     * @return the message, or nullptr (in which case it's likely a brutal termination)
     *
     * @throw std::shared_ptr<NetworkError>
     *
     */
    std::unique_ptr<SimulationMessage> S4U_CommPort::getMessage(bool log) {

        return this->getMessage(-1, log);
    }

    /**
     * @brief Synchronously receive a message from a commport, with a timeout
     *
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

        //        if (log) WRENCH_DEBUG("Getting a message from commport '%s' with timeout %lf sec", this->get_cname(), timeout);
        if (true) WRENCH_DEBUG("Getting a message from commport '%s' with timeout %lf sec", this->get_cname(), timeout);


        simgrid::s4u::ActivitySet pending_receives;
        if (not this->mb_comm_posted) {
            WRENCH_DEBUG("POSTING GET ASYNC ON MB for %s: %p", this->get_cname(), (this->mb_comm.get()));
            this->mb_comm = this->s4u_mb->get_async<SimulationMessage>(&this->msg_mb);
            this->mb_comm_posted = true;
        } else {
            WRENCH_DEBUG("GET ASYNC ON MB ALREADY POSTED FROM BEFORE FOR %s: %p", this->get_cname(), this->mb_comm.get());
        }
        if (not this->mq_comm_posted) {
            WRENCH_DEBUG("POSTING GET ASYNC ON MQ for %s: %p", this->get_cname(), (this->mq_comm.get()));
            this->mq_comm = this->s4u_mq->get_async<SimulationMessage>(&this->msg_mq);
            this->mq_comm_posted = true;
        } else {
            WRENCH_DEBUG("GET ASYNC ON MQ ALREADY POSTED FROM BEFORE FOR %s: %p", this->get_cname(), this->mq_comm.get());
        }

        pending_receives.push(this->mb_comm);
        pending_receives.push(this->mq_comm);

        //        WRENCH_DEBUG("IN GET MESSAGE: %p(%s)   %p(%s)",
        //                    this->mb_comm.get(), this->mb_comm->get_mailbox()->get_cname(),
        //                    this->mq_comm.get(), this->mq_comm->get_queue()->get_cname());

        simgrid::s4u::ActivityPtr finished_recv;
        try {
            // Wait for one activity to complete
            finished_recv = pending_receives.wait_any_for(timeout);
        } catch (simgrid::TimeoutException &e) {
            //            WRENCH_DEBUG("Got A TimeoutException");
            pending_receives.erase(this->mq_comm);
            pending_receives.erase(this->mb_comm);
            this->mq_comm->cancel();
            this->mq_comm_posted = false;
            this->mq_comm = nullptr;
            this->mb_comm->cancel();
            this->mq_comm = nullptr;
            this->mb_comm_posted = false;
            throw ExecutionException(std::make_shared<NetworkError>(NetworkError::RECEIVING, NetworkError::TIMEOUT, this->name, ""));
        } catch (simgrid::Exception &e) {
            //            WRENCH_DEBUG("Got A simgrid::Exception");
            auto failed_recv = pending_receives.get_failed_activity();
            if (failed_recv == this->mb_comm) {
                pending_receives.erase(this->mb_comm);
                pending_receives.erase(this->mq_comm);
                this->mb_comm_posted = false;
                this->mb_comm = nullptr;
                throw ExecutionException(std::make_shared<NetworkError>(
                        NetworkError::RECEIVING, NetworkError::FAILURE, this->name, ""));
            } else {
                pending_receives.erase(this->mq_comm);
                pending_receives.erase(this->mb_comm);
                this->mq_comm_posted = false;
                this->mq_comm = nullptr;
                throw ExecutionException(std::make_shared<NetworkError>(
                        NetworkError::RECEIVING, NetworkError::FAILURE, this->name, ""));
            }
        }

        SimulationMessage *msg = nullptr;

        if (finished_recv == this->mb_comm) {
            //            WRENCH_DEBUG("SOME COMM FINISHED ON MB");
            pending_receives.erase(this->mq_comm);
            pending_receives.erase(this->mb_comm);
            msg = this->msg_mb;
            this->mb_comm_posted = false;
            this->mb_comm = nullptr;
        } else if (finished_recv == this->mq_comm) {
            //            WRENCH_DEBUG("SOME COMM FINISHED ON MQ");
            pending_receives.erase(this->mb_comm);
            pending_receives.erase(this->mq_comm);
            msg = this->msg_mq;
            this->mq_comm_posted = false;
            this->mq_comm = nullptr;
        } else {
            throw std::runtime_error("S4U_CommPort::getMessage(): unknown completed communication - this should never happen: " +
                                     std::to_string((unsigned long) (finished_recv.get())) + "  " + finished_recv->get_name());
        }

#ifdef MESSAGE_MANAGER
        MessageManager::removeReceivedMessage(this, msg);
#endif

        WRENCH_DEBUG("Received a '%s' message from commport '%s' (%lf, %p bytes)",
                     msg->getName().c_str(), this->get_cname(),
                     msg->payload, msg);

        return std::unique_ptr<SimulationMessage>(msg);
    }

    /**
     * @brief Synchronously send a message to a commport
     *
     * @param msg: the SimulationMessage
     *
     * @throw std::shared_ptr<NetworkError>
     */
    void S4U_CommPort::putMessage(SimulationMessage *msg) {

        if (this == S4U_CommPort::NULL_COMMPORT) {
            return;
        }
        WRENCH_DEBUG("Putting a %s message (%.2lf bytes, %p) to commport '%s'",
                     msg->getName().c_str(), msg->payload, msg,
                     this->get_cname());

#ifdef MESSAGE_MANAGER
        MessageManager::manageMessage(this, msg);
#endif
        if (msg->payload > 0) {
            try {
                this->s4u_mb->put(msg, (uint64_t) msg->payload);
            } catch (simgrid::NetworkFailureException &e) {
                throw ExecutionException(std::make_shared<NetworkError>(
                        NetworkError::SENDING, NetworkError::FAILURE, this->s4u_mb->get_name(), msg->getName()));
            } catch (simgrid::TimeoutException &e) {
                // Can happen if the other side is doing a timeout.... I think
                throw ExecutionException(std::make_shared<NetworkError>(
                        NetworkError::SENDING, NetworkError::TIMEOUT, this->s4u_mb->get_name(), msg->getName()));
            }
        } else {
            try {
                this->s4u_mq->put(msg);
            } catch (simgrid::TimeoutException &e) {
                // Can happen if the other side is doing a timeout.... I think
                throw ExecutionException(std::make_shared<NetworkError>(
                        NetworkError::SENDING, NetworkError::TIMEOUT, this->s4u_mq->get_name(), msg->getName()));
            } catch (simgrid::Exception &e) {
                throw ExecutionException(std::make_shared<NetworkError>(
                        NetworkError::SENDING, NetworkError::FAILURE, this->s4u_mq->get_name(), msg->getName()));
            }
        }
    }

    /**
     * @brief Asynchronously send a message to a commport in a "fire and forget" fashion
     *
     * @param msg: the SimulationMessage
     *
     */
    void S4U_CommPort::dputMessage(SimulationMessage *msg) {

        if (this == S4U_CommPort::NULL_COMMPORT) {
            return;
        }

        WRENCH_DEBUG("Dputting a %s message (%.2lf bytes, %p) to commport '%s'",
                     msg->getName().c_str(), msg->payload, msg,
                     this->get_cname());

#ifdef MESSAGE_MANAGER
        MessageManager::manageMessage(this, msg);
#endif
        if (msg->payload != 0) {
            this->s4u_mb->put_init(msg, (uint64_t) msg->payload)->detach();
        } else {
            this->s4u_mq->put_init(msg)->detach();
        }
    }

    /**
    * @brief Asynchronously send a message to a commport
    *
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
                     this->get_cname());

#ifdef MESSAGE_MANAGER
        MessageManager::manageMessage(this, msg);
#endif

        std::shared_ptr<S4U_PendingCommunication> pending_communication;

        if (msg->payload != 0) {
            simgrid::s4u::CommPtr comm_ptr;
            try {
                comm_ptr = this->s4u_mb->put_async(msg, (uint64_t) msg->payload);
            } catch (simgrid::NetworkFailureException &e) {
                throw ExecutionException(std::make_shared<NetworkError>(
                        NetworkError::SENDING, NetworkError::FAILURE, this->s4u_mb->get_name(), msg->getName()));
            }

            pending_communication = std::make_shared<S4U_PendingCommunication>(
                    this, S4U_PendingCommunication::OperationType::SENDING);
            pending_communication->comm_ptr = comm_ptr;
        } else {
            simgrid::s4u::MessPtr mess_ptr;
            try {
                mess_ptr = this->s4u_mq->put_async(msg);
            } catch (simgrid::NetworkFailureException &e) {
                throw ExecutionException(std::make_shared<NetworkError>(
                        NetworkError::SENDING, NetworkError::FAILURE, this->s4u_mq->get_name(), msg->getName()));
            }
            pending_communication = std::make_shared<S4U_PendingCommunication>(
                    this, S4U_PendingCommunication::OperationType::SENDING);
            pending_communication->mess_ptr = mess_ptr;
        }
        return pending_communication;
    }


    /**
    * @brief Asynchronously receive a message from a commport
    *
    * @return a pending communication handle
    *
     * @throw std::shared_ptr<NetworkError>
    */
    std::shared_ptr<S4U_PendingCommunication> S4U_CommPort::igetMessage() {

        if (this == S4U_CommPort::NULL_COMMPORT) {
            throw std::invalid_argument("S4U_CommPort::igetMessage(): Cannot be called with NULL_COMMPORT");
        }

        WRENCH_DEBUG("Igetting a message from commport '%s'", this->get_cname());

        std::shared_ptr<S4U_PendingCommunication> pending_communication = std::make_shared<S4U_PendingCommunication>(
                this, S4U_PendingCommunication::OperationType::RECEIVING);

        try {
            auto comm_ptr = this->s4u_mb->get_async<void>((void **) (&(pending_communication->simulation_message)));
            pending_communication->comm_ptr = comm_ptr;
        } catch (simgrid::NetworkFailureException &e) {
            throw ExecutionException(std::make_shared<NetworkError>(
                    NetworkError::RECEIVING, NetworkError::FAILURE, this->s4u_mb->get_name(), ""));
        }

        simgrid::s4u::MessPtr mess_ptr = this->s4u_mq->get_async<void>((void **) (&(pending_communication->simulation_message)));
        pending_communication->mess_ptr = mess_ptr;
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
            throw std::runtime_error("S4U_CommPort::getTemporaryCommPort(): Out of communication ports! "
                                     "(Increase the communication port pool size with the --wrench-commport-pool-size command-line argument (currently set at: " +
                                     std::to_string(S4U_CommPort::commport_pool_size) + ")");
        }

        //        std::cerr << "FREE MAILBOX: " << S4U_CommPort::free_commports.size() << "\n";

        auto commport = *(S4U_CommPort::free_commports.end() - 1);
        S4U_CommPort::free_commports.pop_back();

        if ((not commport->s4u_mb->empty()) or (not commport->s4u_mq->empty())) {
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
        commport->reset();// Just in case
        //        WRENCH_DEBUG("Gotten temporary commport %s (%p %p)", commport->name.c_str(), commport->mq_comm.get(), commport->mb_comm.get());
        return commport;
    }


    /**
     * @brief Retire a temporary commport
     *
     * @param commport: the commport to retire
     */
    void S4U_CommPort::retireTemporaryCommPort(S4U_CommPort *commport) {
        //        WRENCH_DEBUG("Calling reset() on commport %s", commport->get_cname());
        if (commport->mb_comm) {
            commport->mb_comm->cancel();
        }
        if (commport->mq_comm) {
            commport->mq_comm->cancel();
        }
        commport->reset();
        if (S4U_CommPort::used_commports.find(commport) == S4U_CommPort::used_commports.end()) {
            return;
        }
        WRENCH_DEBUG("Retiring commport %s", commport->name.c_str());
        S4U_CommPort::used_commports.erase(commport);
        S4U_CommPort::free_commports.push_front(commport);//
    }

    /**
     * @brief Create the pool of commports to use
     */
    void S4U_CommPort::createCommPortPool() {
        S4U_CommPort::all_commports.reserve(S4U_CommPort::commport_pool_size);
        for (unsigned long i = 0; i < S4U_CommPort::commport_pool_size; i++) {
            std::unique_ptr<S4U_CommPort> mb = std::make_unique<S4U_CommPort>();
            S4U_CommPort::free_commports.push_back(mb.get());
            S4U_CommPort::all_commports.push_back(std::move(mb));
        }
    }

    unsigned long long S4U_CommPort::messageCounter = 0;
}// namespace wrench
