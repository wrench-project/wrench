/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_CUSTOMPARALLELMODEL_H
#define WRENCH_CUSTOMPARALLELMODEL_H

#include "wrench/workflow/parallel_model/ParallelModel.h"

#include <vector>
#include <functional>

namespace wrench {

    /**
     * @brief A class that defines a custom parallel task performance model.
     */
    class CustomParallelModel : public ParallelModel {

    public:
        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

        //        std::vector<double> getWorkPerThread(double total_work, unsigned long num_threads) override;
        double getPurelySequentialWork(double total_work, unsigned long num_threads) override;
        double getParallelPerThreadWork(double total_work, unsigned long num_threads) override;
        ~CustomParallelModel() override {}

        /***********************/
        /** \endcond          **/
        /***********************/

    protected:
    private:
        friend class ParallelModel;

        CustomParallelModel(std::function<double(double, unsigned long)> lambda_sequential, std::function<double(double, unsigned long)> lambda_per_thread);

        std::function<double(double, unsigned long)> lambda_sequential;
        std::function<double(double, unsigned long)> lambda_per_thread;
    };


}// namespace wrench

#endif//WRENCH_CUSTOMPARALLELMODEL_H
