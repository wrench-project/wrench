/**
 * Copyright (c) 2017. The WRENCH Team.
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
#include <wrench/services/Service.h>
#include <simulation/SimulationMessage.h>

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    class Alarm: public Service {

        friend class S4U_Daemon;

    public:
        Alarm(double date, std::string hostname, std::string reply_mailbox_name,
              std::shared_ptr<SimulationMessage> msg, std::string suffix);

            private:

        double date;
        std::string reply_mailbox_name;
        std::shared_ptr<SimulationMessage> msg;

        int main() override;

    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_ALARM_H
