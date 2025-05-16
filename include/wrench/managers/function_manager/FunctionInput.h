/**
* Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_FUNCTIONINPUT_H
#define WRENCH_FUNCTIONINPUT_H

namespace wrench {

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
     * @brief Represents a completely abstract FunctionInput object
     */
    class FunctionInput {
    public:

        FunctionInput();

        virtual ~FunctionInput() = default;
    };

    /***********************/
    /** \endcond           */
    /***********************/
    
} // namespace wrench

#endif // WRENCH_FUNCTION_INPUT_H
