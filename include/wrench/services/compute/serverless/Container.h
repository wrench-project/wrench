/**
 * Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef CONTAINER_H
#define CONTAINER_H

#include <memory>
#include <wrench/managers/function_manager/Function.h>

namespace wrench {

    /***********************/
    /** \cond INTERNAL    **/
    /***********************/

    /**
     * @class Container
     * @brief Represents a container in a serverless platform.
     */
    class Container {

      enum class ContainerState {
            STARTING,
            IDLE,
            BUSY
        };

    public:

        explicit Container(const std::shared_ptr<Function> &registered_function) {
            _registered_function = registered_function;
            _state = ContainerState::STARTING;
         }

    private:
        std::shared_ptr<Function> _registered_function;
        ContainerState _state;

    /***********************/
    /** \endcond          **/
    /***********************/

    };
}

#endif //CONTAINER_H
