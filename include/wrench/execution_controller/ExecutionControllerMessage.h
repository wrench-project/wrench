/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_EXECUTIONCONTROLLERMESSAGE_H
#define WRENCH_EXECUTIONCONTROLLERMESSAGE_H

#include "wrench/simulation/SimulationMessage.h"

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
    * @brief Top-level class for messages received/sent by a ExecutionController
    */
    class ExecutionControllerMessage : public SimulationMessage {
    protected:
        ExecutionControllerMessage(double payload);
    };


    /**
     * @brief Message sent when a timer set by a ExecutionController goes off
     */
    class ExecutionControllerAlarmTimerMessage : public ExecutionControllerMessage {
    public:
        explicit ExecutionControllerAlarmTimerMessage(std::string message, double payload);
        /** @brief The message sent my the timer */
        std::string message;
    };


    /***********************/
    /** \endcond           */
    /***********************/
}// namespace wrench

#endif//WRENCH_EXECUTIONCONTROLLERMESSAGE_H
