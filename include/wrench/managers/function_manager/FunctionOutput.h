/**
* Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

 #ifndef WRENCH_FUNCTIONOUTPUT_H
 #define WRENCH_FUNCTIONOUTPUT_H
 
 namespace wrench {

     /***********************/
     /** \cond DEVELOPER    */
     /***********************/

    /**
    * @brief Represents a completely abstract FunctionOutput object
    */
    class FunctionOutput {
    public:
        FunctionOutput();
        virtual ~FunctionOutput() = default;
    };

     /***********************/
     /** \endcond           */
     /***********************/
     
 } // namespace wrench
 
 #endif // WRENCH_FUNCTIONOUTPUT_H
 