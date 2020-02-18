#include "wrench/simulation/SimulationTimestampTypes.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include <wrench-dev.h>

WRENCH_LOG_NEW_DEFAULT_CATEGORY(simulation_timestamps, "Log category for SimulationTimeStamps");


namespace wrench {

    /**
     *
     * @param file - tuple of three strings relating to File, Source and Whoami
     * @return XOR of hashes of file
     */
    ///size_t file_hash( const File & file )
    ///{
    ///    return std::hash<void *>()(std::get<0>(file)) ^ std::hash<void *>()(std::get<1>(file)) ^ std::hash<void *>()(std::get<2>(file));
    ///}

    SimulationTimestampType::SimulationTimestampType() {
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
     * @brief Constructor
     */
    SimulationTimestampPair::SimulationTimestampPair() : endpoint(nullptr) {

    }

    /**
     * @brief Constructor
     * @param endpoint: an corresponding "end" timestamp or "start" timestamp
     */
    SimulationTimestampPair::SimulationTimestampPair(SimulationTimestampPair *endpoint)
            : endpoint(endpoint) {

    }

    /**
     * @brief Retrieves the corresponding start/end SimulationTimestampType associated with this timestamp
     * @return A pointer to a start SimulationTimestampType if this is a failure/completion timestamp or vise versa
     */
    SimulationTimestampPair *SimulationTimestampPair::getEndpoint() {
        return this->endpoint;
    }

    /**
     * @brief Constructor
     * @param task: a pointer to the WorkflowTask associated with this timestamp
     * @throw std::invalid_argument
     */
    SimulationTimestampTask::SimulationTimestampTask(WorkflowTask *task) : task(task) {
        if (task == nullptr) {
            throw std::invalid_argument(
                    "SimulationTimestampTask::SimulationTimestampTask() requires a pointer to a Workflowtask");
        }
    }

    /**
     * @brief Retrieves the WorkflowTask associated with this timestamp
     * @return a pointer to the WorkflowTask associated with this timestamp
     */
    WorkflowTask *SimulationTimestampTask::getTask() {
        return this->task;
    }

    /**
     * @brief retrieves the corresponding SimulationTimestampTask object
     * @return a pointer to the start or end SimulationTimestampTask object
     */
    SimulationTimestampTask *SimulationTimestampTask::getEndpoint() {
        return dynamic_cast<SimulationTimestampTask *>(this->endpoint);
    }

    /**
     * @brief A static map of SimulationTimestampTaskStart objects that have yet to matched with SimulationTimestampTaskFailure, SimulationTimestampTaskTerminated, SimulationTimestampTaskCompletion timestamps
     */
    std::map<std::string, SimulationTimestampTask *> SimulationTimestampTask::pending_task_timestamps;



    /**
     * @brief Sets the endpoint of the calling object (SimulationTimestampTaskFailure, SimulationTimestampTaskTerminated, SimulationTimestampTaskStart) with a SimulationTimestampTaskStart object
     */
    void SimulationTimestampTask::setEndpoints() {
        // find the SimulationTimestampTaskStart object containing the same task
        auto pending_tasks_itr = pending_task_timestamps.find(this->task->getID());
        if (pending_tasks_itr != pending_task_timestamps.end()) {
            // set my endpoint to the SimulationTimestampTaskStart
            this->endpoint = (*pending_tasks_itr).second;

            // set the SimulationTimestampTaskStart's endpoint to me
            (*pending_tasks_itr).second->endpoint = this;

            // the SimulationTimestampTaskStart is no longer waiting to be matched with an end timestamp, remove it from the map
            pending_task_timestamps.erase(pending_tasks_itr);
        } else {
            throw std::runtime_error(
                    "SimulationTimestampTask::setEndpoints() could not find a SimulationTimestampTaskStart object with taskID: " +
                    this->task->getID());
        }
    }

    /**
     * @brief Constructor
     * @param task: the WorkflowTask associated with this timestamp
     * @throw std::invalid_argument
     */
    SimulationTimestampTaskStart::SimulationTimestampTaskStart(WorkflowTask *task) : SimulationTimestampTask(task) {
        WRENCH_DEBUG("Inserting a Taskstart timestamp for task '%s'", task->getID().c_str());

        if (task == nullptr) {
            throw std::invalid_argument(
                    "SimulationTimestampTaskStart::SimulationTimestampTaskStart() requires a valid pointer to a WorkflowTask object");
        }

        /*
         * Upon creation, this object adds a pointer of itself to the 'pending_task_timestamps' map so that it's endpoint can
         * be set when a SimulationTimestampTaskFailure, SimulationTimestampTaskTerminated, or SimulationTimestampTaskCompletion is created
         */
        pending_task_timestamps.insert(std::make_pair(task->getID(), this));

    }


    /**
     * @brief Constructor
     * @param task: the WorkflowTask associated with this timestamp
     * @throw std::invalid_argument
     */
    SimulationTimestampTaskFailure::SimulationTimestampTaskFailure(WorkflowTask *task) : SimulationTimestampTask(task) {
        WRENCH_DEBUG("Inserting a TaskFailure timestamp for task '%s'", task->getID().c_str());

        if (task == nullptr) {
            throw std::invalid_argument(
                    "SimulationTimestampTaskFailure::SimulationTimestampTaskFailure() requires a valid pointer to a WorkflowTask object");
        }

        // match this timestamp with a SimulationTimestampTaskStart
        setEndpoints();
    }

    /**
     * @brief Constructor
     * @param task: the WorkflowTask associated with this timestamp
     */
    SimulationTimestampTaskCompletion::SimulationTimestampTaskCompletion(WorkflowTask *task) : SimulationTimestampTask(
            task) {
        WRENCH_DEBUG("Inserting a TaskCompletion timestamp for task '%s'", task->getID().c_str());

        if (task == nullptr) {
            throw std::invalid_argument(
                    "SimulationTimestampTaskCompletion::SimulationTimestampTaskCompletion() requires a valid pointer to a WorkflowTask object");
        }

        // match this timestamp with a SimulationTimestampTaskStart
        setEndpoints();
    }

    /**
     * @brief Constructor
     * @param task: the WorkflowTask associated with this timestamp
     */
    SimulationTimestampTaskTermination::SimulationTimestampTaskTermination(WorkflowTask *task)
            : SimulationTimestampTask(task) {
        WRENCH_DEBUG("Inserting a TaskTerminated timestamp for task '%s'", task->getID().c_str());

        if (task == nullptr) {
            throw std::invalid_argument(
                    "SimulationTimestampTaskTerminated::SimulationTimestampTaskTerminated() requires a valid pointer to a WorkflowTask object");
        }

        // match this timestamp with a SimulationTimestampTaskStart
        setEndpoints();
    }

    /**
     * @brief Constructor
     * @param file: the WorkflowFile associated with this file copy
     * @param src_location: the source location
     * @param dst_location: the destination location
     * @param start_timestamp: the timestamp for the file copy start
     */
    SimulationTimestampFileCopy::SimulationTimestampFileCopy(WorkflowFile *file,
                                                             std::shared_ptr<FileLocation> src_location,
                                                             std::shared_ptr<FileLocation> dst_location) :
             file(file), source(src_location), destination(dst_location) {
    }

    /**
     * @brief retrieves the WorkflowFile being copied
     * @return a pointer to the WorkflowFile associated with this copy
     */
    WorkflowFile *SimulationTimestampFileCopy::getFile() {
        return this->file;
    }

    /**
     * @brief retrieves the location from which the WorkflowFile is being copied
     * @return the copy's source location
     */
    std::shared_ptr<FileLocation> SimulationTimestampFileCopy::getSource() {
        return this->source;
    }

    /**
     * @brief retrieves the location where the WorkflowFile will be copied
     * @return the copy's destination location
     */
    std::shared_ptr<FileLocation> SimulationTimestampFileCopy::getDestination() {
        return this->destination;
    }


    /**
     * @brief retrieves the corresponding SimulationTimestampFileCopy object
     * @return a pointer to the start or end SimulationTimestampFileCopy object
     */
    SimulationTimestampFileCopy *SimulationTimestampFileCopy::getEndpoint() {
        return dynamic_cast<SimulationTimestampFileCopy *>(this->endpoint);
    }

    /**
    * @brief A static unordered multimap of SimulationTimestampFileCopyStart objects that have yet to be matched with Failure, Terminated or Completion timestamps
    */
    std::unordered_multimap<File, SimulationTimestampFileCopy *> SimulationTimestampFileCopy::pending_file_copies;


    /**
     * @brief Sets the endpoint of the calling object (SimulationTimestampFileCopyFailure, SimulationTimestampFileCopyCompletion, SimulationTimestampFileCopyStart) with a SimulationTimestampFileCopyStart object
     */
    void SimulationTimestampFileCopy::setEndpoints() {
        auto pending_copies_itr = pending_file_copies.find(File(this->file, this->source.get(), this->destination.get()));
        if (pending_copies_itr != pending_file_copies.end()) {
            // set my endpoint to the SimulationTimestampFileCopyStart
            this->endpoint = (*pending_copies_itr).second;

            // set the SimulationTimestampFileCopyStart's endpoint to me
            (*pending_copies_itr).second->endpoint = this;

            // the SimulationTimestampFileCopyStart is no longer waiting to be matched with an end timestamp, remove it from the map
            pending_file_copies.erase(pending_copies_itr);
        } else {
            throw std::runtime_error(
                    "SimulationTimestampFileCopy::setEndpoints() could not find a SimulationTimestampFileCopyStart object.");
        }
    }

    /**
     * @brief Constructor
     * @param file: the WorkflowFile associated with this file copy
     * @param src: the source location
     * @param dst: the destination location
     * @throw std::invalid_argument
     */
    SimulationTimestampFileCopyStart::SimulationTimestampFileCopyStart(WorkflowFile *file,
                                                                       std::shared_ptr<FileLocation> src,
                                                                       std::shared_ptr<FileLocation> dst) :
            SimulationTimestampFileCopy(file, src, dst) {
        WRENCH_DEBUG("Inserting a FileCopyStart timestamp for file copy");

        // all information about a file copy should be passed
        if ((this->file == nullptr)
            || (this->source == nullptr)
            || (this->destination == nullptr)) {

            throw std::invalid_argument(
                    "SimulationTimestampFileCopyStart::SimulationTimestampFileCopyStart() cannot take nullptr arguments");
        }

        pending_file_copies.insert(std::make_pair(File(this->file, this->source.get(), this->destination.get()), this));
    }


    /**
     * @brief Constructor
     * @param start_timestamp: a pointer to the SimulationTimestampFileCopyStart associated with this timestamp
     * @throw std::invalid_argument
     */
    SimulationTimestampFileCopyFailure::SimulationTimestampFileCopyFailure(WorkflowFile *file,
                                                                           std::shared_ptr<FileLocation> src,
                                                                           std::shared_ptr<FileLocation> dst) :
            SimulationTimestampFileCopy(file, src, dst) {
        WRENCH_DEBUG("Inserting a FileCopyFailure timestamp for file copy");

        // all information about a file copy should be passed
        if ((this->file == nullptr)
            || (this->source == nullptr)
            || (this->destination == nullptr)) {

            throw std::invalid_argument(
                    "SimulationTimestampFileCopyFailure::SimulationTimestampFileCopyFailure() cannot take nullptr arguments");
        }

        setEndpoints();
    }

    /**
     * @brief Constructor
     * @param start_timestamp: a pointer to the SimulationTimestampFileCopyStart associated with this timestamp
     * @throw std::invalid_argument
     */
    SimulationTimestampFileCopyCompletion::SimulationTimestampFileCopyCompletion(WorkflowFile *file,
                                                                                 std::shared_ptr<FileLocation> src,
                                                                                 std::shared_ptr<FileLocation> dst) :
            SimulationTimestampFileCopy(file, src, dst) {
        WRENCH_DEBUG("Inserting a FileCopyCompletion timestamp for file copy");


        // all information about a file copy should be passed
        if ((this->file == nullptr)
            || (this->source == nullptr)
            || (this->destination == nullptr)) {

            throw std::invalid_argument(
                    "SimulationTimestampFileCopyCompletion::SimulationTimestampFileCopyCompletion() cannot take nullptr arguments");
        }

        setEndpoints();
    }



    /**
     * @brief Constructor
     * @param file: the WorkflowFile associated with this file read
     * @param src_location: the source location
     * @param service: service requesting file read
     */
    SimulationTimestampFileRead::SimulationTimestampFileRead(WorkflowFile *file,
                                                             FileLocation *src_location,
                                                             StorageService *service,
                                                             WorkflowTask *task) :
            file(file), source(src_location), service(service), task(task){
    }

    /**
     * @brief retrieves the WorkflowFile being read
     * @return a pointer to the WorkflowFile associated with this reads
     */
    WorkflowFile *SimulationTimestampFileRead::getFile() {
        return this->file;
    }

    /**
     * @brief retrieves the location from which the WorkflowFile is being copied
     * @return the read's source location
     */
    FileLocation *SimulationTimestampFileRead::getSource() {
        return this->source;
    }

    /**
     * @brief retrieves the storage service for file read
     * @return point to the service associated with this read
     */
    StorageService *SimulationTimestampFileRead::getService() {
        return this->service;
    }

    /**
     * @brief retrieves task associated w/ file read
     * @return a pointer to the WorkflowTask associated with this timestamp
     */
    WorkflowTask *SimulationTimestampFileRead::getTask() {
        return this->task;
    }

    /**
     * @brief retrieves the corresponding SimulationTimestampFileRead object
     * @return a pointer to the start or end SimulationTimestampFileRead object
     */
    SimulationTimestampFileRead *SimulationTimestampFileRead::getEndpoint() {
        return dynamic_cast<SimulationTimestampFileRead *>(this->endpoint);
    }

    /**
     * @brief A static unordered multimap of SimulationTimestampFileReadStart objects that have yet to be matched with Failure, Terminated or Completion timestamps
     */
    std::unordered_multimap<File, std::pair<SimulationTimestampFileRead *, WorkflowTask *>> SimulationTimestampFileRead::pending_file_reads;

    /**
     * @brief Sets the endpoint of the calling object (SimulationTimestampFileReadFailure, SimulationTimestampFileReadTerminated, SimulationTimestampFileReadStart) with a SimulationTimestampFileReadStart object
     */
    void SimulationTimestampFileRead::setEndpoints() {
        // find the SimulationTimestampFileRead object containing the same task
        auto pending_reads_itr = pending_file_reads.find(File(this->file, this->source, this->service));
        if (pending_reads_itr != pending_file_reads.end()) {
            // set my endpoint to the SimulationTimestampFileReadStart
            this->endpoint = (*pending_reads_itr).second.first;

            // set the SimulationTimestampFileReadStart's endpoint to me
            (*pending_reads_itr).second.first->endpoint = this;

            // the SimulationTimestampFileReadStart is no longer waiting to be matched with an end timestamp, remove it from the map
            pending_file_reads.erase(pending_reads_itr);
        } else {
            throw std::runtime_error(
                    "SimulationTimestampFileRead::setEndpoints() could not find a SimulationTimestampFileReadStart object.");
        }
    }



    /**
     * @brief Constructor
     * @param file: the WorkflowFile associated with this file read
     * @param src: the source location
     * @param service: service requesting file read
     * @throw std::invalid_argument
     */
    SimulationTimestampFileReadStart::SimulationTimestampFileReadStart(WorkflowFile *file,
                                                                       FileLocation *src,
                                                                       StorageService *service,
                                                                       WorkflowTask *task) :
            SimulationTimestampFileRead(file, src, service, task) {
        WRENCH_DEBUG("Inserting a FileReadStart timestamp for file read");

        // all information about a file read should be passed
        if ((this->file == nullptr)
            || (this->source == nullptr)
            || (this->service == nullptr)) {

            throw std::invalid_argument(
                    "SimulationTimestampFileReadStart::SimulationTimestampFileReadStart() cannot take nullptr arguments");
        }


        pending_file_reads.insert(std::make_pair(File(this->file, this->source, this->service), std::make_pair(this, this->task)));
    }


    /**
     * @brief Constructor
     * @param file: the WorkflowFile associated with this file read
     * @param src: the source location
     * @param service: service requesting file read
     * @throw std::invalid_argument
     */
    SimulationTimestampFileReadFailure::SimulationTimestampFileReadFailure(WorkflowFile *file,
                                                                           FileLocation *src,
                                                                           StorageService *service,
                                                                           WorkflowTask *task) :
            SimulationTimestampFileRead(file, src, service, task) {
        WRENCH_DEBUG("Inserting a FileReadFailure timestamp for file read");

        if (file == nullptr
            || src == nullptr
            || (service == nullptr)) {
            throw std::invalid_argument(
                    "SimulationTimestampFileReadFailure::SimulationTimestampFileReadFailure() requires a valid pointer to file, source and service objects");
        }

        // match this timestamp with a SimulationTimestampFileReadStart
        setEndpoints();
    }

    /**
     * @brief Constructor
     * @param file: the WorkflowFile associated with this file read
     * @param src: the source location
     * @param service: service requesting file read
     * @throw std::invalid_argument
     */
    SimulationTimestampFileReadCompletion::SimulationTimestampFileReadCompletion(WorkflowFile *file,
                                                                                 FileLocation *src,
                                                                                 StorageService *service,
                                                                                 WorkflowTask *task) :
            SimulationTimestampFileRead(file, src, service, task)  {
        WRENCH_DEBUG("Inserting a FileReadCompletion timestamp for file read");

        if (file == nullptr
            || src == nullptr
            || service == nullptr) {
            throw std::invalid_argument(
                    "SimulationTimestampFileReadFailure::SimulationTimestampFileReadFailure() requires a valid pointer to file, source and service objects");
        }

        // match this timestamp with a SimulationTimestampFileReadStart
        setEndpoints();
    }

    /**
     * @brief Constructor
     * @param file: the WorkflowFile associated with this file write
     * @param dst_location: the destination location
     * @param service: service requesting file write
     */
    SimulationTimestampFileWrite::SimulationTimestampFileWrite(WorkflowFile *file,
                                                               FileLocation *dst_location,
                                                               StorageService *service,
                                                               WorkflowTask *task) :
            file(file), destination(dst_location), service(service), task(task){
    }

    /**
     * @brief retrieves the WorkflowFile being written
     * @return a pointer to the WorkflowFile associated with this writes
     */
    WorkflowFile *SimulationTimestampFileWrite::getFile() {
        return this->file;
    }

    /**
     * @brief retrieves the location from which the WorkflowFile is being copied
     * @return the write's destination location
     */
    FileLocation *SimulationTimestampFileWrite::getDestination() {
        return this->destination;
    }

    /**
     * @brief retrieves the Service that ordered file write
     * @return point to the service associated with this write
     */
    StorageService *SimulationTimestampFileWrite::getService() {
        return this->service;
    }

    /**
     * @brief retrieves task associated w/ file write
     * @return a pointer to the WorkflowTask associated with this timestamp
     */
    WorkflowTask *SimulationTimestampFileWrite::getTask() {
        return this->task;
    }

    /**
     * @brief retrieves the corresponding SimulationTimestampFileWrite object
     * @return a pointer to the start or end SimulationTimestampFileWrite object
     */
    SimulationTimestampFileWrite *SimulationTimestampFileWrite::getEndpoint() {
        return dynamic_cast<SimulationTimestampFileWrite *>(this->endpoint);
    }

    /**
     * @brief A static unordered multimap of SimulationTimestampFileWriteStart objects that have yet to be matched with Failure, Terminated or Completion timestamps
     */
    std::unordered_multimap<File, std::pair<SimulationTimestampFileWrite *, WorkflowTask *>> SimulationTimestampFileWrite::pending_file_writes;

    /**
     * @brief Sets the endpoint of the calling object (SimulationTimestampFileWriteFailure, SimulationTimestampFileWriteTerminated, SimulationTimestampFileWriteStart) with a SimulationTimestampFileWriteStart object
     */
    void SimulationTimestampFileWrite::setEndpoints() {
        // find the SimulationTimestampFileWrite object containing the same task
        auto pending_writes_itr = pending_file_writes.find(File(this->file, this->destination, this->service));
        if (pending_writes_itr != pending_file_writes.end()) {
            // set my endpoint to the SimulationTimestampFileWriteStart
            this->endpoint = (*pending_writes_itr).second.first;

            // set the SimulationTimestampFileWriteStart's endpoint to me
            (*pending_writes_itr).second.first->endpoint = this;

            // the SimulationTimestampFileWriteStart is no longer waiting to be matched with an end timestamp, remove it from the map
            pending_file_writes.erase(pending_writes_itr);
        } else {
            throw std::runtime_error(
                    "SimulationTimestampFileWrite::setEndpoints() could not find a SimulationTimestampFileWriteStart object.");
        }
    }



    /**
     * @brief Constructor
     * @param file: the WorkflowFile associated with this file write
     * @param dst: the destination location
     * @param service: service requesting file write
     * @throw std::invalid_argument
     */
    SimulationTimestampFileWriteStart::SimulationTimestampFileWriteStart(WorkflowFile *file,
                                                                         FileLocation *dst,
                                                                         StorageService *service,
                                                                         WorkflowTask *task) :
            SimulationTimestampFileWrite(file, dst, service, task) {
        WRENCH_DEBUG("Inserting a FileWriteStart timestamp for file write");

        // all information about a file write should be passed
        if ((this->file == nullptr)
            || (this->destination == nullptr)
            || (this->service == nullptr)) {

            throw std::invalid_argument(
                    "SimulationTimestampFileWriteStart::SimulationTimestampFileWriteStart() cannot take nullptr arguments");
        }


        pending_file_writes.insert(std::make_pair(File(this->file, this->destination, this->service), std::make_pair(this, this->task)));

    }


    /**
     * @brief Constructor
     * @param file: the WorkflowFile associated with this file write
     * @param dst: the destination location
     * @param service: service requesting file write
     * @throw std::invalid_argument
     */
    SimulationTimestampFileWriteFailure::SimulationTimestampFileWriteFailure(WorkflowFile *file,
                                                                             FileLocation *dst,
                                                                             StorageService *service,
                                                                             WorkflowTask *task) :
            SimulationTimestampFileWrite(file, dst, service, task) {
        WRENCH_DEBUG("Inserting a FileWriteFailure timestamp for file write");

        if (file == nullptr
            || dst == nullptr
            || service == nullptr) {
            throw std::invalid_argument(
                    "SimulationTimestampFileWriteFailure::SimulationTimestampFileWriteFailure() requires a valid pointer to file, destination and service objects");
        }

        // match this timestamp with a SimulationTimestampFileWriteStart
        setEndpoints();
    }

    /**
     * @brief Constructor
     * @param file: the WorkflowFile associated with this file write
     * @param dst: the destination location
     * @param service: service requesting file write
     * @throw std::invalid_argument
     */
    SimulationTimestampFileWriteCompletion::SimulationTimestampFileWriteCompletion(WorkflowFile *file,
                                                                                   FileLocation *dst,
                                                                                   StorageService *service,
                                                                                   WorkflowTask *task) :
            SimulationTimestampFileWrite(file, dst, service, task)  {
        WRENCH_DEBUG("Inserting a FileWriteCompletion timestamp for file write");

        if (file == nullptr
            || dst == nullptr
            || service == nullptr) {
            throw std::invalid_argument(
                    "SimulationTimestampFileWriteFailure::SimulationTimestampFileWriteFailure() requires a valid pointer to file, destination and service objects");
        }

        // match this timestamp with a SimulationTimestampFileWriteStart
        setEndpoints();
    }




    /**
     * @brief Constructor
     * @param hostname: the host on which a pstate is being set
     * @param pstate: the pstate that is being set on this host
     */
    SimulationTimestampPstateSet::SimulationTimestampPstateSet(std::string hostname, int pstate) :
            hostname(hostname), pstate(pstate) {

        if (hostname.empty()) {
            throw std::invalid_argument(
                    "SimulationTimestampPstateSet::SimulationTimestampPstateSet() requires a valid hostname");
        }
    }

    /**
     * @brief Get the hostname associated with this timestamp
     * @return the hostname associated with this timestamp
     */
    std::string SimulationTimestampPstateSet::getHostname() {
        return this->hostname;
    }

    /**
     * @brief Get the pstate associated with this timestamp
     * @return the pstate associated with this timestamp
     */
    int SimulationTimestampPstateSet::getPstate() {
        return this->pstate;
    }

    /**
     * @brief Constructor
     * @param hostname: the host on which this energy consumption timestamp applies to
     * @param joules: the energy consumption in joules 
     */
    SimulationTimestampEnergyConsumption::SimulationTimestampEnergyConsumption(std::string hostname, double joules)
            : hostname(hostname), joules(joules) {

        if (hostname.empty() || joules < 0.0) {
            throw std::invalid_argument(
                    "SimulationTimestampEnergyConsumption::SimulationTimestampEnergyConsumption() requires a valid hostname and an energy usage amount >= 0.0");
        }
    }

    /**
     * @brief Get the hostname associated with this timestamp
     * @return the hostname associated with this timestamp
     */
    std::string SimulationTimestampEnergyConsumption::getHostname() {
        return this->hostname;
    }

    /**
     * @brief Get the energy consumption in joules
     * @return energy consumed by this host in joules
     */
    double SimulationTimestampEnergyConsumption::getConsumption() {
        return this->joules;
    }

}

