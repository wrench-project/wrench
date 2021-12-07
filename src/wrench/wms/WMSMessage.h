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

#include "wrench/simulation/SimulationMessage.h"

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
    * @brief Top-level class for messages received/sent by a WMS
    */
    class WMSMessage : public SimulationMessage {
    protected:
        WMSMessage(std::string name, double payload);
    };

    /**
     * @brief Message sent by an alarm to a WMS to tell it that it can start
     *        executing its workflow
     */
    class AlarmWMSDeferredStartMessage : public WMSMessage {
    public:
        explicit AlarmWMSDeferredStartMessage(double payload);

    };

    /***********************/
    /** \endcond           */
    /***********************/
}

#endif //WRENCH_WMSMESSAGE_H
