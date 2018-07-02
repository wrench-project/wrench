#include "wrench/simulation/SimulationTimestampTypes.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"

namespace wrench {

    /**
     * @brief Constructor
     */
    SimulationTimestampType::SimulationTimestampType() : SimulationTimestampType(nullptr) {

    }

    /**
     *  @brief Constructor
     */
    SimulationTimestampType::SimulationTimestampType(wrench::SimulationTimestampType *endpoint) : endpoint(endpoint){
        this->date = S4U_Simulation::getClock();
    }

    /**
     * @brief Retrieve the date recorded for this timestamp
     * @return the date of this timestamp
     */
    double SimulationTimestampType::getDate() {
        return this->date;
    }

    /**
     * @brief Retrieve the corresponding start timestamp if this is an end(failure or completion) timestamp or vice versa.
     * @return a pointer to the corresponding timestamp object
     */
    SimulationTimestampType *SimulationTimestampType::getEndpoint() {
        return this->endpoint;
    }

    /**
     * @brief Constructor
     * @param task: a pointer to the WorkflowTask associated with this timestamp
     */
    SimulationTimestampTask::SimulationTimestampTask(WorkflowTask *task) : task(task) {

    }

    /**
     * @brief Retrieves the WorkflowTask associated with this timestamp
     * @return a pointer to the WorkflowTask associated with this timestamp
     */
    WorkflowTask *SimulationTimestampTask::getTask() {
        return this->task;
    }

    /**
     * @brief A static map of SimulationTimestampTaskStart objects that have yet to matched with SimulationTimestampTaskFailure or SimulationTimestampTaskCompletion timestamps
     */
    std::map<std::string, SimulationTimestampTask *> SimulationTimestampTask::pending_task_timestamps;

    /**
     * @brief Sets the endpoint of the calling object (SimulationTimestampTaskFailure or SimulationTimestampTaskStart) with a SimulationTimestampTaskStart object
     */
    void SimulationTimestampTask::setEndpoints() {
        // find the SimulationTimestampTaskStart object containing the same task
        auto pending_tasks_itr = pending_task_timestamps.find(this->task->getID());
        if (pending_tasks_itr != pending_task_timestamps.end()) {
            // set the SimulationTimestampTaskStart's endpoint to me
            (*pending_tasks_itr).second->endpoint = this;

            // set my endpoint to the SimulationTimestampTaskStart
            this->endpoint = (*pending_tasks_itr).second->endpoint;

            // the SimulationTimestampTaskStart is no longer waiting to be matched with an end timestamp, remove it from the map
            pending_task_timestamps.erase(pending_tasks_itr);
        }
    }

    /**
     * @brief Constructor
     * @param task: the WorkflowTask associated with this timestamp
     */
    SimulationTimestampTaskStart::SimulationTimestampTaskStart(WorkflowTask *task) : SimulationTimestampTask(task) {
        /*
         * Upon creation, this object adds a pointer of itself to the 'pending_task_timestamps' map so that it's endpoint can
         * be set when a SimulationTimestampTaskFailure or SimulationTimestampTaskCompletion is created
         */
        pending_task_timestamps.insert(std::make_pair(task->getID(), this));
    }


    /**
     * @brief Constructor
     * @param task: the WorkflowTask associated with this timestamp
     */
    SimulationTimestampTaskFailure::SimulationTimestampTaskFailure(WorkflowTask *task) : SimulationTimestampTask(task) {
        // match this timestamp with a SimulationTimestampTaskStart
        if (task != nullptr) {
            setEndpoints();
        }
    }

    /**
     * @brief Constructor
     * @param task: the WorkflowTask associated with this timestamp
     */
    SimulationTimestampTaskCompletion::SimulationTimestampTaskCompletion(WorkflowTask *task) : SimulationTimestampTask(
            task) {
        // match this timestamp with a SimulationTimestampTaskStart
        if (task != nullptr) {
            setEndpoints();
        }
    }

    /**
     * @brief Constructor
     * @param file: the file associated with this file copy
     * @param src: the source StorageService from which this file is being copied
     * @param src_partition: the partition in the source StorageService from which this file is being copied
     * @param dst: the destination StorageService where this file will be copied
     * @param dst_partition: the partition in the destination StorageService where this file will be copied
     */
    SimulationTimestampFileCopy::SimulationTimestampFileCopy(WorkflowFile *file, StorageService *src, std::string src_partition, StorageService *dst, std::string dst_partition, SimulationTimestampFileCopyStart *start_timestamp) :
            SimulationTimestampType(start_timestamp), file(file), source(FileLocation(src, src_partition)), destination(FileLocation(dst, dst_partition)) {
    }

    /**
     * @brief retrieves the file being copied
     * @return a pointer to the WorkflowFile associated with this copy
     */
    WorkflowFile* SimulationTimestampFileCopy::getFile() {
        return this->file;
    }

    /**
     * @brief retrieves the location from which the file is being copied
     * @return a FileLocation object containing the source StorageService and source partition from which the file is being copied
     */
    SimulationTimestampFileCopy::FileLocation SimulationTimestampFileCopy::getSource() {
        return this->source;
    }

    /**
     * @brief retrieves the location where the file will be copied
     * @return a FileLocation object containing the destination StorageService and destination partition into which the file is being copied
     */
    SimulationTimestampFileCopy::FileLocation SimulationTimestampFileCopy::getDestination() {
        return this->destination;
    }

    /**
     * @brief Constructor
     * @param file: the file associated with this file copy
     * @param src: the source StorageService from which this file is being copied
     * @param src_partition: the partition in the source StorageService from which this file is being copied
     * @param dst: the destination StorageService where this file will be copied
     * @param dst_partition: the partition in the destination StorageService where this file will be copied
     */
    SimulationTimestampFileCopyStart::SimulationTimestampFileCopyStart(WorkflowFile *file, StorageService *src, std::string src_partition,
                                                                       StorageService *dst, std::string dst_partition) :
            SimulationTimestampFileCopy(file, src, src_partition, dst, dst_partition) {

    }


    /**
     * @brief Constructor
     * @param file: the file associated with this file copy
     * @param src: the source StorageService from which this file was being copied
     * @param src_partition: the partition in the source StorageService from which this file was being copied
     * @param dst: the destination StorageService where this file was going to be copied
     * @param dst_partition: the partition in the destination StorageService where this file was going to be copied
     * @param start_timestamp: a pointer to the SimulationTimestampFileCopyStart associated with this timestamp
     */
    SimulationTimestampFileCopyFailure::SimulationTimestampFileCopyFailure(WorkflowFile *file, StorageService *src, std::string src_partition,
                                                                           StorageService *dst, std::string dst_partition, SimulationTimestampFileCopyStart *start_timestamp) :
            SimulationTimestampFileCopy(file, src, src_partition, dst, dst_partition, start_timestamp) {
            setEndpoints();
    }

    /**
     * @brief Constructor
     * @param file: the file associated with this file copy
     * @param src: the source StorageService from which this file was being copied
     * @param src_partition: the partition in the source StorageService from which this file was being copied
     * @param dst: the destination StorageService where this file was going to be copied
     * @param dst_partition: the partition in the destination StorageService where this file was going to be copied
     * @param start_timestamp: a pointer to the SimulationTimestampFileCopyStart associated with this timestamp
     */
    SimulationTimestampFileCopyCompletion::SimulationTimestampFileCopyCompletion(WorkflowFile *file, StorageService *src, std::string src_partition,
                                                                           StorageService *dst, std::string dst_partition, SimulationTimestampFileCopyStart *start_timestamp) :
            SimulationTimestampFileCopy(file, src, src_partition, dst, dst_partition, start_timestamp) {
            setEndpoints();
    }

    void SimulationTimestampFileCopy::setEndpoints() {
        this->endpoint->endpoint = this;
    }
}

