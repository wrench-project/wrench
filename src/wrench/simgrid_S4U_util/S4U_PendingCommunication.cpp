/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <memory>
#include <iostream>
#include <wrench/failure_causes/NetworkError.h>
#include "wrench/failure_causes/FatalFailure.h"


#ifdef MESSAGE_MANAGER
#include <wrench/util/MessageManager.h>
#endif

#include <wrench/simulation/Simulation.h>

#include <wrench/logging/TerminalOutput.h>
#include <wrench/simgrid_S4U_util/S4U_PendingCommunication.h>
#include <wrench/simulation/SimulationMessage.h>
#include <wrench/failure_causes/FailureCause.h>
#include "wrench/exceptions/ExecutionException.h"

WRENCH_LOG_CATEGORY(wrench_core_pending_communication, "Log category for Pending Communication");

namespace wrench {

    /**
     * @brief Wait for the pending communication to complete
     *
     * @return A (unique pointer to a) simulation message
     *
     * @throw std::shared_ptr<NetworkError>
     */
    std::unique_ptr<SimulationMessage> S4U_PendingCommunication::wait() {
        return this->wait(-1);
    }

    /**
     * @brief Wait for the pending communication to complete with a timeout
     * @param timeout: a timeout in seconds
     *
     * @return A (unique pointer to a) simulation message
     *
     * @throw std::shared_ptr<NetworkError>
     */
    std::unique_ptr<SimulationMessage> S4U_PendingCommunication::wait(double timeout) {

#if 0
        try {
            // IS THIS NECESSARY?
            if (this->comm_ptr->get_state() != simgrid::s4u::Activity::State::FINISHED) {
                this->comm_ptr->wait_for(timeout);
            }
        } catch (simgrid::NetworkFailureException &e) {
            if (this->operation_type == S4U_PendingCommunication::OperationType::SENDING) {
                throw ExecutionException(std::make_shared<NetworkError>(
                        NetworkError::OperationType::SENDING, NetworkError::FAILURE, this->commport->s4u_mb->get_name(), ""));
            } else {
                throw ExecutionException(std::make_shared<NetworkError>(
                        NetworkError::OperationType::RECEIVING, NetworkError::FAILURE, this->commport->s4u_mb->get_name(), ""));
            }
        } catch (simgrid::TimeoutException &e) {
            if (this->operation_type == S4U_PendingCommunication::OperationType::SENDING) {
                throw ExecutionException(std::make_shared<NetworkError>(
                        NetworkError::OperationType::SENDING, NetworkError::TIMEOUT, this->commport->s4u_mb->get_name(), ""));
            } else {
                throw ExecutionException(std::make_shared<NetworkError>(
                        NetworkError::OperationType::RECEIVING, NetworkError::TIMEOUT, this->commport->s4u_mb->get_name(), ""));
            }
        }
#ifdef MESSAGE_MANAGER
        MessageManager::removeReceivedMessage(this->commport, this->simulation_message.get());
#endif
        return std::move(this->simulation_message);
#endif
        if (this->operation_type == S4U_PendingCommunication::OperationType::RECEIVING) {

            //        if (log) WRENCH_DEBUG("Getting a message from commport '%s' with timeout %lf sec", this->comm_ptr->get_cname(), timeout);

            simgrid::s4u::ActivitySet pending_receives;
            pending_receives.push(this->comm_ptr);
            pending_receives.push(this->mess_ptr);

            simgrid::s4u::ActivityPtr finished_recv;
            try {
                // Wait for one activity to complete
                finished_recv = pending_receives.wait_any_for(timeout);
            } catch (simgrid::TimeoutException &e) {
                auto failed_recv = pending_receives.get_failed_activity();
                if (failed_recv == comm_ptr) {
                    mess_ptr->cancel();
                } else {
                    comm_ptr->cancel();
                }
                throw ExecutionException(std::make_shared<NetworkError>(NetworkError::RECEIVING, NetworkError::TIMEOUT, this->commport->get_name(), ""));
            } catch (simgrid::Exception &e) {
                auto failed_recv = pending_receives.get_failed_activity();
                if (failed_recv == comm_ptr) {
                    mess_ptr->cancel();
                    throw ExecutionException(std::make_shared<NetworkError>(
                            NetworkError::RECEIVING, NetworkError::FAILURE, this->comm_ptr->get_name(), ""));
                } else {
                    comm_ptr->cancel();
                    throw ExecutionException(std::make_shared<wrench::FatalFailure>("A communication on a MQ should never fail"));
                }
            }

            if (finished_recv == comm_ptr) {
                mess_ptr->cancel();
            } else if (finished_recv == mess_ptr) {
                comm_ptr->cancel();
            }

#ifdef MESSAGE_MANAGER
            MessageManager::removeReceivedMessage(this, msg);
#endif

            WRENCH_DEBUG("Received a '%s' message from commport '%s'", this->simulation_message->getName().c_str(), this->commport->get_cname());

            return std::move(this->simulation_message);
        } else {
            if (this->comm_ptr) {
                try {
                    this->comm_ptr->wait_for(timeout);
                } catch (simgrid::NetworkFailureException &e) {
                        if (this->operation_type == S4U_PendingCommunication::OperationType::SENDING) {
                            throw ExecutionException(std::make_shared<NetworkError>(
                                    NetworkError::OperationType::SENDING, NetworkError::FAILURE, this->commport->s4u_mb->get_name(), ""));
                        } else {
                            throw ExecutionException(std::make_shared<NetworkError>(
                                    NetworkError::OperationType::RECEIVING, NetworkError::FAILURE, this->commport->s4u_mb->get_name(), ""));
                        }
                    } catch (simgrid::TimeoutException &e) {
                        if (this->operation_type == S4U_PendingCommunication::OperationType::SENDING) {
                            throw ExecutionException(std::make_shared<NetworkError>(
                                    NetworkError::OperationType::SENDING, NetworkError::TIMEOUT, this->commport->s4u_mb->get_name(), ""));
                        } else {
                            throw ExecutionException(std::make_shared<NetworkError>(
                                    NetworkError::OperationType::RECEIVING, NetworkError::TIMEOUT, this->commport->s4u_mb->get_name(), ""));
                        }
                    }
            } else if (this->mess_ptr) {
                this->mess_ptr->wait_for(timeout);
            }
            return nullptr;
        }
    }

    /**
     * @brief Wait for any pending communication completion
     * @param pending_comms: a list of pending communications
     * @param timeout: timeout value in seconds (-1 means no timeout)
     *
     * @return the index of the comm to which something happened (success or failure)
     *
     * @throw std::invalid_argument
     */
    unsigned long S4U_PendingCommunication::waitForSomethingToHappen(
            const std::vector<std::shared_ptr<S4U_PendingCommunication>> &pending_comms, double timeout) {
        std::vector<S4U_PendingCommunication *> raw_pointer_comms;
        raw_pointer_comms.reserve(pending_comms.size());
        for (auto const &pc: pending_comms) {
            raw_pointer_comms.push_back(pc.get());
        }
        return S4U_PendingCommunication::waitForSomethingToHappen(raw_pointer_comms, timeout);
    }


    /**
     * @brief Wait for any pending communication completion
     * @param pending_comms: a list of pending communications
     * @param timeout: timeout value in seconds (-1 means no timeout)
     *
     * @return the index of the comm to which something happened (success or failure), or
     *         ULONG_MAX if nothing happened before the timeout expired.
     *
     * @throw std::invalid_argument
     */
    unsigned long S4U_PendingCommunication::waitForSomethingToHappen(
            std::vector<S4U_PendingCommunication *> pending_comms, double timeout) {

        if (pending_comms.empty()) {
            throw std::invalid_argument("S4U_PendingCommunication::waitForSomethingToHappen(): invalid argument");
        }

#if 0
        std::vector<simgrid::s4u::CommPtr> pending_s4u_comms;
        for (auto it = pending_comms.begin(); it < pending_comms.end(); it++) {
            pending_s4u_comms.push_back((*it)->comm_ptr);
        }
#else

        simgrid::s4u::ActivitySet pending_activities;
        for (auto it = pending_comms.begin(); it < pending_comms.end(); it++) {
            if ((*it)->comm_ptr) pending_activities.push((*it)->comm_ptr);
            if ((*it)->mess_ptr) pending_activities.push((*it)->mess_ptr);
        }
#endif

        // Wait one activity to complete
        simgrid::s4u::ActivityPtr finished_activity = nullptr;
        try {
            finished_activity = pending_activities.wait_any_for(timeout);
        } catch (simgrid::TimeoutException &e) {
            for (auto it = pending_comms.begin(); it < pending_comms.end(); it++) {
                if ((*it)->comm_ptr) (*it)->comm_ptr->cancel();
                if ((*it)->mess_ptr) (*it)->mess_ptr->cancel();
            }
            return ULONG_MAX;
        } catch (simgrid::Exception &e) {
            auto failed_activity = pending_activities.get_failed_activity();
            for (unsigned long idx = 0; idx < pending_comms.size(); idx++ ) {
                if (pending_comms.at(idx)->comm_ptr == failed_activity) {
                    return idx;
                }
                if (pending_comms.at(idx)->mess_ptr == failed_activity) {
                    return idx;
                }
            }
        }

        if (finished_activity) {
            for (unsigned long idx = 0; idx < pending_comms.size(); idx++ ) {
                if (pending_comms.at(idx)->comm_ptr == finished_activity) {
                    return idx;
                }
                if (pending_comms.at(idx)->mess_ptr == finished_activity) {
                    return idx;
                }
            }
        } else {
            return -1;
        }

        throw std::runtime_error("S4U_PendingCommunication::waitForSomethingToHappen(): this should not have happened");
    }

}// namespace wrench
