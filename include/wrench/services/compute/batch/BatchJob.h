#ifndef WRENCH_BATCHJOB_H
#define WRENCH_BATCHJOB_H

#include "wrench/workflow/job/StandardJob.h"

namespace wrench {

    /***********************/
    /** \cond INTERNAL    */
    /***********************/

    /**
     * @brief A batch job, which encapsulates a WorkflowJob and additional information
     *        used by a BatchService
     */
    class BatchJob {
    public:
        //job, jobid, -t, -N, -c, ending s4u_timestamp (-1 as undetermined)
        BatchJob(std::shared_ptr<WorkflowJob> job, unsigned long job_id, unsigned long time_in_minutes, unsigned long number_nodes,
                 unsigned long cores_per_node, std::string username, double ending_time_stamp, double arrival_time_stamp);


        unsigned long getJobID();
        unsigned long getRequestedTime();
        void setRequestedTime(unsigned long time);
        unsigned long getRequestedCoresPerNode();
        std::string getUsername();
        double getMemoryRequirement();
        double getBeginTimestamp();
        void setBeginTimestamp(double time_stamp);
        double getEndingTimestamp();
        double getArrivalTimestamp();
        unsigned long getRequestedNumNodes();
        std::shared_ptr<WorkflowJob> getWorkflowJob();
        void setEndingTimestamp(double time_stamp);
        std::map<std::string, std::tuple<unsigned long, double>> getResourcesAllocated();
        void setAllocatedResources(std::map<std::string, std::tuple<unsigned long, double>> resources);

        void setAllocatedNodeIndices(std::vector<int> indices) {
            this->allocated_node_indices = indices;
        }

        std::vector<int> getAllocatedNodeIndices() {
            return this->allocated_node_indices;
        }

    private:

        friend class CONSERVATIVEBFBatchScheduler;
        friend class CONSERVATIVEBFBatchSchedulerCoreLevel;

        u_int32_t conservative_bf_start_date;           // Field used by CONSERVATIVE_BF
        u_int32_t conservative_bf_expected_end_date;    // Field used by CONSERVATIVE_BF

        unsigned long job_id;
        unsigned long requested_num_nodes;
        unsigned long  requested_time;
        std::shared_ptr<WorkflowJob> job;
        unsigned long requested_cores_per_node;
        std::string username;
        double begin_time_stamp;
        double ending_time_stamp;
        double arrival_time_stamp;
        std::map<std::string, std::tuple<unsigned long, double>> resources_allocated;

        std::vector<int> allocated_node_indices;

    public:
        // Variables below are for the BatSim-style CVS output log file (only ifdef ENABLED_BATSCHED)
        /** @brief The meta-data field for BatSim-style CSV output */
        std::string csv_metadata;
        /** @brief The allocated processors field for BatSim-style CSV output */
        std::string csv_allocated_processors; 
    };

    /***********************/
    /** \endcond           */
    /***********************/

}




#endif //WRENCH_BATCHJOB_H
