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

namespace wrench {

    class SimulationMessage;

    class Alarm {
        
    public:
        Alarm(double date, std::unique_ptr<SimulationMessage> msg, std::string mailbox_name);

    };

};


#endif //WRENCH_ALARM_H
