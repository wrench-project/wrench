/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/helper_services/compute_thread/ComputeThreadMessage.h>

#include <utility>

namespace wrench {

    /**
     * @brief Constructor
     *
     */
    ComputeThreadMessage::ComputeThreadMessage() :
            SimulationMessage( 0) {
    }

    /**
     * @brief Constructor
     *
     */
    ComputeThreadDoneMessage::ComputeThreadDoneMessage() :
            ComputeThreadMessage() {
    }

}
