/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SIMPLETASKCLUSTERING_H
#define WRENCH_SIMPLETASKCLUSTERING_H

#include <wrench-dev.h>

namespace wrench {


    /**
     *  @brief A simple task clustering algorithm to group tasks in a pipeline
     */
    class SimplePipelineClustering : public StaticOptimization {
    public:

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/
        void process(Workflow *workflow);

    private:
        WorkflowTask *getTask(std::vector<WorkflowTask *> tasks);
        /***********************/
        /** \endcond           */
        /***********************/
    };

}

#endif //WRENCH_SIMPLETASKCLUSTERING_H
