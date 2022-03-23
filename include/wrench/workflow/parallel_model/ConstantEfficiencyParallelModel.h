/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_CONSTANTEFFICIENCYPARALLELMODEL_H
#define WRENCH_CONSTANTEFFICIENCYPARALLELMODEL_H

#include "wrench/workflow/parallel_model/ParallelModel.h"

#include <vector>

namespace wrench {

    /**
     * @brief A class the implemens a constant-efficiency parallel task performance model
     */
    class ConstantEfficiencyParallelModel : public ParallelModel {

    public:
        double getEfficiency();

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

        //        std::vector<double> getWorkPerThread(double total_work, unsigned long num_threads) override;
        double getPurelySequentialWork(double total_work, unsigned long num_threads) override;
        double getParallelPerThreadWork(double total_work, unsigned long num_threads) override;
        ~ConstantEfficiencyParallelModel() override {}

        /***********************/
        /** \endcond          **/
        /***********************/

    private:
        friend class ParallelModel;

        ConstantEfficiencyParallelModel(double efficiency);
        double efficiency;// Parallel efficiency
    };


}// namespace wrench

#endif//WRENCH_CONSTANTEFFICIENCYPARALLELMODEL_H
