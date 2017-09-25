//
// Created by suraj on 9/16/17.
//

#ifndef WRENCH_BATCHJOB_H
#define WRENCH_BATCHJOB_H

#include "wrench/workflow/job/StandardJob.h"

namespace wrench {
    class BatchJob {
    public:
        //job, jobid, -t, -N, -c, ending s4u_timestamp (-1 as undetermined)
        BatchJob(WorkflowJob* job, unsigned long jobid, unsigned long time_in_minutes, unsigned long number_nodes,
                 unsigned long cores_per_node,double ending_time_stamp);


        unsigned long getJobID();
        unsigned long getAllocatedTime();
        unsigned long getAllocatedCoresPerNode();
        double getEndingTimeStamp();
        unsigned long getNumNodes();
        WorkflowJob* getWorkflowJob();
        void setEndingTimeStamp(double);
        std::set<std::pair<std::string,unsigned long>> getResourcesAllocated();
        void setAllocatedResources(std::set<std::pair<std::string,unsigned long>>);

    private:
        unsigned long jobid;
        unsigned long time_in_minutes;
        WorkflowJob* job;
        unsigned long num_nodes;
        unsigned long cores_per_node;
        double ending_time_stamp;
        std::set<std::pair<std::string,unsigned long>> resources_allocated;
    };
}


#endif //WRENCH_BATCHJOB_H
