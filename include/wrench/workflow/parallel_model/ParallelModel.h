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

    class ParallelModel {

    public:

        static std::shared_ptr<ParallelModel> AMDAHL(double alpha);
        static std::shared_ptr<ParallelModel> CONSTANTEFFICIENCY(double efficiency);
        static std::shared_ptr<ParallelModel> CUSTOM(std::function<std::vector<double>(double, long)> lambda);

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

        virtual std::vector<double> getWorkPerThread(double total_work, unsigned long num_threads) = 0;
        virtual ~ParallelModel() {};

        /***********************/
        /** \endcond          **/
        /***********************/

    private:

    };

}

#endif //WRENCH_PARALLELMODEL_H
