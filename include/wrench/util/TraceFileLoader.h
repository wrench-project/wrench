//
// Created by Suraj Pandey on 12/26/17.
//

#ifndef WRENCH_TRACEFILELOADER_H
#define WRENCH_TRACEFILELOADER_H

#include <string>
#include <wrench/workflow/WorkflowTask.h>

namespace wrench {

    class TraceFileLoader {
    public:
        static std::vector<std::pair<double,std::tuple<std::string, double, int, int, double, int>>> loadFromTraceFile(std::string filename,double load_time_compensation);
    };
}


#endif //WRENCH_TRACEFILELOADER_H
