/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_DYNAMICOPTIMIZATION_H
#define WRENCH_DYNAMICOPTIMIZATION_H


namespace wrench {

    class Workflow;

    /***********************/
    /** \cond DEVELOPER */
    /***********************/

    /**
     *  @brief An abstract dynamic optimization class
     */
    class DynamicOptimization {
    public:
        /**
         * @brief Method to process a workflow at runtime so as to optimize its execution (to be overridden)
         *
         * @param workflow: the workflow
         */
        virtual void process(Workflow *workflow) = 0;
    };

    /***********************/
    /** \endcond           */
    /***********************/
}

#endif //WRENCH_DYNAMICOPTIMIZATION_H
