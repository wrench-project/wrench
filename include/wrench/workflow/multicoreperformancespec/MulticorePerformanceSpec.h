/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_MULTICOREPERFORMANCESPEC_H
#define WRENCH_MULTICOREPERFORMANCESPEC_H

#include <vector>

namespace wrench {

    class WorkflowTask;

    class MulticorePerformanceSpec {

    public:

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

        virtual std::vector<double> getWorkPerThread(double total_work, unsigned long num_threads) = 0;

        /***********************/
        /** \endcond          **/
        /***********************/

    protected:
        friend class WorkflowTask;
        WorkflowTask *task = nullptr;
    };

}

#endif //WRENCH_MULTICOREPERFORMANCESPEC_H
