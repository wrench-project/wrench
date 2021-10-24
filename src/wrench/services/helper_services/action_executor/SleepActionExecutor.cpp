/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/logging/TerminalOutput.h"

#include <wrench/action/SleepAction.h>
#include <wrench/services/helper_services/action_executor/ActionExecutor.h>
#include <wrench/services/helper_services/action_executor/SleepActionExecutor.h>
#include <wrench/failure_causes/HostError.h>

#include <utility>
#include "wrench/services/helper_services/action_executor/ActionExecutorMessage.h"

WRENCH_LOG_CATEGORY(wrench_core_sleep_action_executor,"Log category for Sleep Action Executor");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host on which the action will run
     * @param callback_mailbox: the callback mailbox to which a "work done" or "work failed" message will be sent
     * @param action: the action to perform
     */
    SleepActionExecutor::SleepActionExecutor(
            std::string hostname,
            std::string callback_mailbox,
            std::shared_ptr<SleepAction> action) :
            ActionExecutor(std::move(hostname), callback_mailbox, action) {
    }

    /**
     * @brief Kill the executor
     *
     * @param job_termination: if the reason for being killed is that the job was terminated by the submitter
     * (as opposed to being terminated because the above service was also terminated).
     */
    void ActionExecutor::kill(bool job_termination) {
        this->acquireDaemonLock();
        this->killActor();
        this->releaseDaemonLock();
    }

    /**
    * @brief Main method of the action exedcutor
    *
    * @return 1 if a failure timestamp should be generated, 0 otherwise
    *
    * @throw std::runtime_error
    */
    int SleepActionExecutor::main() {

        S4U_Simulation::computeZeroFlop(); // to block in case pstate speed is 0

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_BLUE);
        WRENCH_INFO("New Sleep Action Executor starting (%s) to do action %s",
                    this->getName().c_str(), this->action->getName().c_str());

        auto sleep_action = std::dynamic_pointer_cast<SleepAction>(this->action);
        if (not sleep_action) {
            throw std::runtime_error("SleepActionExecutor::main(): invalid action");
        }

        SimulationMessage *msg_to_send_back = nullptr;

        try {
            this->action->setStartDate(S4U_Simulation::getClock());
            this->action->setState(Action::State::STARTED);
            S4U_Simulation::sleep(sleep_action->getSleepTime());
            this->action->setEndDate(S4U_Simulation::getClock());
            this->action->setState(Action::State::COMPLETED);

            // build "success!" message
            msg_to_send_back = new ActionExecutorDoneMessage(
                    this->getSharedPtr<ActionExecutor>());

        } catch (ExecutionException &e) {
            // build "failed!" message
            WRENCH_DEBUG("Got an exception while performing action %s: %s",
                         this->action->getName().c_str(),
                         e.getCause()->toString().c_str());
            this->action->setState(Action::State::FAILED);
            this->action->setFailureCause(e.getCause());
            msg_to_send_back = new ActionExecutorDoneMessage(this->getSharedPtr<ActionExecutor>());
        }

        WRENCH_INFO("Action executor for action %s terminating!", this->action->getName().c_str());

        try {
            S4U_Mailbox::putMessage(this->callback_mailbox, msg_to_send_back);
        } catch (std::shared_ptr <NetworkError> &cause) {
            WRENCH_INFO("Action executor can't report back due to network error.. oh well!");
        }

        return 0;
    }

    /**
     * @brief Kill the executor
     *
     * @param job_termination: if the reason for being killed is that the job was terminated by the submitter
     * (as opposed to being terminated because the above service was also terminated).
     */
    void SleepActionExecutor::kill(bool job_termination) {
        this->acquireDaemonLock();
        this->killed_on_purpose = job_termination;
        this->killActor();
        this->releaseDaemonLock();
    }


    /**
     * @brief Cleanup method
     *
     * @param has_returned_from_main: whether main() returned
     * @param return_value: the return value (if main() returned)
     */
    void SleepActionExecutor::cleanup(bool has_returned_from_main, int return_value) {
        WRENCH_DEBUG(
                "In on_exit.cleanup(): SleepActionExecutor: %s has_returned_from_main = %d (return_value = %d, killed_on_pupose = %d)",
                this->getName().c_str(), has_returned_from_main, return_value,
                this->killed_on_purpose);

        // Handle brutal failure or termination
        if (not has_returned_from_main and this->action->getState() == Action::State::STARTED) {
            this->action->setEndDate(S4U_Simulation::getClock());
            if (this->killed_on_purpose) {
                this->action->setState(Action::State::KILLED);
            } else {
                this->action->setState(Action::State::FAILED);
                // If no failure cause was set, then it's a host failure
                if (not this->action->getFailureCause()) {
                    this->action->setFailureCause(
                            std::shared_ptr<HostError>(new HostError(this->hostname)));
                }
            }
        }
    }

}
