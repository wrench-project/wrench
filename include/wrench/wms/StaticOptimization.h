/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_STATICOPTIMIZATION_H
#define WRENCH_STATICOPTIMIZATION_H

namespace wrench {

    class Workflow;

    /***********************/
    /** \cond DEVELOPER */
    /***********************/

    /**
     *  @brief An abstract static optimization class
     */
    class StaticOptimization {
    public:
        /**
         * @brief Method to pre-process a workflow so as to optimize its execution (to be overridden)
         *
         * @param workflow: the workflow
         */
        virtual void process(Workflow *workflow) = 0;
    };

    /***********************/
    /** \endcond           */
    /***********************/
}

#endif //WRENCH_STATICOPTIMIZATION_H
