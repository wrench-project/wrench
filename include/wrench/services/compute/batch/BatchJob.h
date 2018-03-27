//
// Created by suraj on 9/16/17.
//

#ifndef WRENCH_BATCHJOB_H
#define WRENCH_BATCHJOB_H

#include "wrench/workflow/job/StandardJob.h"

namespace wrench {

    /***********************/
    /** \cond INTERNAL    */
    /***********************/

    /**
     * @brief A batch job, which encapsulates a WorkflowJob and additional information
     *        used be a BatchService
     */
    class BatchJob {
    public:
        //job, jobid, -t, -N, -c, ending s4u_timestamp (-1 as undetermined)
        BatchJob(WorkflowJob* job, unsigned long jobid, unsigned long time_in_minutes, unsigned long number_nodes,
                 unsigned long cores_per_node,double ending_time_stamp, double appeared_time_stamp);


        unsigned long getJobID();
        double getAllocatedTime();
        void setAllocatedTime(double);
        unsigned long getAllocatedCoresPerNode();
        double getMemoryRequirement();
        double getEndingTimeStamp();
        double getAppearedTimeStamp();
        unsigned long getNumNodes();
        WorkflowJob* getWorkflowJob();
        void setEndingTimeStamp(double);
        std::set<std::tuple<std::string,unsigned long, double>> getResourcesAllocated();
        void setAllocatedResources(std::set<std::tuple<std::string,unsigned long, double>>);

    private:
        unsigned long jobid;
        double  allocated_time;
        WorkflowJob* job;
        unsigned long num_nodes;
        unsigned long cores_per_node;
        double ending_time_stamp;
        double appeared_time_stamp;
        std::set<std::tuple<std::string,unsigned long, double>> resources_allocated;
    };

    /***********************/
    /** \endcond           */
    /***********************/

}




#endif //WRENCH_BATCHJOB_H
