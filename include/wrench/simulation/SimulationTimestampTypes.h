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


#include "wrench/workflow/WorkflowTask.h"
#include <unordered_map>

using namespace std;
/***********************/
/** \cond              */
/***********************/

typedef std::tuple<void *, void *, void *> File;
typedef std::tuple<std::string, std::string, int> DiskAccess;

namespace std {
    template <>
    struct hash<File>{
    public :
        size_t operator()(const File &file ) const
        {
            return std::hash<void *>()(std::get<0>(file)) ^ std::hash<void *>()(std::get<1>(file)) ^ std::hash<void *>()(std::get<2>(file));
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
    ///typedef std::tuple<WorkflowFile *, FileLocation *, StorageService *> File;

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

    private:
        double date = -0.1;
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
        SimulationTimestampPair(SimulationTimestampPair *endpoint);
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
        WorkflowTask *getTask();
        SimulationTimestampTask *getEndpoint() override;

    protected:
        static std::map<std::string, SimulationTimestampTask *> pending_task_timestamps;
        void setEndpoints();
        explicit SimulationTimestampTask(WorkflowTask *);

    private:
        friend class SimulationOutput;
        WorkflowTask *task;
    };

    /**
     * @brief A simulation timestamp class for WorkflowTask start times
     */
    class SimulationTimestampTaskStart : public SimulationTimestampTask {
    private:
        friend class SimulationOutput;
        explicit SimulationTimestampTaskStart(WorkflowTask *task);
    };

    /**
     * @brief A simulation timestamp class for WorkflowTask failure times
     */
    class SimulationTimestampTaskFailure : public SimulationTimestampTask {
    private:
        friend class SimulationOutput;
        explicit SimulationTimestampTaskFailure(WorkflowTask *task);
    };

    /**
     * @brief A simulation timestamp class for WorkflowTask completion times
     */
    class SimulationTimestampTaskCompletion : public SimulationTimestampTask {
    private:
        friend class SimulationOutput;
        explicit SimulationTimestampTaskCompletion(WorkflowTask *);
    };

    /**
     * @brief A simulation timestamp class for WorkflowTask termination times
     */
     class SimulationTimestampTaskTermination : public SimulationTimestampTask {
     private:
         friend class SimulationOutput;
         explicit SimulationTimestampTaskTermination(WorkflowTask *);
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
        WorkflowFile *getFile();
        FileLocation *getSource();
        StorageService *getService();
        WorkflowTask *getTask();

    protected:
        /**
         * @brief The WorkflowFile that was being read
         */
        WorkflowFile *file;

        /**
         * @brief The location where the WorkflowFile was being read from
         */
        FileLocation *source;

        /**
         * @brief Service that initiated the read
         */
        StorageService *service;

        /**
         * @brief Task tied to read
         */
         WorkflowTask *task;

        /**
         * @brief the data structure that holds the ongoing file reads.
         */
        ///static std::unordered_multimap<File, std::pair<SimulationTimestampFileRead *, double>, decltype(&file_hash)> pending_file_reads;
        static std::unordered_multimap<File, std::pair<SimulationTimestampFileRead *, WorkflowTask *>> pending_file_reads;

        void setEndpoints();
        friend class SimulationOutput;
        SimulationTimestampFileRead(WorkflowFile *file, FileLocation *src, StorageService *service, WorkflowTask *task = nullptr);
    };

    class SimulationTimestampFileReadFailure;
    class SimulationTimestampFileReadCompletion;

    /**
     * @brief A simulation timestamp class for file read start times
     */
    class SimulationTimestampFileReadStart : public SimulationTimestampFileRead {
    public:
        friend class SimulationOutput;
        SimulationTimestampFileReadStart(WorkflowFile *file, FileLocation *src, StorageService *service, WorkflowTask *task = nullptr);

        friend class SimulationTimestampFileReadFailure;
        friend class SimulationTimestampFileReadCompletion;
    };

    /**
     * @brief A simulation timestamp class for file read failure times
     */
    class SimulationTimestampFileReadFailure : public SimulationTimestampFileRead {
    private:
        friend class SimulationOutput;
        SimulationTimestampFileReadFailure(WorkflowFile *file, FileLocation *src, StorageService *service, WorkflowTask *task = nullptr);
    };

    /**
     * @brief A simulation timestamp class for file read completions
     */
    class SimulationTimestampFileReadCompletion : public SimulationTimestampFileRead {
    private:
        friend class SimulationOutput;
        SimulationTimestampFileReadCompletion(WorkflowFile *file, FileLocation *src, StorageService *service, WorkflowTask *task = nullptr);
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
        WorkflowFile *getFile();
        FileLocation *getDestination();
        StorageService *getService();
        WorkflowTask *getTask();

    protected:
        /**
         * @brief The WorkflowFile that was being write
         */
        WorkflowFile *file;

        /**
         * @brief The location where the WorkflowFile was being write from
         */
        FileLocation *destination;

        /**
         * @brief Service that initiated the write
         */
        StorageService *service;

        /**
         * @brief Task associated with write
         */
         WorkflowTask *task;

        /**
         * @brief the data structure that holds the ongoing file writes.
         */
        ///static std::unordered_multimap<File, std::pair<SimulationTimestampFileWrite *, double>, decltype(&file_hash)> pending_file_writes;
        static std::unordered_multimap<File, std::pair<SimulationTimestampFileWrite *, WorkflowTask *>> pending_file_writes;

        void setEndpoints();

        friend class SimulationOutput;
        SimulationTimestampFileWrite(WorkflowFile *file, FileLocation *dst, StorageService *service, WorkflowTask *task = nullptr);
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
        SimulationTimestampFileWriteStart(WorkflowFile *file, FileLocation *dst, StorageService *service, WorkflowTask *task = nullptr);
    };

    /**
     * @brief A simulation timestamp class for file write failure times
     */
    class SimulationTimestampFileWriteFailure : public SimulationTimestampFileWrite {
    private:
        friend class SimulationOutput;
        SimulationTimestampFileWriteFailure(WorkflowFile *file, FileLocation *dst, StorageService *service, WorkflowTask *task = nullptr);
    };

    /**
     * @brief A simulation timestamp class for file write completions
     */
    class SimulationTimestampFileWriteCompletion : public SimulationTimestampFileWrite {
    private:
        friend class SimulationOutput;
        SimulationTimestampFileWriteCompletion(WorkflowFile *file, FileLocation *dst, StorageService *service, WorkflowTask *task = nullptr);
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
        WorkflowFile *getFile();
        std::shared_ptr<FileLocation> getSource();
        std::shared_ptr<FileLocation> getDestination();

    protected:
        SimulationTimestampFileCopy(WorkflowFile *file, std::shared_ptr<FileLocation>  src, std::shared_ptr<FileLocation> dst);

        /**
         * @brief The WorkflowFile that was being copied
         */
        WorkflowFile *file;

        /**
         * @brief The location where the WorkflowFile was being copied from
         */
        std::shared_ptr<FileLocation> source;

        /**
         * @brief The location where the WorkflowFile was being copied to
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
        SimulationTimestampFileCopyStart(WorkflowFile *file, std::shared_ptr<FileLocation> src, std::shared_ptr<FileLocation> dst);
    };

    /**
     * @brief A simulation timestamp class for file copy failure times
     */
    class SimulationTimestampFileCopyFailure : public SimulationTimestampFileCopy {
    private:
        friend class SimulationOutput;
        SimulationTimestampFileCopyFailure(WorkflowFile *file, std::shared_ptr<FileLocation>  src, std::shared_ptr<FileLocation> dst);
    };

    /**
     * @brief A simulation timestamp class for file copy completions
     */
    class SimulationTimestampFileCopyCompletion : public SimulationTimestampFileCopy {
    private:
        friend class SimulationOutput;
        SimulationTimestampFileCopyCompletion(WorkflowFile *file, std::shared_ptr<FileLocation>  src, std::shared_ptr<FileLocation> dst);
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
        SimulationTimestampDiskRead(std::string hostname, std::string mount, double bytes, int counter);
    };

    class SimulationTimestampDiskReadFailure;
    class SimulationTimestampDiskReadCompletion;

    /**
     * @brief A simulation timestamp class for disk read start times
     */
    class SimulationTimestampDiskReadStart : public SimulationTimestampDiskRead {
    public:
        friend class SimulationOutput;
        SimulationTimestampDiskReadStart(std::string hostname, std::string mount, double bytes, int counter);

        friend class SimulationTimestampDiskReadFailure;
        friend class SimulationTimestampDiskReadCompletion;
    };

    /**
     * @brief A simulation timestamp class for disk read failure times
     */
    class SimulationTimestampDiskReadFailure : public SimulationTimestampDiskRead {
    private:
        friend class SimulationOutput;
        SimulationTimestampDiskReadFailure(std::string hostname, std::string mount, double bytes, int counter);
    };

    /**
     * @brief A simulation timestamp class for disk read completions
     */
    class SimulationTimestampDiskReadCompletion : public SimulationTimestampDiskRead {
    private:
        friend class SimulationOutput;
        SimulationTimestampDiskReadCompletion(std::string hostname, std::string mount, double bytes, int counter);
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
        SimulationTimestampDiskWrite(std::string hostname, std::string mount, double bytes, int counter);
    };

    class SimulationTimestampDiskWriteFailure;
    class SimulationTimestampDiskWriteCompletion;

    /**
     * @brief A simulation timestamp class for disk write start times
     */
    class SimulationTimestampDiskWriteStart : public SimulationTimestampDiskWrite {
    public:
        friend class SimulationOutput;
        SimulationTimestampDiskWriteStart(std::string hostname, std::string mount, double bytes, int counter);

        friend class SimulationTimestampDiskWriteFailure;
        friend class SimulationTimestampDiskWriteCompletion;
    };

    /**
     * @brief A simulation timestamp class for disk write failure times
     */
    class SimulationTimestampDiskWriteFailure : public SimulationTimestampDiskWrite {
    private:
        friend class SimulationOutput;
        SimulationTimestampDiskWriteFailure(std::string hostname, std::string mount, double bytes, int counter);
    };

    /**
     * @brief A simulation timestamp class for disk write completions
     */
    class SimulationTimestampDiskWriteCompletion : public SimulationTimestampDiskWrite {
    private:
        friend class SimulationOutput;
        SimulationTimestampDiskWriteCompletion(std::string hostname, std::string mount, double bytes, int counter);
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
        SimulationTimestampPstateSet(std::string hostname, int pstate);
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
        SimulationTimestampEnergyConsumption(std::string hostname, double joules);
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
        SimulationTimestampLinkUsage(std::string linkname, double bytes_per_second);
        std::string linkname;
        double bytes_per_second;
    };
};

#endif //WRENCH_SIMULATIONTIMESTAMPTYPES_H
