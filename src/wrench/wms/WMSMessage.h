/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_WMSMESSAGE_H
#define WRENCH_WMSMESSAGE_H

#include "simulation/SimulationMessage.h"

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
    * @brief Generic class that describe a message communicated by simulated processes
    */
    class WMSMessage : public SimulationMessage {
    protected:
        WMSMessage(std::string name, double payload);
    };

    /**
     * @brief AlarmJobTimeOutMessage class
     */
    class AlarmWMSDeferredStartMessage : public WMSMessage {
    public:
        AlarmWMSDeferredStartMessage(std::string &answer_mailbox, double start_time, double payload);

        /** @brief The mailbox to answer to */
        std::string answer_mailbox;
        /** @brief The simulation time to start the workflow execution */
        double start_time;
    };

    /***********************/
    /** \endcond           */
    /***********************/
}

#endif //WRENCH_WMSMESSAGE_H
