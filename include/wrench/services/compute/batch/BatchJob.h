#ifndef WRENCH_BATCHJOB_H
#define WRENCH_BATCHJOB_H

#include "wrench/job/CompoundJob.h"

namespace wrench {

    /***********************/
    /** \cond INTERNAL    */
    /***********************/

    /**
     * @brief A batch job, which encapsulates a Job and additional information
     *        used by a batch
     */
    class BatchJob {
    public:
        //job, jobid, -t, -N, -c, ending s4u_timestamp (-1 as undetermined)
        BatchJob(const std::shared_ptr<CompoundJob> &job, unsigned long job_id, unsigned long time_in_minutes, unsigned long number_nodes,
                 unsigned long cores_per_node, const std::string &username, double ending_time_stamp, double arrival_time_stamp);


        unsigned long getJobID() const;
        unsigned long getRequestedTime() const;
        void setRequestedTime(unsigned long time);
        unsigned long getRequestedCoresPerNode() const;
        std::string getUsername();
        double getMemoryRequirement();
        double getBeginTimestamp() const;
        void setBeginTimestamp(double time_stamp);
        double getEndingTimestamp() const;
        double getArrivalTimestamp() const;
        unsigned long getRequestedNumNodes() const;
        std::shared_ptr<CompoundJob> getCompoundJob();
        void setEndingTimestamp(double time_stamp);
        std::map<std::string, std::tuple<unsigned long, double>> getResourcesAllocated();
        void setAllocatedResources(const std::map<std::string, std::tuple<unsigned long, double>> &resources);

        /** 
         * @brief Set the indices of the allocated nodes
         * @param indices: a list of indices
         */
        void setAllocatedNodeIndices(std::vector<int> indices) {
            this->allocated_node_indices = indices;
        }

        /** 
         * @brief Get the indices of allocated nodes
         * @return a list of indices
         */
        std::vector<int> getAllocatedNodeIndices() {
            return this->allocated_node_indices;
        }

    private:
        friend class ConservativeBackfillingBatchScheduler;
        friend class ConservativeBackfillingBatchSchedulerCoreLevel;

        u_int32_t conservative_bf_start_date;       // Field used by CONSERVATIVE_BF
        u_int32_t conservative_bf_expected_end_date;// Field used by CONSERVATIVE_BF

        unsigned long job_id;
        unsigned long requested_num_nodes;
        unsigned long requested_time;
        std::shared_ptr<CompoundJob> compound_job;
        unsigned long requested_cores_per_node;
        std::string username;
        double begin_time_stamp;
        double ending_time_stamp;
        double arrival_time_stamp;
        std::map<std::string, std::tuple<unsigned long, double>> resources_allocated;

        std::vector<int> allocated_node_indices;

    public:
        // Variables below are for the BatSim-style CSV output log file (only ifdef ENABLED_BATSCHED)
        /** @brief The meta-data field for BatSim-style CSV output */
        std::string csv_metadata;
        /** @brief The allocated processors field for BatSim-style CSV output */
        std::string csv_allocated_processors;
    };

    /***********************/
    /** \endcond           */
    /***********************/

}// namespace wrench


#endif//WRENCH_BATCHJOB_H
