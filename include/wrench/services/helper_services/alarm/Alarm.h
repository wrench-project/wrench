/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_ALARM_H
#define WRENCH_ALARM_H

#include <string>
#include <memory>
#include "wrench/services/Service.h"
#include "wrench/simulation/SimulationMessage.h"

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/


    class Simulation;

    /**
     * @brief A one-shot service that sends a message to a commport_name after some specified amount of time and terminates
     */
    class Alarm : public Service {

        friend class S4U_Daemon;

    public:
        static std::shared_ptr<Alarm> createAndStartAlarm(Simulation *simulation, double date, std::string hostname,
                                                          S4U_CommPort *reply_commport,
                                                          SimulationMessage *msg, std::string suffix);

        void kill();

    private:
        Alarm(double date, std::string hostname, S4U_CommPort *reply_commport,
              SimulationMessage *msg, std::string suffix);

        double date;
        //        std::string reply_commport_name;
        S4U_CommPort *reply_commport;
        std::unique_ptr<SimulationMessage> msg;

        int main() override;
    };

    /***********************/
    /** \endcond           */
    /***********************/

}// namespace wrench

#endif//WRENCH_ALARM_H
