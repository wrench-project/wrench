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
     *  @brief An abstract class that serves as a base class for implementing dynamic (i.e., at runtime) optimizations
     *         to be used by a WMS while executing a Workflow
     */
    class DynamicOptimization {
    public:
        /**
         * @brief Method to process (i.e., modify the structure of) a workflow at runtime so as to
         * optimize its execution (to be overridden)
         *
         * @param workflow: the workflow
         */
        virtual void process(std::shared_ptr<Workflow> workflow) = 0;

        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        virtual ~DynamicOptimization() = default;
        /***********************/
        /** \endcond           */
        /***********************/
    };

    /***********************/
    /** \endcond           */
    /***********************/
}

#endif //WRENCH_DYNAMICOPTIMIZATION_H
