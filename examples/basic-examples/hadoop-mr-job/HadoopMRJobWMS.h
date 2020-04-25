/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_EXAMPLE_HADOOP_MR_JOB_H
#define WRENCH_EXAMPLE_HADOOP_MR_JOB_H

#include <wrench-dev.h>


namespace wrench {

    class Simulation;

    /**
     *  @brief A Workflow Management System (WMS) implementation (inherits from WMS)
     */
    class HadoopMRJobWMS : public WMS {

    public:
        // Constructor
        HadoopMRJobWMS(
                  const std::string &hostname);

    protected:

    private:
        // main() method of the WMS
        int main() override;

    };
}
#endif //WRENCH_EXAMPLE_HADOOP_MR_JOB_H
