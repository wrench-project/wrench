/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_COMPUTEACTION_H
#define WRENCH_COMPUTEACTION_H

#include <string>

#include "wrench/action/Action.h"

namespace wrench {

    class ComputeAction : public Action {

    public:
        double getFlops() const;
        unsigned long getMinNumCores() const;
        unsigned long getMaxNumCores() const;
        double getRAM() const;
        std::shared_ptr<ParallelModel> getParallelModel() const;

    protected:
        friend class CompoundJob;

        ComputeAction(std::string name,
                      std::shared_ptr<CompoundJob> job,
                      double flops,
                      unsigned long min_core,
                      unsigned long max_core,
                      double ram,
                      std::shared_ptr<ParallelModel> parallel_model);


    private:
        double flops;
        unsigned long min_num_cores;
        unsigned long max_num_cores;
        double ram;
        std::shared_ptr<ParallelModel> parallel_model;


    };
}


#endif //WRENCH_COMPUTEACTION_H