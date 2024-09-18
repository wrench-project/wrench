/**
 * Copyright (c) 2020. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_CONDORTIMESTAMP_H
#define WRENCH_CONDORTIMESTAMP_H


#include <wrench-dev.h>


namespace wrench {

    class Simulation;
    class WorkflowTask;
    class StorageService;
    class FileLocation;

    class CondorGridStartTimestamp : public SimulationTimestampPair {
    public:
    protected:
    private:
    };

    class CondorGridEndTimestamp : public SimulationTimestampPair {
    public:
    protected:
    private:
    };


}// namespace wrench

#endif//WRENCH_CONDORTIMESTAMP_H
