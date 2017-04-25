/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SIMPLEWMS_H
#define WRENCH_SIMPLEWMS_H

#include "wms/WMS.h"

namespace wrench {

    class Simulation; // forward ref

    /**
     *  @brief A simple WMS implementation
     */
    class SimpleWMS : public WMS {

    public:
        SimpleWMS(Simulation *, Workflow *, std::unique_ptr<Scheduler>, std::string);

    private:
        int main();
    };
}
#endif //WRENCH_SIMPLEWMS_H
