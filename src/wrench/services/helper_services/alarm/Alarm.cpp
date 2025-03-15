/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/logging/TerminalOutput.h>
#include <wrench/services/ServiceMessage.h>
#include <wrench/services/helper_services/alarm/Alarm.h>
#include <wrench/simgrid_S4U_util/S4U_CommPort.h>
#include <wrench/simgrid_S4U_util/S4U_Simulation.h>
#include <wrench/failure_causes/NetworkError.h>
#include <wrench/failure_causes/HostError.h>

#include <utility>
#include "wrench/exceptions/ExecutionException.h"

WRENCH_LOG_CATEGORY(wrench_core_alarm_service, "Log category for Alarm Service");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param date: the date at which the message should be sent. If date is in the past
     *              message will be sent immediately.
     * @param hostname: the name of the host on which the Alarm daemon should run
     * @param reply_commport: the commport to which the message should be sent
     * @param msg: the message to send
     * @param suffix: a (possibly empty) suffix to append to the daemon name
     */
    Alarm::Alarm(double date, const std::string& hostname, S4U_CommPort *reply_commport,
                 SimulationMessage *msg, const std::string& suffix) : Service(hostname, "alarm_service_" + suffix) {
        this->date = date;
        this->reply_commport = reply_commport;
        // Make it unique so that there will be no memory leak in case the
        // alarm never goes off
        this->msg = std::unique_ptr<SimulationMessage>(msg);
    }

    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int Alarm::main() {
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_MAGENTA);
        WRENCH_INFO("Alarm Service starting with alarm date = %.20f", this->date);

        double time_to_sleep = this->date - S4U_Simulation::getClock();

        if (time_to_sleep > 0) {
            S4U_Simulation::sleep(time_to_sleep);
        }

        WRENCH_INFO("Alarm Service Sending a message to %s", this->reply_commport->get_cname());
        try {
            auto to_send = this->msg.release();
            this->reply_commport->putMessage(to_send);
        } catch (ExecutionException &e) {
            WRENCH_WARN("AlarmService was not able to send its message");
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
     * @param reply_commport: the commport to which the alarm service will send a message
     * @param msg: the message to send
     * @param suffix: a (possibly empty) suffix to append to the daemon name (useful in debug output)
     * @return a shared_ptr reference to the alarm service
     *
     */
    std::shared_ptr<Alarm>
    Alarm::createAndStartAlarm(Simulation *simulation, double date, const std::string& hostname,
                               S4U_CommPort *reply_commport,
                               SimulationMessage *msg, const std::string& suffix) {
        auto alarm_ptr = std::shared_ptr<Alarm>(
                new Alarm(date, hostname, reply_commport, msg, suffix));
        alarm_ptr->simulation_ = simulation;
        alarm_ptr->start(alarm_ptr, true, false);// Daemonized, no auto-restart
        return alarm_ptr;
    }

    /**
     * @brief Immediately (i.e., brutally) terminate the alarm service
     */
    void Alarm::kill() {
        this->killActor();
    }

}// namespace wrench
