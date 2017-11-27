//
// Created by suraj on 9/16/17.
//

#ifndef WRENCH_BATCHSCHEDULER_H
#define WRENCH_BATCHSCHEDULER_H


#include "BatchJob.h"
#include "BatchService.h"

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief An implementation of a batch scheduling algorithm to be used in a BatchService
     */
    class BatchScheduler {
    public:
        BatchScheduler();
        std::set<std::pair<std::string,unsigned long>> schedule_on_hosts(BatchJob *job);
        BatchJob* schedule_job();
    };
    /***********************/
    /** \endcond           */
    /***********************/
}


#endif //WRENCH_BATCHSCHEDULER_H
