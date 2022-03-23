/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_PARALLELMODEL_H
#define WRENCH_PARALLELMODEL_H

#include <vector>
#include <memory>
#include <functional>

namespace wrench {

    /**
     * @brief A virtual class (with convenient static methods) to define 
     *        parallel task performance models
     */
    class ParallelModel {

    public:
        static std::shared_ptr<ParallelModel> AMDAHL(double alpha);
        static std::shared_ptr<ParallelModel> CONSTANTEFFICIENCY(double efficiency);
        static std::shared_ptr<ParallelModel> CUSTOM(std::function<double(double, long)> lambda_sequential, std::function<double(double, long)> lambda_per_thread);

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

        /**
         * @brief A method the, for this parallel model, computes how much work each thread that is
         * part of a parallel task should do
         *
         * @param total_work: the total amount of work (in flops)
         * @param num_threads: the number of threads
         *
         * @return an amount of work (in flop) per thread
         */
        //        virtual std::vector<double> getWorkPerThread(double total_work, unsigned long num_threads) = 0;
        virtual double getPurelySequentialWork(double total_work, unsigned long num_threads) = 0;
        virtual double getParallelPerThreadWork(double total_work, unsigned long num_threads) = 0;

        virtual ~ParallelModel() = default;

        /***********************/
        /** \endcond          **/
        /***********************/

    private:
    };

}// namespace wrench

#endif//WRENCH_PARALLELMODEL_H
