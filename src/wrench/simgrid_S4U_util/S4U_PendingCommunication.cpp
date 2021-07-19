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
#include <wrench/workflow/failure_causes/NetworkError.h>

#ifdef MESSAGE_MANAGER
#include <wrench/util/MessageManager.h>
#endif

#include "wrench/logging/TerminalOutput.h"
#include "wrench/simgrid_S4U_util/S4U_PendingCommunication.h"
#include "wrench/simulation/SimulationMessage.h"
#include "wrench/workflow/failure_causes/FailureCause.h"

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

        try {
            if (this->comm_ptr->get_state() != simgrid::s4u::Activity::State::FINISHED) {
                this->comm_ptr->wait();
            }
        } catch (simgrid::NetworkFailureException &e) {
            if (this->operation_type == S4U_PendingCommunication::OperationType::SENDING) {
                throw std::shared_ptr<NetworkError>(
                        new NetworkError(NetworkError::OperationType::SENDING, NetworkError::FAILURE, mailbox_name));
            } else {
                throw std::shared_ptr<NetworkError>(
                        new NetworkError(NetworkError::OperationType::RECEIVING, NetworkError::FAILURE, mailbox_name));
            }
        }
        return std::move(this->simulation_message);
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
            std::vector<std::shared_ptr<S4U_PendingCommunication>> pending_comms, double timeout) {
        std::vector<S4U_PendingCommunication *> raw_pointer_comms;
        for (auto const &pc : pending_comms) {
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
     *         ULONG_MAX if nothing happened before the timeout expired
     *
     * @throw std::invalid_argument
     */
    unsigned long S4U_PendingCommunication::waitForSomethingToHappen(
            std::vector<S4U_PendingCommunication *> pending_comms, double timeout) {

        if (pending_comms.empty()) {
            throw std::invalid_argument("S4U_PendingCommunication::waitForSomethingToHappen(): invalid argument");
        }

        std::vector<simgrid::s4u::CommPtr> pending_s4u_comms;
        for (auto it = pending_comms.begin(); it < pending_comms.end(); it++) {
            pending_s4u_comms.push_back((*it)->comm_ptr);
        }

        int index = 0;
        bool one_comm_failed = false;
        try {
            WRENCH_INFO("Calling wait_any_for");
            index =  simgrid::s4u::Comm::wait_any_for(&pending_s4u_comms, timeout);
            WRENCH_INFO("wait_any_for() returned: %d", index);
#ifdef MESSAGE_MANAGER
            MessageManager::removeReceivedMessage(pending_comms[index]->mailbox_name, pending_comms[index]->simulation_message.get());
#endif
        } catch (simgrid::NetworkFailureException &e) {
            one_comm_failed = true;
            WRENCH_INFO("wait_any_for() in Handle for NetworkFailureException");
        } catch (simgrid::TimeoutException &e) {
            // This likely doesn't happen, but let's keep it here for now
            WRENCH_INFO("wait_any_for() in Hanlde for TimeoutException");
            one_comm_failed = true;
        }


        if (index == -1) {
            return ULONG_MAX;
        }

        if (one_comm_failed) {
            WRENCH_INFO("SINCE ONE COMM FAILED, LET'S FIND OUT WHICH ONE");
            for (index = 0; index < (int) pending_s4u_comms.size(); index++) {
                try {
                    WRENCH_INFO("CALLING TEST()");
                    WRENCH_INFO("STATE = %d", pending_s4u_comms[index]->get_state());
                    auto fuck = pending_s4u_comms[index]->test();
                    WRENCH_INFO("DIDN'T GET AN EXCEPTION, but test sent back: %d", fuck);
                } catch (simgrid::NetworkFailureException &e) {
                    break;
                } catch (simgrid::TimeoutException &e) {
                    break;
                }
            }
        }

        return (unsigned long)index;
    }

};
