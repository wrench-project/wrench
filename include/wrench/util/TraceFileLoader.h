/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_TRACEFILELOADER_H
#define WRENCH_TRACEFILELOADER_H

#include <string>
#include <wrench/workflow/WorkflowTask.h>

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/


    /**
     * @brief A class that can load a job submission trace in the SWF format
     *        and store it as a vector of simulation-relevant fields
     */
    class TraceFileLoader {
    public:
        static std::vector<std::tuple<std::string, double, double, double, double, unsigned int>>
           loadFromTraceFile(std::string filename, double load_time_compensation);
    };

    /***********************/
    /** \endcond           */
    /***********************/

}


#endif //WRENCH_TRACEFILELOADER_H
