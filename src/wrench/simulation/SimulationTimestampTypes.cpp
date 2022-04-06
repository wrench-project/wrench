#include <wrench/simulation/SimulationTimestampTypes.h>
#include <wrench/simgrid_S4U_util/S4U_Simulation.h>
#include <wrench-dev.h>

#include <utility>

WRENCH_LOG_CATEGORY(wrench_core_simulation_timestamps, "Log category for SimulationTimestamps");


namespace wrench {

    ///*
    // *
    // * @param file - tuple of three strings relating to File, Source and Whoami
    // * @return XOR of hashes of file
    // */
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
    double SimulationTimestampType::getDate() const {
        return this->date;
    }

    /**
     * @brief Set the date recorded for this timestamp
     * @param d: the date of this timestamp
     */
    void SimulationTimestampType::setDate(double d) {
        this->date = d;
    }

    /**
     * @brief Constructor
     */
    SimulationTimestampPair::SimulationTimestampPair() : endpoint(nullptr) {
    }

    /**
     * @brief Constructor
     * @param date: the date
     * @param endpoint: an corresponding "end" timestamp or "start" timestamp
     */
    SimulationTimestampPair::SimulationTimestampPair(double date, SimulationTimestampPair *endpoint)
            : endpoint(endpoint) {
        this->date = date;
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
     * @param date: the date
     * @param task: a pointer to the WorkflowTask associated with this timestamp
     * @throw std::invalid_argument
     */
    SimulationTimestampTask::SimulationTimestampTask(double date, const std::shared_ptr<WorkflowTask> &task) : task(task) {
        this->date = date;
        if (task == nullptr) {
            throw std::invalid_argument(
                    "SimulationTimestampTask::SimulationTimestampTask() requires a pointer to a Workflowtask");
        }
    }

    /**
     * @brief Retrieves the WorkflowTask associated with this timestamp
     * @return a pointer to the WorkflowTask associated with this timestamp
     */
    std::shared_ptr<WorkflowTask> SimulationTimestampTask::getTask() {
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
     * @param date: the date
     * @param task: the WorkflowTask associated with this timestamp
     * @throw std::invalid_argument
     */
    SimulationTimestampTaskStart::SimulationTimestampTaskStart(double date, const std::shared_ptr<WorkflowTask> &task) : SimulationTimestampTask(date, task) {
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
     * @param date: the date
     * @param task: the WorkflowTask associated with this timestamp
     * @throw std::invalid_argument
     */
    SimulationTimestampTaskFailure::SimulationTimestampTaskFailure(double date, const std::shared_ptr<WorkflowTask> &task) : SimulationTimestampTask(date, task) {
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
     * @param date: the date
     * @param task: the WorkflowTask associated with this timestamp
     */
    SimulationTimestampTaskCompletion::SimulationTimestampTaskCompletion(double date, const std::shared_ptr<WorkflowTask>& task) : SimulationTimestampTask(date, task) {
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
     * @param date: the date
     * @param task: the WorkflowTask associated with this timestamp
     */
    SimulationTimestampTaskTermination::SimulationTimestampTaskTermination(double date, const std::shared_ptr<WorkflowTask>& task)
            : SimulationTimestampTask(date, task) {
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
     * @param date: the date
     * @param file: the DataFile associated with this file copy
     * @param src_location: the source location
     * @param dst_location: the destination location
     */
    SimulationTimestampFileCopy::SimulationTimestampFileCopy(double date, std::shared_ptr<DataFile> file,
                                                             std::shared_ptr<FileLocation> src_location,
                                                             std::shared_ptr<FileLocation> dst_location) : file(std::move(file)), source(std::move(src_location)), destination(std::move(dst_location)) {
        this->date = date;
    }

    /**
     * @brief retrieves the DataFile being copied
     * @return a pointer to the DataFile associated with this copy
     */
    std::shared_ptr<DataFile> SimulationTimestampFileCopy::getFile() {
        return this->file;
    }

    /**
     * @brief retrieves the location from which the DataFile is being copied
     * @return the copy's source location
     */
    std::shared_ptr<FileLocation> SimulationTimestampFileCopy::getSource() {
        return this->source;
    }

    /**
     * @brief retrieves the location where the DataFile will be copied
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
    std::unordered_multimap<FileCopy, SimulationTimestampFileCopy *> SimulationTimestampFileCopy::pending_file_copies;


    /**
     * @brief Sets the endpoint of the calling object (SimulationTimestampFileCopyFailure, SimulationTimestampFileCopyCompletion, SimulationTimestampFileCopyStart) with a SimulationTimestampFileCopyStart object
     */
    void SimulationTimestampFileCopy::setEndpoints() {
        auto pending_copies_itr = pending_file_copies.find(FileCopy(this->file, this->source, this->destination));
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
     * @param date: the date
     * @param file: the DataFile associated with this file copy
     * @param src: the source location
     * @param dst: the destination location
     * @throw std::invalid_argument
     */
    SimulationTimestampFileCopyStart::SimulationTimestampFileCopyStart(double date, std::shared_ptr<DataFile> file,
                                                                       std::shared_ptr<FileLocation> src,
                                                                       std::shared_ptr<FileLocation> dst) : SimulationTimestampFileCopy(date, std::move(file), std::move(src), std::move(dst)) {
        WRENCH_DEBUG("Inserting a FileCopyStart timestamp for file copy");

        // all information about a file copy should be passed
        if ((this->file == nullptr) || (this->source == nullptr) || (this->destination == nullptr)) {

            throw std::invalid_argument(
                    "SimulationTimestampFileCopyStart::SimulationTimestampFileCopyStart() cannot take nullptr arguments");
        }

        pending_file_copies.insert(std::make_pair(FileCopy(this->file, this->source, this->destination), this));
    }


    /**
     * @brief Constructor
     * @param date: the date
     * @param file: A workflow file
     * @param src: the source location
     * @param dst: the destination location
     * @throw std::invalid_argument
     */
    SimulationTimestampFileCopyFailure::SimulationTimestampFileCopyFailure(double date, std::shared_ptr<DataFile> file,
                                                                           std::shared_ptr<FileLocation> src,
                                                                           std::shared_ptr<FileLocation> dst) : SimulationTimestampFileCopy(date, std::move(file), std::move(src), std::move(dst)) {
        WRENCH_DEBUG("Inserting a FileCopyFailure timestamp for file copy");

        // all information about a file copy should be passed
        if ((this->file == nullptr) || (this->source == nullptr) || (this->destination == nullptr)) {

            throw std::invalid_argument(
                    "SimulationTimestampFileCopyFailure::SimulationTimestampFileCopyFailure() cannot take nullptr arguments");
        }

        setEndpoints();
    }

    /**
     * @brief Constructor
     * @param date: the date
     * @param file: a workflow file
     * @param src: the source location
     * @param dst: the destination location
     * @throw std::invalid_argument
     */
    SimulationTimestampFileCopyCompletion::SimulationTimestampFileCopyCompletion(double date, std::shared_ptr<DataFile> file,
                                                                                 std::shared_ptr<FileLocation> src,
                                                                                 std::shared_ptr<FileLocation> dst) : SimulationTimestampFileCopy(date, std::move(file), std::move(src), std::move(dst)) {
        WRENCH_DEBUG("Inserting a FileCopyCompletion timestamp for file copy");

        // all information about a file copy should be passed
        if ((this->file == nullptr) || (this->source == nullptr) || (this->destination == nullptr)) {

            throw std::invalid_argument(
                    "SimulationTimestampFileCopyCompletion::SimulationTimestampFileCopyCompletion() cannot take nullptr arguments");
        }
        setEndpoints();
    }

    /**
     * @brief Constructor
     * @param date: the date
     * @param file: the DataFile associated with this file read
     * @param src_location: the source location
     * @param service: service requesting file read
     * @param task: a task associated to  this file read (or nullptr)
     */
    SimulationTimestampFileRead::SimulationTimestampFileRead(double date, std::shared_ptr<DataFile> file,
                                                             std::shared_ptr<FileLocation> src_location,
                                                             std::shared_ptr<StorageService> service,
                                                             std::shared_ptr<WorkflowTask> task) : file(std::move(file)), source(std::move(src_location)), service(std::move(std::move(service))), task(std::move(task)) {
        this->date = date;
    }

    /**
     * @brief retrieves the DataFile being read
     * @return a pointer to the DataFile associated with this reads
     */
    std::shared_ptr<DataFile> SimulationTimestampFileRead::getFile() {
        return this->file;
    }

    /**
     * @brief retrieves the location from which the DataFile is being copied
     * @return the read's source location
     */
    std::shared_ptr<FileLocation> SimulationTimestampFileRead::getSource() {
        return this->source;
    }

    /**
     * @brief retrieves the storage service for file read
     * @return point to the service associated with this read
     */
    std::shared_ptr<StorageService> SimulationTimestampFileRead::getService() {
        return this->service;
    }

    /**
     * @brief retrieves task associated w/ file read
     * @return a pointer to the WorkflowTask associated with this timestamp
     */
    std::shared_ptr<WorkflowTask> SimulationTimestampFileRead::getTask() {
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
    std::unordered_multimap<FileReadWrite, std::pair<SimulationTimestampFileRead *, std::shared_ptr<WorkflowTask>>> SimulationTimestampFileRead::pending_file_reads;

    /**
     * @brief Sets the endpoint of the calling object (SimulationTimestampFileReadFailure, SimulationTimestampFileReadTerminated, SimulationTimestampFileReadStart) with a SimulationTimestampFileReadStart object
     */
    void SimulationTimestampFileRead::setEndpoints() {
        // find the SimulationTimestampFileRead object containing the same task
        auto pending_reads_itr = pending_file_reads.find(FileReadWrite(this->file, this->source, this->service));
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
     * @param date: the date
     * @param file: the DataFile associated with this file read
     * @param src: the source location
     * @param service: service requesting file read
     * @param task: a  task associated to  this file read (or nullptr)
     * @throw std::invalid_argument
     */
    SimulationTimestampFileReadStart::SimulationTimestampFileReadStart(double date,
                                                                       std::shared_ptr<DataFile> file,
                                                                       std::shared_ptr<FileLocation> src,
                                                                       std::shared_ptr<StorageService> service,
                                                                       std::shared_ptr<WorkflowTask> task) : SimulationTimestampFileRead(date, std::move(file), std::move(src), std::move(service), std::move(task)) {
        WRENCH_DEBUG("Inserting a FileReadStart timestamp for file read");

        // all information about a file read should be passed
        if ((this->file == nullptr) || (this->source == nullptr) || (this->service == nullptr)) {

            throw std::invalid_argument(
                    "SimulationTimestampFileReadStart::SimulationTimestampFileReadStart() cannot take nullptr arguments");
        }


        pending_file_reads.insert(std::make_pair(FileReadWrite(this->file, this->source, this->service), std::make_pair(this, this->task)));
    }


    /**
     * @brief Constructor
     * @param date: the date
     * @param file: the DataFile associated with this file read
     * @param src: the source location
     * @param service: service requesting file read
     * @param task: a workflow task associated to this file read
     * @throw std::invalid_argument
     */
    SimulationTimestampFileReadFailure::SimulationTimestampFileReadFailure(double date, const std::shared_ptr<DataFile> &file,
                                                                           const std::shared_ptr<FileLocation> &src,
                                                                           const std::shared_ptr<StorageService> &service,
                                                                           std::shared_ptr<WorkflowTask> task) : SimulationTimestampFileRead(date, file, src, service, std::move(task)) {
        WRENCH_DEBUG("Inserting a FileReadFailure timestamp for file read");

        if (file == nullptr || src == nullptr || (service == nullptr)) {
            throw std::invalid_argument(
                    "SimulationTimestampFileReadFailure::SimulationTimestampFileReadFailure() requires a valid pointer to file, source and service objects");
        }

        // match this timestamp with a SimulationTimestampFileReadStart
        setEndpoints();
    }

    /**
     * @brief Constructor
     * @param date: the date
     * @param file: the DataFile associated with this file read
     * @param src: the source location
     * @param service: service requesting file read
     * @param task: a task associated to  this file read (or nullptr)
     * @throw std::invalid_argument
     */
    SimulationTimestampFileReadCompletion::SimulationTimestampFileReadCompletion(double date, const std::shared_ptr<DataFile> &file,
                                                                                 const std::shared_ptr<FileLocation> &src,
                                                                                 const std::shared_ptr<StorageService> &service,
                                                                                 std::shared_ptr<WorkflowTask> task) : SimulationTimestampFileRead(date, file, src, service, std::move(task)) {
        WRENCH_DEBUG("Inserting a FileReadCompletion timestamp for file read");

        if (file == nullptr || src == nullptr || service == nullptr) {
            throw std::invalid_argument(
                    "SimulationTimestampFileReadFailure::SimulationTimestampFileReadFailure() requires a valid pointer to file, source and service objects");
        }

        // match this timestamp with a SimulationTimestampFileReadStart
        setEndpoints();
    }

    /**
     * @brief Constructor
     * @param date: the date
     * @param file: the DataFile associated with this file write
     * @param dst_location: the destination location
     * @param service: service requesting file write
     * @param task: a task associated to  this file read (or nullptr)
     */
    SimulationTimestampFileWrite::SimulationTimestampFileWrite(double date,
                                                               std::shared_ptr<DataFile> file,
                                                               std::shared_ptr<FileLocation> dst_location,
                                                               std::shared_ptr<StorageService> service,
                                                               std::shared_ptr<WorkflowTask> task) : file(std::move(file)), destination(std::move(dst_location)), service(std::move(service)), task(std::move(task)) {
        this->date = date;
    }

    /**
     * @brief retrieves the DataFile being written
     * @return a pointer to the DataFile associated with this writes
     */
    std::shared_ptr<DataFile> SimulationTimestampFileWrite::getFile() {
        return this->file;
    }

    /**
     * @brief retrieves the location from which the DataFile is being copied
     * @return the write's destination location
     */
    std::shared_ptr<FileLocation> SimulationTimestampFileWrite::getDestination() {
        return this->destination;
    }

    /**
     * @brief retrieves the Service that ordered file write
     * @return point to the service associated with this write
     */
    std::shared_ptr<StorageService> SimulationTimestampFileWrite::getService() {
        return this->service;
    }

    /**
     * @brief retrieves task associated w/ file write
     * @return a pointer to the WorkflowTask associated with this timestamp
     */
    std::shared_ptr<WorkflowTask> SimulationTimestampFileWrite::getTask() {
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
    std::unordered_multimap<FileReadWrite, std::pair<SimulationTimestampFileWrite *, std::shared_ptr<WorkflowTask>>> SimulationTimestampFileWrite::pending_file_writes;

    /**
     * @brief Sets the endpoint of the calling object (SimulationTimestampFileWriteFailure, SimulationTimestampFileWriteTerminated, SimulationTimestampFileWriteStart) with a SimulationTimestampFileWriteStart object
     */
    void SimulationTimestampFileWrite::setEndpoints() {
        // find the SimulationTimestampFileWrite object containing the same task
        auto pending_writes_itr = pending_file_writes.find(FileReadWrite(this->file, this->destination, this->service));
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
     * @param date: the date
     * @param file: the DataFile associated with this file write
     * @param dst: the destination location
     * @param service: service requesting file write
     * @param task: a  task associated to  this file read (or nullptr)
     * @throw std::invalid_argument
     */
    SimulationTimestampFileWriteStart::SimulationTimestampFileWriteStart(double date, std::shared_ptr<DataFile> file,
                                                                         std::shared_ptr<FileLocation> dst,
                                                                         std::shared_ptr<StorageService> service,
                                                                         std::shared_ptr<WorkflowTask> task) : SimulationTimestampFileWrite(date,
                                                                                                                                            std::move(file),
                                                                                                                                            std::move(dst),
                                                                                                                                            std::move(service),
                                                                                                                                            std::move(task)) {
        WRENCH_DEBUG("Inserting a FileWriteStart timestamp for file write");

        // all information about a file write should be passed
        if ((this->file == nullptr) || (this->destination == nullptr) || (this->service == nullptr)) {

            throw std::invalid_argument(
                    "SimulationTimestampFileWriteStart::SimulationTimestampFileWriteStart() cannot take nullptr arguments");
        }


        pending_file_writes.insert(std::make_pair(FileReadWrite(this->file, this->destination, this->service), std::make_pair(this, this->task)));
    }


    /**
     * @brief Constructor
     * @param date: the date
     * @param file: the DataFile associated with this file write
     * @param dst: the destination location
     * @param service: service requesting file write
     * @param task: the workflow task
     * @throw std::invalid_argument
     */
    SimulationTimestampFileWriteFailure::SimulationTimestampFileWriteFailure(double date, const std::shared_ptr<DataFile> &file,
                                                                             const std::shared_ptr<FileLocation> &dst,
                                                                             const std::shared_ptr<StorageService> &service,
                                                                             std::shared_ptr<WorkflowTask> task) : SimulationTimestampFileWrite(date, file, dst, service, std::move(task)) {
        WRENCH_DEBUG("Inserting a FileWriteFailure timestamp for file write");

        if (file == nullptr || dst == nullptr || service == nullptr) {
            throw std::invalid_argument(
                    "SimulationTimestampFileWriteFailure::SimulationTimestampFileWriteFailure() requires a valid pointer to file, destination and service objects");
        }

        // match this timestamp with a SimulationTimestampFileWriteStart
        setEndpoints();
    }

    /**
     * @brief Constructor
     * @param date: the date
     * @param file: the DataFile associated with this file write
     * @param dst: the destination location
     * @param service: service requesting file write
     * @param task: a workfow task associated to this file write
     * @throw std::invalid_argument
     */
    SimulationTimestampFileWriteCompletion::SimulationTimestampFileWriteCompletion(double date, const std::shared_ptr<DataFile> &file,
                                                                                   const std::shared_ptr<FileLocation> &dst,
                                                                                   const std::shared_ptr<StorageService> &service,
                                                                                   std::shared_ptr<WorkflowTask> task) : SimulationTimestampFileWrite(date, file, dst, service, std::move(task)) {
        WRENCH_DEBUG("Inserting a FileWriteCompletion timestamp for file write");

        if (file == nullptr || dst == nullptr || service == nullptr) {
            throw std::invalid_argument(
                    "SimulationTimestampFileWriteFailure::SimulationTimestampFileWriteFailure() requires a valid pointer to file, destination and service objects");
        }

        // match this timestamp with a SimulationTimestampFileWriteStart
        setEndpoints();
    }

    /**
 * @brief Constructor
 * @param date: the date
 * @param hostname: hostname being read from
 * @param mount: mountpoint of disk
 * @param bytes: number of bytes read
 * @param counter: An integer ID
 */
    SimulationTimestampDiskRead::SimulationTimestampDiskRead(double date,
                                                             std::string hostname,
                                                             std::string mount,
                                                             double bytes,
                                                             int counter) : hostname(std::move(hostname)), mount(std::move(mount)), bytes(bytes), counter(counter) {
        this->date = date;
    }

    /**
     * @brief retrieves the hostname where read occurs
     * @return string of hostname
     */
    std::string SimulationTimestampDiskRead::getHostname() {
        return this->hostname;
    }

    /**
     * @brief retrieves mount point of read
     * @return string of mount point
     */
    std::string SimulationTimestampDiskRead::getMount() {
        return this->mount;
    }

    /**
     * @brief retrieves the amount of bytes being read
     * @return number of bytes as double
     */
    double SimulationTimestampDiskRead::getBytes() const {
        return this->bytes;
    }

    /**
     * @brief To get counter of disk operation
     * @return int of counter
     */
    int SimulationTimestampDiskRead::getCounter() const {
        return this->counter;
    }

    /**
     * @brief retrieves the corresponding SimulationTimestampDiskRead object
     * @return a pointer to the start or end SimulationTimestampDiskRead object
     */
    SimulationTimestampDiskRead *SimulationTimestampDiskRead::getEndpoint() {
        return dynamic_cast<SimulationTimestampDiskRead *>(this->endpoint);
    }

    /**
     * @brief A static unordered multimap of SimulationTimestampDiskReadStart objects that have yet to be matched with Failure, Terminated or Completion timestamps
     */
    std::unordered_multimap<DiskAccess, SimulationTimestampDiskRead *> SimulationTimestampDiskRead::pending_disk_reads;

    /**
     * @brief Sets the endpoint of the calling object (SimulationTimestampDiskReadFailure, SimulationTimestampDiskReadTerminated, SimulationTimestampDiskReadStart) with a SimulationTimestampDiskReadStart object
     */
    void SimulationTimestampDiskRead::setEndpoints() {
        // find the SimulationTimestampDiskRead object containing the same task
        auto pending_disk_reads_itr = pending_disk_reads.find(DiskAccess(this->hostname, this->mount, this->counter));
        if (pending_disk_reads_itr != pending_disk_reads.end()) {
            // set my endpoint to the SimulationTimestampDiskReadStart
            this->endpoint = (*pending_disk_reads_itr).second;

            // set the SimulationTimestampDiskReadStart's endpoint to me
            (*pending_disk_reads_itr).second->endpoint = this;

            // the SimulationTimestampDiskReadStart is no longer waiting to be matched with an end timestamp, remove it from the map
            pending_disk_reads.erase(pending_disk_reads_itr);
        } else {
            throw std::runtime_error(
                    "SimulationTimestampDiskRead::setEndpoints() could not find a SimulationTimestampDiskReadStart object.");
        }
    }


    /**
     * @brief Constructor
     * @param date: the date
     * @param hostname: hostname of disk being read
     * @param mount: mount point of disk being read
     * @param bytes: number of bytes read
     * @param counter: an integer ID
     * @throw std::invalid_argument
     */
    SimulationTimestampDiskReadStart::SimulationTimestampDiskReadStart(double date, std::string hostname,
                                                                       std::string mount,
                                                                       double bytes,
                                                                       int counter) : SimulationTimestampDiskRead(date, std::move(hostname), std::move(mount), bytes, counter) {
        WRENCH_DEBUG("Inserting a DiskReadStart timestamp for disk read");

        // all information about a disk read should be passed
        if (this->hostname.empty() || this->mount.empty()) {

            throw std::invalid_argument(
                    "SimulationTimestampDiskReadStart::SimulationTimestampDiskReadStart() cannot take nullptr arguments");
        }


        pending_disk_reads.insert(std::make_pair(DiskAccess(this->hostname, this->mount, this->counter), this));
    }


    /**
     * @brief Constructor
     * @param date: the date
     * @param hostname: hostname of disk being read
     * @param mount: mount point of disk being read
     * @param bytes: number of bytes read
     * @param counter: an integer ID
     * @throw std::invalid_argument
     */
    SimulationTimestampDiskReadFailure::SimulationTimestampDiskReadFailure(double date, const std::string& hostname,
                                                                           const std::string& mount,
                                                                           double bytes,
                                                                           int counter) : SimulationTimestampDiskRead(date, hostname, mount, bytes, counter) {
        WRENCH_DEBUG("Inserting a DiskReadFailure timestamp for disk read");

        if (hostname.empty() || mount.empty()) {
            throw std::invalid_argument(
                    "SimulationTimestampDiskReadFailure::SimulationTimestampDiskReadFailure() requires a valid pointer to file, destination and service objects");
        }

        // match this timestamp with a SimulationTimestampDiskReadStart
        setEndpoints();
    }

    /**
     * @brief Constructor
     * @param date: the date
     * @param hostname: hostname of disk being read
     * @param mount: mount point of disk being read
     * @param bytes: number of bytes read
     * @param counter: an integer ID
     * @throw std::invalid_argument
     */
    SimulationTimestampDiskReadCompletion::SimulationTimestampDiskReadCompletion(double date,
                                                                                 const std::string& hostname,
                                                                                 const std::string& mount,
                                                                                 double bytes,
                                                                                 int counter) : SimulationTimestampDiskRead(date, hostname, mount, bytes, counter) {
        WRENCH_DEBUG("Inserting a DiskReadCompletion timestamp for disk read");

        if (hostname.empty() || mount.empty()) {
            throw std::invalid_argument(
                    "SimulationTimestampDiskReadFailure::SimulationTimestampDiskReadFailure() requires a valid pointer to file, destination and service objects");
        }

        // match this timestamp with a SimulationTimestampDiskReadStart
        setEndpoints();
    }

    /**
     * @brief Constructor
     * @param date: the date
     * @param hostname: hostname being write from
     * @param mount: mountpoint of disk
     * @param bytes: number of bytes written
     * @param counter: an integer ID
     */
    SimulationTimestampDiskWrite::SimulationTimestampDiskWrite(double date, std::string hostname,
                                                               std::string mount,
                                                               double bytes,
                                                               int counter) : hostname(std::move(hostname)), mount(std::move(mount)), bytes(bytes), counter(counter) {
        this->date = date;
    }

    /**
     * @brief retrieves the hostname where write occurs
     * @return string of hostname
     */
    std::string SimulationTimestampDiskWrite::getHostname() {
        return this->hostname;
    }

    /**
     * @brief retrieves mount point of write
     * @return string of mount point
     */
    std::string SimulationTimestampDiskWrite::getMount() {
        return this->mount;
    }

    /**
     * @brief retrieves the amount of bytes being written
     * @return number of bytes as double
     */
    double SimulationTimestampDiskWrite::getBytes() const {
        return this->bytes;
    }

    /**
     * @brief retrieves the counter for this disk operation
     * @return int of counter
     */
    int SimulationTimestampDiskWrite::getCounter() const {
        return this->counter;
    }

    /**
     * @brief retrieves the corresponding SimulationTimestampDiskWrite object
     * @return a pointer to the start or end SimulationTimestampDiskWrite object
     */
    SimulationTimestampDiskWrite *SimulationTimestampDiskWrite::getEndpoint() {
        return dynamic_cast<SimulationTimestampDiskWrite *>(this->endpoint);
    }

    /**
     * @brief A static unordered multimap of SimulationTimestampDiskWriteStart objects that have yet to be matched with Failure, Terminated or Completion timestamps
     */
    std::unordered_multimap<DiskAccess, SimulationTimestampDiskWrite *> SimulationTimestampDiskWrite::pending_disk_writes;

    /**
     * @brief Sets the endpoint of the calling object (SimulationTimestampDiskWriteFailure, SimulationTimestampDiskWriteTerminated, SimulationTimestampDiskWriteStart) with a SimulationTimestampDiskWriteStart object
     */
    void SimulationTimestampDiskWrite::setEndpoints() {
        // find the SimulationTimestampDiskWrite object containing the same task
        auto pending_disk_writes_itr = pending_disk_writes.find(DiskAccess(this->hostname, this->mount, this->counter));
        if (pending_disk_writes_itr != pending_disk_writes.end()) {
            // set my endpoint to the SimulationTimestampDiskWriteStart
            this->endpoint = (*pending_disk_writes_itr).second;

            // set the SimulationTimestampDiskWriteStart's endpoint to me
            (*pending_disk_writes_itr).second->endpoint = this;

            // the SimulationTimestampDiskWriteStart is no longer waiting to be matched with an end timestamp, remove it from the map
            pending_disk_writes.erase(pending_disk_writes_itr);
        } else {
            throw std::runtime_error(
                    "SimulationTimestampDiskWrite::setEndpoints() could not find a SimulationTimestampDiskWriteStart object.");
        }
    }


    /**
     * @brief Constructor
     * @param date: the date
     * @param hostname: hostname of disk being write
     * @param mount: mount point of disk being write
     * @param bytes: number of bytes write
     * @param counter: an integer ID
     * @throw std::invalid_argument
     */
    SimulationTimestampDiskWriteStart::SimulationTimestampDiskWriteStart(double date, std::string hostname,
                                                                         std::string mount,
                                                                         double bytes,
                                                                         int counter) : SimulationTimestampDiskWrite(date, std::move(hostname), std::move(mount), bytes, counter) {
        WRENCH_DEBUG("Inserting a DiskWriteStart timestamp for disk write");

        // all information about a disk write should be passed
        if (this->hostname.empty() || this->mount.empty()) {

            throw std::invalid_argument(
                    "SimulationTimestampDiskWriteStart::SimulationTimestampDiskWriteStart() cannot take nullptr arguments");
        }


        pending_disk_writes.insert(std::make_pair(DiskAccess(this->hostname, this->mount, this->counter), this));
    }


    /**
     * @brief Constructor
     * @param date: the date
     * @param hostname: hostname of disk being write
     * @param mount: mount point of disk being write
     * @param bytes: number of bytes write
     * @param counter: an integer ID
     * @throw std::invalid_argument
     */
    SimulationTimestampDiskWriteFailure::SimulationTimestampDiskWriteFailure(double date, const std::string& hostname,
                                                                             const std::string& mount,
                                                                             double bytes,
                                                                             int counter) : SimulationTimestampDiskWrite(date, hostname, mount, bytes, counter) {
        WRENCH_DEBUG("Inserting a DiskWriteFailure timestamp for disk write");

        if (hostname.empty() || mount.empty()) {
            throw std::invalid_argument(
                    "SimulationTimestampDiskWriteFailure::SimulationTimestampDiskWriteFailure() requires a valid pointer to file, destination and service objects");
        }

        // match this timestamp with a SimulationTimestampDiskWriteStart
        setEndpoints();
    }

    /**
     * @brief Constructor
     * @param date: the date
     * @param hostname: hostname of disk being write
     * @param mount: mount point of disk being write
     * @param bytes: number of bytes write
     * @param counter: an integer ID
     * @throw std::invalid_argument
     */
    SimulationTimestampDiskWriteCompletion::SimulationTimestampDiskWriteCompletion(double date, const std::string& hostname,
                                                                                   const std::string& mount,
                                                                                   double bytes,
                                                                                   int counter) : SimulationTimestampDiskWrite(date, hostname, mount, bytes, counter) {
        WRENCH_DEBUG("Inserting a DiskWriteCompletion timestamp for disk write");

        if (hostname.empty() || mount.empty()) {
            throw std::invalid_argument(
                    "SimulationTimestampDiskWriteFailure::SimulationTimestampDiskWriteFailure() requires a valid pointer to file, destination and service objects");
        }

        // match this timestamp with a SimulationTimestampDiskWriteStart
        setEndpoints();
    }


    /**
     * @brief Constructor
     * @param date: the date
     * @param hostname: the host on which a pstate is being set
     * @param pstate: the pstate that is being set on this host
     */
    SimulationTimestampPstateSet::SimulationTimestampPstateSet(double date, const std::string& hostname, int pstate) : hostname(hostname), pstate(pstate) {
        this->date = date;

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
    int SimulationTimestampPstateSet::getPstate() const {
        return this->pstate;
    }

    /**
     * @brief Constructor
     * @param date: the date
     * @param hostname: the host on which this energy consumption timestamp applies to
     * @param joules: the energy consumption in joules 
     */
    SimulationTimestampEnergyConsumption::SimulationTimestampEnergyConsumption(double date, const std::string& hostname, double joules)
            : hostname(hostname), joules(joules) {

        this->date = date;
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
    double SimulationTimestampEnergyConsumption::getConsumption() const {
        return this->joules;
    }

    /**
     * @brief Constructor
     * @param date: the date
     * @param linkname: the link for which this bandwidth usage timestamp applies to
     * @param bytes_per_second: the bandwidth usage in bytes per second
     */
    SimulationTimestampLinkUsage::SimulationTimestampLinkUsage(double date, const std::string& linkname, double bytes_per_second)
            : linkname(linkname), bytes_per_second(bytes_per_second) {
        this->date = date;
        if (linkname.empty() || bytes_per_second < 0.0) {
            throw std::invalid_argument(
                    "SimulationTimestampLinkUsage::SimulationTimestampLinkUsage() requires a valid linkname and a link usage amount >= 0.0");
        }
    }

    /**
     * @brief Get the linkname associated with this timestamp
     * @return the linkname associated with this timestamp
     */
    std::string SimulationTimestampLinkUsage::getLinkname() {
        return this->linkname;
    }

    /**
     * @brief Get the bandwidth usage in bytes per second
     * @return the bandwidth usage in bytes per second
     */
    double SimulationTimestampLinkUsage::getUsage() const {
        return this->bytes_per_second;
    }

}// namespace wrench
