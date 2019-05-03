/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/logging/TerminalOutput.h"
#include "wrench/services/ServiceMessage.h"
#include "wrench/services/helpers/Alarm.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "wrench/workflow/execution_events/FailureCause.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(alarm_service, "Log category for Alarm Service");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param date: the date at which the message should be sent. If date is in the past
     *              message will be sent immediately.
     * @param hostname: the name of the host on which the Alarm daemon should run
     * @param reply_mailbox_name: the mailbox to which the message should be sent
     * @param msg: the message to send
     * @param suffix: a (possibly empty) suffix to append to the daemon name
     */
    Alarm::Alarm(double date, std::string hostname, std::string &reply_mailbox_name,
                 SimulationMessage *msg, std::string suffix) : Service(hostname, "alarm_service_" + suffix,
                                                                       "alarm_service_" + suffix) {

      this->date = date;
//      if (this->date <= S4U_Simulation::getClock()) {
//        WRENCH_INFO(
//                "Alarm is being started but notification date is in the past. Notification will be sent immediately.");
//      }
      this->reply_mailbox_name = reply_mailbox_name;
      this->msg = std::shared_ptr<SimulationMessage>(msg);
    }

    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int Alarm::main() {
      TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_MAGENTA);
      WRENCH_INFO("Alarm Service starting");

      double time_to_sleep = this->date - S4U_Simulation::getClock();

      if (time_to_sleep > 0) {
        S4U_Simulation::sleep(time_to_sleep);
        WRENCH_INFO("Alarm Service Sending a message to %s", this->reply_mailbox_name.c_str());
        try {
          S4U_Mailbox::putMessage(this->reply_mailbox_name,
                                  msg.get());
        } catch (std::shared_ptr<NetworkError> &cause) {
          WRENCH_WARN("AlarmService was not able to send the trigger to its upper service");
        }
      }

      this->setStateToDown();
      return 0;
    }


    /**
     * @brief Create and start an alarm service
     * @param simulation: a pointer to the simulation object
     * @param date: the date at which the message should be sent (if date is in the past
     *              then the message will be sent immediately)
     * @param hostname: the name of the host on which to start the alarm service
     * @param reply_mailbox_name: the mailbox to which the alarm service will send a message
     * @param msg: the message to send
     * @param suffix: a (possibly empty) suffix to append to the daemon name (useful in debug output)
     * @return a shared_ptr reference to the alarm service
     *
     * @throw std::invalid_argument
     */
    std::shared_ptr<Alarm>
    Alarm::createAndStartAlarm(Simulation *simulation, double date, std::string hostname, std::string &reply_mailbox_name,
                               SimulationMessage *msg, std::string suffix) {
      std::shared_ptr<Alarm> alarm_ptr = std::shared_ptr<Alarm>(
              new Alarm(date, hostname, reply_mailbox_name, msg, suffix));
      alarm_ptr->simulation = simulation;
      try {
        alarm_ptr->start(alarm_ptr, true, false); // Daemonized, no auto-restart
      } catch (std::invalid_argument &e) {
        throw;
      }
      return alarm_ptr;
    }

    /**
     * @brief Immediately (i.e., brutally) terminate the alarm service
     */
    void Alarm::kill() {
      this->killActor();
    }

};
