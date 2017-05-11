/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <simgrid_S4U_util/S4U_Simulation.h>
#include <logging/TerminalOutput.h>
#include "simgrid_S4U_util/S4U_Mailbox.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(sequential_task_executor, "Log category for Sequential Task Executor");

namespace wrench {

    /**
     * @brief Constructor, which starts the daemon for the service on a host
     *
     * @param hostname: the name of the host
     * @param callback_mailbox: the callback mailbox to which the sequential
     *        task executor sends back "task done" or "task failed" messages
     */
    SequentialTaskExecutor::SequentialTaskExecutor(std::string hostname, std::string callback_mailbox,
                                                   double task_startup_overhead) :
            S4U_DaemonWithMailbox("sequential_task_executor", "sequential_task_executor") {

      if (task_startup_overhead < 0) {
          throw std::invalid_argument("SequentialTaskExecutor::SequentialTaskExecutor(): task startup overhead must be >0");
      }
      this->hostname = hostname;
      this->callback_mailbox = callback_mailbox;
      this->task_start_up_overhead = task_startup_overhead;

      // Start my daemon on the host
      this->start(this->hostname);
    }

    /**
     * @brief Terminate the sequential task executor
     */
    void SequentialTaskExecutor::stop() {
      // Send a termination message to the daemon's mailbox
      S4U_Mailbox::put(this->mailbox_name, new StopDaemonMessage("", 0.0));
    }

    /**
     * @brief Kill the sequential task executor
     */
    void SequentialTaskExecutor::kill() {
      if (!this->killed) {
        WRENCH_INFO("Killing SequentialTaskExecutor %s on %s", this->process_name.c_str(), this->hostname.c_str());
        this->kill_actor();
      }
    }

    /**
     * @brief Have the sequential task executor a task
     *
     * @param task: a pointer to the task
     *
     * @return 0 on success
     */
    int SequentialTaskExecutor::runTask(WorkflowTask *task) {
      if (task == nullptr) {
        throw std::invalid_argument("SequentialTaskExecutor::runTask(): passed a nullptr task");
      }
      // Send a "run a task" message to the daemon's mailbox
      S4U_Mailbox::put(this->mailbox_name, new RunTaskMessage(task, 0.0));
      return 0;
    };

    /**
     * @brief Main method of the sequential task executor daemon
     *
     * @return 0 on termination
     *
     * @throw std::runtime_error
     */
    int SequentialTaskExecutor::main() {

      TerminalOutput::setThisProcessLoggingColor(WRENCH_LOGGING_COLOR_BLUE);

      WRENCH_INFO("New Sequential Task Executor starting (%s) ", this->mailbox_name.c_str());

      bool keep_going = true;
      while (keep_going) {

        // Wait for a message
        std::unique_ptr<SimulationMessage> message = S4U_Mailbox::get(this->mailbox_name);

        switch (message->type) {

          case SimulationMessage::STOP_DAEMON: {
            std::unique_ptr<StopDaemonMessage> m(static_cast<StopDaemonMessage *>(message.release()));
            if (m->ack_mailbox != "") {
              S4U_Mailbox::put(m->ack_mailbox, new DaemonStoppedMessage(0.0));
            }
            keep_going = false;
            break;
          }

          case SimulationMessage::RUN_TASK: {
            std::unique_ptr<RunTaskMessage> m(static_cast<RunTaskMessage *>(message.release()));

            // Run the task
            WRENCH_INFO("Executing task %s (%lf flops)", m->task->getId().c_str(), m->task->getFlops());
            m->task->setRunning();
            S4U_Simulation::sleep(this->task_start_up_overhead);
            S4U_Simulation::compute(m->task->getFlops());

            // Set the task completion time and state
            m->task->setEndDate(S4U_Simulation::getClock());
            m->task->setCompleted();

            // Send the callback
            WRENCH_INFO("Notifying mailbox %s that task %s has finished",
                        this->callback_mailbox.c_str(),
                        m->task->getId().c_str());
            S4U_Mailbox::dput(this->callback_mailbox,
                              new TaskDoneMessage(m->task, this, 0.0));

            break;
          }

          default: {
            throw std::runtime_error("Unknown message type");
          }
        }
      }

      WRENCH_INFO("Sequential Task Executor Daemon on host %s terminated!", S4U_Simulation::getHostName().c_str());
      this->killed = true;
      return 0;
    }

};
