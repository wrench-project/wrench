/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_COMPUTETHREADMESSAGE_H
#define WRENCH_COMPUTETHREADMESSAGE_H


#include "wrench/simulation/SimulationMessage.h"
#include "wrench-dev.h"

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    class ActionExecutor;

    /**
     * @brief Top-level class for messages received/sent by an ComputeThread
     */
    class ComputeThreadMessage : public SimulationMessage {
    protected:
        explicit ComputeThreadMessage();
    };

    /**
     * @brief A message sent by a ComputeTherad when it's successfully completed
     */
    class ComputeThreadDoneMessage : public ComputeThreadMessage {
    public:
        explicit ComputeThreadDoneMessage();
    };

    /***********************/
    /** \endcond           */
    /***********************/

};

#endif //WRENCH_COMPUTETHREADMESSAGE_H
