/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/simgrid_S4U_util/S4U_Simulation.h>
#include <wrench-dev.h>
#include <services/ServiceMessage.h>
#include "wrench/services/helpers/Alarm.h"
#include <simgrid_S4U_util/S4U_Mailbox.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(alarm_service, "Log category for Alarm Service");

namespace wrench {

    Alarm::~Alarm() {
        WRENCH_INFO("Alarm::~Alarm()::In the destructor of Alarm");
    }

    /**
     * @brief Constructor
     * @param date: the date at this the message should be sent
     * @param hostname: the name of the host on which the Alarm daemon should run
     * @param reply_mailbox_name: the mailbox to which the message should be sent
     * @param msg: the message to send
     * @param suffix: a (possibly empty) suffix to append to the daemon name
     */
    Alarm::Alarm(double date, std::string hostname, std::string reply_mailbox_name,
                 SimulationMessage* msg, std::string suffix):Service("alarm_service_"+suffix,"alarm_service_"+suffix) {

        //it would be helpful to debug which service this alarm is for if we pass <compute_service_name> in the suffix=
        this->date = date;
        if(this->date<=S4U_Simulation::getClock()){
            WRENCH_INFO("Alarm is being started but the date to notify is less than current timestamp. will be notified immediately");
        }
        this->reply_mailbox_name = reply_mailbox_name;
        this->msg = msg;
        // Start the daemon on the same host
        try {
            WRENCH_INFO("Alarm Service starting...");
            this->start(hostname,true);
        } catch (std::invalid_argument e) {
            throw e;
        }
    }

    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int Alarm::main() {
        TerminalOutput::setThisProcessLoggingColor(WRENCH_LOGGING_COLOR_MAGENTA);
        WRENCH_INFO("Alarm Service starting on host %s!", S4U_Simulation::getHostName().c_str());

        double time_to_sleep = this->date-S4U_Simulation::getClock();

        if(time_to_sleep>0) {
            S4U_Simulation::sleep(time_to_sleep);
                try {
                    S4U_Mailbox::putMessage(this->reply_mailbox_name,
                                            msg);
                } catch (std::shared_ptr<NetworkError> cause) {
                    WRENCH_WARN("AlarmService was not able to send the trigger to its upper service");
                }
        }

        WRENCH_INFO("Alarm Service on host %s terminated!", S4U_Simulation::getHostName().c_str());
        this->setStateToDown();
        return 0;
    }

    /**
     * @brief Brutally kill the daemon
     *
     */
    void Alarm::kill() {
        //kill itself
        this->kill_actor();
    }

};