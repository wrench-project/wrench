/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_MULTINODEMULTICORESTANDARDJOBEXECUTORPROPERTY_H
#define WRENCH_MULTINODEMULTICORESTANDARDJOBEXECUTORPROPERTY_H


#include <services/ServiceProperty.h>

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    class StandardJobExecutorProperty {

    public:

        /** @brief The number of seconds to start a work unit executor **/
        DECLARE_PROPERTY_NAME(THREAD_STARTUP_OVERHEAD);
        DECLARE_PROPERTY_NAME(STANDARD_JOB_DONE_MESSAGE_PAYLOAD);
        DECLARE_PROPERTY_NAME(STANDARD_JOB_FAILED_MESSAGE_PAYLOAD);
    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_MULTINODEMULTICORESTANDARDJOBEXECUTORPROPERTY_H
