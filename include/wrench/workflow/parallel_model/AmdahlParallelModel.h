/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_AMDAHLPARALLELMODEL_H
#define WRENCH_AMDAHLPARALLELMODEL_H

#include "wrench/workflow/parallel_model/ParallelModel.h"

#include <vector>

namespace wrench {

    /**
     * @brief A class that defines an Amdahl's Law-based parallel task1 performance model.
     */
    class AmdahlParallelModel : public ParallelModel {

    public:

        double getAlpha();

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

        std::vector<double> getWorkPerThread(double total_work, unsigned long num_threads) override;
        ~AmdahlParallelModel() override {}

        /***********************/
        /** \endcond          **/
        /***********************/

    protected:

    private:
        friend class ParallelModel;

        AmdahlParallelModel(double alpha);
        double alpha; // Fraction of the work that's parallelizable
    };


}

#endif //WRENCH_AMDAHLPARALLELMODEL_H
