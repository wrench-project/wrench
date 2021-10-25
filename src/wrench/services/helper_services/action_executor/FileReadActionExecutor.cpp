/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/logging/TerminalOutput.h>

#include <wrench/action/FileReadAction.h>
#include <wrench/services/helper_services/action_executor/ActionExecutor.h>
#include <wrench/services/helper_services/action_executor/FileReadActionExecutor.h>
#include <wrench/failure_causes/HostError.h>

#include <utility>
#include <wrench/services/helper_services/action_executor/ActionExecutorMessage.h>

WRENCH_LOG_CATEGORY(wrench_core_file_read_action_executor,"Log category for File Read Action Executor");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host on which the action will run
     * @param callback_mailbox: the callback mailbox to which a "work done" or "work failed" message will be sent
     * @param action: the action to perform
     */
    FileReadActionExecutor::FileReadActionExecutor(
            std::string hostname,
            std::string callback_mailbox,
            std::shared_ptr<FileReadAction> action) :
            ActionExecutor(std::move(hostname), callback_mailbox, action) {
    }

    /**
    * @brief Main method of the action exedcutor
    *
    * @return 1 if a failure timestamp should be generated, 0 otherwise
    *
    * @throw std::runtime_error
    */
    int FileReadActionExecutor::main() {

        S4U_Simulation::computeZeroFlop(); // to block in case pstate speed is 0

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_BLUE);
        WRENCH_INFO("New FileRead Action Executor starting (%s) to do action %s",
                    this->getName().c_str(), this->action->getName().c_str());

        auto file_read_action = std::dynamic_pointer_cast<FileReadAction>(this->action);
        if (not file_read_action) {
            throw std::runtime_error("FileReadActionExecutor::main(): invalid action");
        }

        SimulationMessage *msg_to_send_back = nullptr;

        try {
            this->action->setStartDate(S4U_Simulation::getClock());
            this->action->setState(Action::State::STARTED);
            StorageService::readFile(file_read_action->getFile().get(), file_read_action->getFileLocation());
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
    void FileReadActionExecutor::kill(bool job_termination) {
        this->killed_on_purpose = job_termination;
        this->killActor();
    }


    /**
     * @brief Cleanup method
     *
     * @param has_returned_from_main: whether main() returned
     * @param return_value: the return value (if main() returned)
     */
    void FileReadActionExecutor::cleanup(bool has_returned_from_main, int return_value) {
        // Just do the basics
        commonCleanup(has_returned_from_main, return_value);
    }

}
