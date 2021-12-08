/**
* Copyright (c) 2017. The WRENCH Team.
*
* This program is free software: you can redistribute it and/or modify
        * it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/

#ifndef WRENCH_SIMULATIONTIMESTAMPTYPES_H
#define WRENCH_SIMULATIONTIMESTAMPTYPES_H


#include "wrench/data_file//DataFile.h"
#include "wrench/workflow/WorkflowTask.h"
#include <unordered_map>

using namespace std;
/***********************/
/** \cond              */
/***********************/

typedef std::tuple<std::shared_ptr<wrench::DataFile>, void *, void *> File;
typedef std::tuple<std::string, std::string, int> DiskAccess;

namespace std {
    template <>
    struct hash<File>{
    public :
        size_t operator()(const File &file ) const
        {
            return std::hash<void *>()(std::get<0>(file).get()) ^ std::hash<void *>()(std::get<1>(file)) ^ std::hash<void *>()(std::get<2>(file));
        }
    };

    template <>
    struct hash<DiskAccess>{
    public :
        size_t operator()(const DiskAccess &diskAccess ) const
        {
            return std::hash<std::string>()(std::get<0>(diskAccess)) ^ std::hash<std::string>()(std::get<1>(diskAccess)) ^ std::hash<int>()(std::get<2>(diskAccess));
        }
    };
};

/***********************/
/** \endcond           */
/***********************/

namespace wrench {


    class WorkflowTask;
    class StorageService;
    class FileLocation;

    /**
     * @brief File, Source, Whoami used to be hashed as key for unordered multimap for ongoing file operations.
     */
    ///typedef std::tuple<std::shared_ptr<DataFile>, FileLocation *, StorageService *> File;

    /**
     *
     * @param file - tuple of three strings relating to File, Source and Whoami
     * @return XOR of hashes of file
     */
    ///size_t file_hash( const File & file );

    /**
     * @brief A top-level base class for simulation timestamps
     */
    class SimulationTimestampType {
    public:
        SimulationTimestampType();
        double getDate();

    protected:
        /**
         * @brief Date
         */
        double date = -0.1;

    private:
        friend  class SimulationOutput;
        void setDate(double date);
    };

    /**
     * @brief A base class for simulation timestamps
     */
    class SimulationTimestampPair : public SimulationTimestampType {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        SimulationTimestampPair();
        SimulationTimestampPair(double date, SimulationTimestampPair *endpoint);
        virtual ~SimulationTimestampPair() = default;
        /***********************/
        /** \endcond           */
        /***********************/

        virtual SimulationTimestampPair *getEndpoint();

    protected:
        /** @brief An optional associated "endpoint" simulation timestamp */
        SimulationTimestampPair *endpoint = nullptr;
    };

    /**
    * @brief A base class for simulation timestamps regarding workflow tasks
    */
    class SimulationTimestampTask : public SimulationTimestampPair {

    public:
        std::shared_ptr<WorkflowTask>getTask();
        SimulationTimestampTask *getEndpoint() override;

    protected:
        static std::map<std::string, SimulationTimestampTask *> pending_task_timestamps;
        void setEndpoints();
        SimulationTimestampTask(double date, std::shared_ptr<WorkflowTask>task);

    private:
        friend class SimulationOutput;
        std::shared_ptr<WorkflowTask>task;
    };

    /**
     * @brief A simulation timestamp class for WorkflowTask start times
     */
    class SimulationTimestampTaskStart : public SimulationTimestampTask {
    private:
        friend class SimulationOutput;
        SimulationTimestampTaskStart(double date, std::shared_ptr<WorkflowTask>task);
    };

    /**
     * @brief A simulation timestamp class for WorkflowTask failure times
     */
    class SimulationTimestampTaskFailure : public SimulationTimestampTask {
    private:
        friend class SimulationOutput;
        SimulationTimestampTaskFailure(double date, std::shared_ptr<WorkflowTask>task);
    };

    /**
     * @brief A simulation timestamp class for WorkflowTask completion times
     */
    class SimulationTimestampTaskCompletion : public SimulationTimestampTask {
    private:
        friend class SimulationOutput;
        SimulationTimestampTaskCompletion(double date, std::shared_ptr<WorkflowTask>task);
    };

    /**
     * @brief A simulation timestamp class for WorkflowTask termination times
     */
     class SimulationTimestampTaskTermination : public SimulationTimestampTask {
     private:
         friend class SimulationOutput;
         SimulationTimestampTaskTermination(double date, std::shared_ptr<WorkflowTask>);
     };

    class SimulationTimestampFileReadStart;


    /**
     * @brief A base class for simulation timestamps regarding file reads
     */
    class SimulationTimestampFileRead : public SimulationTimestampPair {
    public:

        /**
         * @brief Retrieve the matching endpoint, if any
         */
        SimulationTimestampFileRead *getEndpoint() override;
        std::shared_ptr<DataFile>getFile();
        FileLocation *getSource();
        StorageService *getService();
        std::shared_ptr<WorkflowTask>getTask();

    protected:
        /**
         * @brief The DataFile that was being read
         */
        std::shared_ptr<DataFile>file;

        /**
         * @brief The location where the DataFile was being read from
         */
        FileLocation *source;

        /**
         * @brief Service that initiated the read
         */
        StorageService *service;

        /**
         * @brief Task tied to read
         */
         std::shared_ptr<WorkflowTask>task;

        /**
         * @brief the data structure that holds the ongoing file reads.
         */
        ///static std::unordered_multimap<File, std::pair<SimulationTimestampFileRead *, double>, decltype(&file_hash)> pending_file_reads;
        static std::unordered_multimap<File, std::pair<SimulationTimestampFileRead *, std::shared_ptr<WorkflowTask>>> pending_file_reads;

        void setEndpoints();
        friend class SimulationOutput;
        SimulationTimestampFileRead(double date, std::shared_ptr<DataFile>file, FileLocation *src, StorageService *service, std::shared_ptr<WorkflowTask>task = nullptr);
    };

    class SimulationTimestampFileReadFailure;
    class SimulationTimestampFileReadCompletion;

    /**
     * @brief A simulation timestamp class for file read start times
     */
    class SimulationTimestampFileReadStart : public SimulationTimestampFileRead {
    public:
        friend class SimulationOutput;
        SimulationTimestampFileReadStart(double date, std::shared_ptr<DataFile>file, FileLocation *src, StorageService *service, std::shared_ptr<WorkflowTask>task = nullptr);

        friend class SimulationTimestampFileReadFailure;
        friend class SimulationTimestampFileReadCompletion;
    };

    /**
     * @brief A simulation timestamp class for file read failure times
     */
    class SimulationTimestampFileReadFailure : public SimulationTimestampFileRead {
    private:
        friend class SimulationOutput;
        SimulationTimestampFileReadFailure(double date, std::shared_ptr<DataFile>file, FileLocation *src, StorageService *service, std::shared_ptr<WorkflowTask>task = nullptr);
    };

    /**
     * @brief A simulation timestamp class for file read completions
     */
    class SimulationTimestampFileReadCompletion : public SimulationTimestampFileRead {
    private:
        friend class SimulationOutput;
        SimulationTimestampFileReadCompletion(double date, std::shared_ptr<DataFile>file, FileLocation *src, StorageService *service, std::shared_ptr<WorkflowTask>task = nullptr);
    };

    class SimulationTimestampFileWriteStart;


    /**
     * @brief A base class for simulation timestamps regarding file writes
     */
    class SimulationTimestampFileWrite : public SimulationTimestampPair {
    public:

        /**
         * @brief Retrieve the matching endpoint, if any
         */
        SimulationTimestampFileWrite *getEndpoint() override;
        std::shared_ptr<DataFile>getFile();
        FileLocation *getDestination();
        StorageService *getService();
        std::shared_ptr<WorkflowTask>getTask();

    protected:
        /**
         * @brief The DataFile that was being write
         */
        std::shared_ptr<DataFile>file;

        /**
         * @brief The location where the DataFile was being write from
         */
        FileLocation *destination;

        /**
         * @brief Service that initiated the write
         */
        StorageService *service;

        /**
         * @brief Task associated with write
         */
         std::shared_ptr<WorkflowTask>task;

        /**
         * @brief the data structure that holds the ongoing file writes.
         */
        ///static std::unordered_multimap<File, std::pair<SimulationTimestampFileWrite *, double>, decltype(&file_hash)> pending_file_writes;
        static std::unordered_multimap<File, std::pair<SimulationTimestampFileWrite *, std::shared_ptr<WorkflowTask>>> pending_file_writes;

        void setEndpoints();

        friend class SimulationOutput;
        SimulationTimestampFileWrite(double date, std::shared_ptr<DataFile>file, FileLocation *dst, StorageService *service, std::shared_ptr<WorkflowTask>task = nullptr);
    };

    class SimulationTimestampFileWriteFailure;
    class SimulationTimestampFileWriteCompletion;

    /**
     * @brief A simulation timestamp class for file write start times
     */
    class SimulationTimestampFileWriteStart : public SimulationTimestampFileWrite {
    private:
        friend class SimulationOutput;
        friend class SimulationTimestampFileWriteFailure;
        friend class SimulationTimestampFileWriteCompletion;
        SimulationTimestampFileWriteStart(double date, std::shared_ptr<DataFile>file, FileLocation *dst, StorageService *service, std::shared_ptr<WorkflowTask>task = nullptr);
    };

    /**
     * @brief A simulation timestamp class for file write failure times
     */
    class SimulationTimestampFileWriteFailure : public SimulationTimestampFileWrite {
    private:
        friend class SimulationOutput;
        SimulationTimestampFileWriteFailure(double date, std::shared_ptr<DataFile>file, FileLocation *dst, StorageService *service, std::shared_ptr<WorkflowTask>task = nullptr);
    };

    /**
     * @brief A simulation timestamp class for file write completions
     */
    class SimulationTimestampFileWriteCompletion : public SimulationTimestampFileWrite {
    private:
        friend class SimulationOutput;
        SimulationTimestampFileWriteCompletion(double date, std::shared_ptr<DataFile>file, FileLocation *dst, StorageService *service, std::shared_ptr<WorkflowTask>task = nullptr);
    };

    class SimulationTimestampFileCopyStart;
    class FileLocation;

    /**
     * @brief A base class for simulation timestamps regarding file copies
     */
    class SimulationTimestampFileCopy : public SimulationTimestampPair {
    public:

        /**
         * @brief Retrieve the matching endpoint, if any
         */
        SimulationTimestampFileCopy *getEndpoint() override;
        std::shared_ptr<DataFile>getFile();
        std::shared_ptr<FileLocation> getSource();
        std::shared_ptr<FileLocation> getDestination();

    protected:
        SimulationTimestampFileCopy(double date, std::shared_ptr<DataFile> file, std::shared_ptr<FileLocation>  src, std::shared_ptr<FileLocation> dst);

        /**
         * @brief The DataFile that was being copied
         */
        std::shared_ptr<DataFile> file;

        /**
         * @brief The location where the DataFile was being copied from
         */
        std::shared_ptr<FileLocation> source;

        /**
         * @brief The location where the DataFile was being copied to
         */
        std::shared_ptr<FileLocation> destination;

        /**
         * @brief the data structure that holds the ongoing file writes.
         */
         static std::unordered_multimap<File, SimulationTimestampFileCopy *> pending_file_copies;

        void setEndpoints();
    };

    class SimulationTimestampFileCopyFailure;
    class SimulationTimestampFileCopyCompletion;

    /**
     * @brief A simulation timestamp class for file copy start times
     */
    class SimulationTimestampFileCopyStart : public SimulationTimestampFileCopy {
    private:
        friend class SimulationOutput;
        friend class SimulationTimestampFileCopyFailure;
        friend class SimulationTimestampFileCopyCompletion;
        SimulationTimestampFileCopyStart(double date, std::shared_ptr<DataFile>file, std::shared_ptr<FileLocation> src, std::shared_ptr<FileLocation> dst);
    };

    /**
     * @brief A simulation timestamp class for file copy failure times
     */
    class SimulationTimestampFileCopyFailure : public SimulationTimestampFileCopy {
    private:
        friend class SimulationOutput;
        SimulationTimestampFileCopyFailure(double date, std::shared_ptr<DataFile>file, std::shared_ptr<FileLocation>  src, std::shared_ptr<FileLocation> dst);
    };

    /**
     * @brief A simulation timestamp class for file copy completions
     */
    class SimulationTimestampFileCopyCompletion : public SimulationTimestampFileCopy {
    private:
        friend class SimulationOutput;
        SimulationTimestampFileCopyCompletion(double date, std::shared_ptr<DataFile>file, std::shared_ptr<FileLocation>  src, std::shared_ptr<FileLocation> dst);
    };

    class SimulationTimestampDiskReadStart;


    /**
     * @brief A base class for simulation timestamps regarding disk reads
     */
    class SimulationTimestampDiskRead : public SimulationTimestampPair {
    public:

        /**
         * @brief Retrieve the matching endpoint, if any
         */
        SimulationTimestampDiskRead *getEndpoint() override;
        double getBytes();
        std::string getHostname();
        std::string getMount();
        int getCounter();

    protected:
        /**
         * @brief hostname of disk being read from
         */
        std::string hostname;

        /**
         * @brief mount point of disk being read from
         */
        std::string mount;

        /**
         * @brief amount of bytes being read
         */
        double bytes;

        /**
         * @brief counter to differentiate reads
         */
        int counter;


        /**
         * @brief the data structure that holds the ongoing disk reads.
         */
        static std::unordered_multimap<DiskAccess, SimulationTimestampDiskRead *> pending_disk_reads;

        void setEndpoints();
        friend class SimulationOutput;
        SimulationTimestampDiskRead(double date, std::string hostname, std::string mount, double bytes, int counter);
    };

    class SimulationTimestampDiskReadFailure;
    class SimulationTimestampDiskReadCompletion;

    /**
     * @brief A simulation timestamp class for disk read start times
     */
    class SimulationTimestampDiskReadStart : public SimulationTimestampDiskRead {
    public:
        friend class SimulationOutput;
        SimulationTimestampDiskReadStart(double date, std::string hostname, std::string mount, double bytes, int counter);

        friend class SimulationTimestampDiskReadFailure;
        friend class SimulationTimestampDiskReadCompletion;
    };

    /**
     * @brief A simulation timestamp class for disk read failure times
     */
    class SimulationTimestampDiskReadFailure : public SimulationTimestampDiskRead {
    private:
        friend class SimulationOutput;
        SimulationTimestampDiskReadFailure(double date, std::string hostname, std::string mount, double bytes, int counter);
    };

    /**
     * @brief A simulation timestamp class for disk read completions
     */
    class SimulationTimestampDiskReadCompletion : public SimulationTimestampDiskRead {
    private:
        friend class SimulationOutput;
        SimulationTimestampDiskReadCompletion(double date, std::string hostname, std::string mount, double bytes, int counter);
    };

    class SimulationTimestampDiskWriteStart;


    /**
     * @brief A base class for simulation timestamps regarding disk writes
     */
    class SimulationTimestampDiskWrite : public SimulationTimestampPair {
    public:

        /**
         * @brief Retrieve the matching endpoint, if any
         */
        SimulationTimestampDiskWrite *getEndpoint() override;
        double getBytes();
        std::string getHostname();
        std::string getMount();
        int getCounter();

    protected:

        /**
         * @brief hostname of disk being written to
         */
        std::string hostname;

        /**
         * @brief mount point of disk being written to
         */
        std::string mount;

        /**
         * @brief amount of bytes being written
         */
        double bytes;

        /**
         * @brief counter to differentiate writes
         */
        int counter;

        /**
         * @brief the data structure that holds the ongoing disk writes.
         */
        static std::unordered_multimap<DiskAccess, SimulationTimestampDiskWrite *> pending_disk_writes;

        void setEndpoints();
        friend class SimulationOutput;
        SimulationTimestampDiskWrite(double date, std::string hostname, std::string mount, double bytes, int counter);
    };

    class SimulationTimestampDiskWriteFailure;
    class SimulationTimestampDiskWriteCompletion;

    /**
     * @brief A simulation timestamp class for disk write start times
     */
    class SimulationTimestampDiskWriteStart : public SimulationTimestampDiskWrite {
    public:
        friend class SimulationOutput;
        SimulationTimestampDiskWriteStart(double date, std::string hostname, std::string mount, double bytes, int counter);

        friend class SimulationTimestampDiskWriteFailure;
        friend class SimulationTimestampDiskWriteCompletion;
    };

    /**
     * @brief A simulation timestamp class for disk write failure times
     */
    class SimulationTimestampDiskWriteFailure : public SimulationTimestampDiskWrite {
    private:
        friend class SimulationOutput;
        SimulationTimestampDiskWriteFailure(double date, std::string hostname, std::string mount, double bytes, int counter);
    };

    /**
     * @brief A simulation timestamp class for disk write completions
     */
    class SimulationTimestampDiskWriteCompletion : public SimulationTimestampDiskWrite {
    private:
        friend class SimulationOutput;
        SimulationTimestampDiskWriteCompletion(double date, std::string hostname, std::string mount, double bytes, int counter);
    };

    /**
     * @brief A simulation timestamp class for changes in a host's pstate
     */
    class SimulationTimestampPstateSet : public SimulationTimestampType {
    public:
        std::string getHostname();
        int getPstate();

    private:
        friend class SimulationOutput;
        SimulationTimestampPstateSet(double date, std::string hostname, int pstate);
        std::string hostname;
        int pstate;
    };

    /**
     * @brief A simulation timestamp class for energy consumption
     */
    class SimulationTimestampEnergyConsumption: public SimulationTimestampType {
    public:
        std::string getHostname();
        double getConsumption();

    private:
        friend class SimulationOutput;
        SimulationTimestampEnergyConsumption(double date, std::string hostname, double joules);
        std::string hostname;
        double joules;
    };

    /**
     * @brief A simulation timestamp class for link usage
     */
    class SimulationTimestampLinkUsage: public SimulationTimestampType {
    public:
        std::string getLinkname();
        double getUsage();

    private:
        friend class SimulationOutput;
        SimulationTimestampLinkUsage(double date, std::string linkname, double bytes_per_second);
        std::string linkname;
        double bytes_per_second;
    };
};

#endif //WRENCH_SIMULATIONTIMESTAMPTYPES_H
