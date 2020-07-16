/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_AMDAHLMULTICOREPERFORMANCESPEC_H
#define WRENCH_AMDAHLMULTICOREPERFORMANCESPEC_H

#include "MulticorePerformanceSpec.h"

namespace wrench {

    class AmdahlMulticorePerformanceSpec : public MulticorePerformanceSpec {

    public:
        AmdahlMulticorePerformanceSpec(double alpha);

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

        std::vector<double> getWorkPerThread(unsigned long num_threads) override;

        /***********************/
        /** \endcond          **/
        /***********************/

    private:
        double alpha; // Fraction of the work  that's parallelizable
    };


}

#endif //WRENCH_AMDAHLMULTICOREPERFORMANCESPEC_H
