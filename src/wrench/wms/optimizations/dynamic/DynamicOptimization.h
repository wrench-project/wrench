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

#include "workflow/Workflow.h"

namespace wrench {

    /***********************/
    /** \cond DEVELOPER */
    /***********************/

    /**
     *  @brief An abstract static optimizastion classe
     */
    class DynamicOptimization {
    public:
        virtual void process(Workflow *workflow) = 0;
    };

    /***********************/
    /** \endcond           */
    /***********************/
}

#endif //WRENCH_DYNAMICOPTIMIZATION_H
