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

typedef std::tuple<void *, void *, void *> File;

namespace std {
    template <>
    struct hash<File>{
    public :
        size_t operator()(const File &file ) const
        {
            return std::hash<void *>()(std::get<0>(file)) ^ std::hash<void *>()(std::get<1>(file)) ^ std::hash<void *>()(std::get<2>(file));
        }
    };
};

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
        virtual ~SimulationTimestampPair() {}
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

        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        SimulationTimestampTask(WorkflowTask *);
        /***********************/
        /** \endcond           */
        /***********************/

        WorkflowTask *getTask();
        SimulationTimestampTask *getEndpoint() override;

    protected:
        static std::map<std::string, SimulationTimestampTask *> pending_task_timestamps;

        void setEndpoints();

    private:
        WorkflowTask *task;
    };

    /**
     * @brief A simulation timestamp class for WorkflowTask start times
     */
    class SimulationTimestampTaskStart : public SimulationTimestampTask {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        SimulationTimestampTaskStart(WorkflowTask *);
        /***********************/
        /** \endcond           */
        /***********************/
    };

    /**
     * @brief A simulation timestamp class for WorkflowTask failure times
     */
    class SimulationTimestampTaskFailure : public SimulationTimestampTask {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        SimulationTimestampTaskFailure(WorkflowTask *);
        /***********************/
        /** \endcond           */
        /***********************/

    };

    /**
     * @brief A simulation timestamp class for WorkflowTask completion times
     */
    class SimulationTimestampTaskCompletion : public SimulationTimestampTask {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        SimulationTimestampTaskCompletion(WorkflowTask *);
        /***********************/
        /** \endcond           */
        /***********************/
    };

    /**
     * @brief A simulation timestamp class for WorkflowTask termination times
     */
     class SimulationTimestampTaskTermination : public SimulationTimestampTask {
     public:
         /***********************/
         /** \cond INTERNAL     */
         /***********************/
         SimulationTimestampTaskTermination(WorkflowTask *);
         /***********************/
         /** \endcond           */
         /***********************/
     };

    class SimulationTimestampFileReadStart;


    /**
     * @brief A base class for simulation timestamps regarding file reads
     */
    class SimulationTimestampFileRead : public SimulationTimestampPair {
    public:

        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        SimulationTimestampFileRead(WorkflowFile *file, FileLocation *src, StorageService *service, WorkflowTask *task = nullptr);
        /***********************/
        /** \endcond           */
        /***********************/

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
    };

    class SimulationTimestampFileReadFailure;
    class SimulationTimestampFileReadCompletion;

    /**
     * @brief A simulation timestamp class for file read start times
     */
    class SimulationTimestampFileReadStart : public SimulationTimestampFileRead {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        SimulationTimestampFileReadStart(WorkflowFile *file, FileLocation *src, StorageService *service, WorkflowTask *task = nullptr);
        /***********************/
        /** \endcond           */
        /***********************/

        friend class SimulationTimestampFileReadFailure;
        friend class SimulationTimestampFileReadCompletion;
    };

    /**
     * @brief A simulation timestamp class for file read failure times
     */
    class SimulationTimestampFileReadFailure : public SimulationTimestampFileRead {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        SimulationTimestampFileReadFailure(WorkflowFile *file, FileLocation *src, StorageService *service, WorkflowTask *task = nullptr);
        /***********************/
        /** \endcond           */
        /***********************/
    };

    /**
     * @brief A simulation timestamp class for file read completions
     */
    class SimulationTimestampFileReadCompletion : public SimulationTimestampFileRead {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        SimulationTimestampFileReadCompletion(WorkflowFile *file, FileLocation *src, StorageService *service, WorkflowTask *task = nullptr);
        /***********************/
        /** \endcond           */
        /***********************/
    };

    class SimulationTimestampFileWriteStart;


    /**
     * @brief A base class for simulation timestamps regarding file writes
     */
    class SimulationTimestampFileWrite : public SimulationTimestampPair {
    public:

        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        SimulationTimestampFileWrite(WorkflowFile *file, FileLocation *dst, StorageService *service, WorkflowTask *task = nullptr);
        /***********************/
        /** \endcond           */
        /***********************/

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
    };

    class SimulationTimestampFileWriteFailure;
    class SimulationTimestampFileWriteCompletion;

    /**
     * @brief A simulation timestamp class for file write start times
     */
    class SimulationTimestampFileWriteStart : public SimulationTimestampFileWrite {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        SimulationTimestampFileWriteStart(WorkflowFile *file, FileLocation *dst, StorageService *service, WorkflowTask *task = nullptr);
        /***********************/
        /** \endcond           */
        /***********************/

        friend class SimulationTimestampFileWriteFailure;
        friend class SimulationTimestampFileWriteCompletion;
    };

    /**
     * @brief A simulation timestamp class for file write failure times
     */
    class SimulationTimestampFileWriteFailure : public SimulationTimestampFileWrite {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        SimulationTimestampFileWriteFailure(WorkflowFile *file, FileLocation *dst, StorageService *service, WorkflowTask *task = nullptr);
        /***********************/
        /** \endcond           */
        /***********************/
    };

    /**
     * @brief A simulation timestamp class for file write completions
     */
    class SimulationTimestampFileWriteCompletion : public SimulationTimestampFileWrite {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        SimulationTimestampFileWriteCompletion(WorkflowFile *file, FileLocation *dst, StorageService *service, WorkflowTask *task = nullptr);
        /***********************/
        /** \endcond           */
        /***********************/
    };

    class SimulationTimestampFileCopyStart;
    class FileLocation;

    /**
     * @brief A base class for simulation timestamps regarding file copies
     */
    class SimulationTimestampFileCopy : public SimulationTimestampPair {
    public:

        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        SimulationTimestampFileCopy(WorkflowFile *file, std::shared_ptr<FileLocation>  src, std::shared_ptr<FileLocation> dst);
        /***********************/
        /** \endcond           */
        /***********************/

        /**
         * @brief Retrieve the matching endpoint, if any
         */
        SimulationTimestampFileCopy *getEndpoint() override;
        WorkflowFile *getFile();
        std::shared_ptr<FileLocation> getSource();
        std::shared_ptr<FileLocation> getDestination();

    protected:
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
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        SimulationTimestampFileCopyStart(WorkflowFile *file, std::shared_ptr<FileLocation> src, std::shared_ptr<FileLocation> dst);
        /***********************/
        /** \endcond           */
        /***********************/

        friend class SimulationTimestampFileCopyFailure;
        friend class SimulationTimestampFileCopyCompletion;
    };

    /**
     * @brief A simulation timestamp class for file copy failure times
     */
    class SimulationTimestampFileCopyFailure : public SimulationTimestampFileCopy {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        SimulationTimestampFileCopyFailure(WorkflowFile *file, std::shared_ptr<FileLocation>  src, std::shared_ptr<FileLocation> dst);
        /***********************/
        /** \endcond           */
        /***********************/
    };

    /**
     * @brief A simulation timestamp class for file copy completions
     */
    class SimulationTimestampFileCopyCompletion : public SimulationTimestampFileCopy {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        SimulationTimestampFileCopyCompletion(WorkflowFile *file, std::shared_ptr<FileLocation>  src, std::shared_ptr<FileLocation> dst);
        /***********************/
        /** \endcond           */
        /***********************/
    };




    /**
     * @brief A simulation timestamp class for changes in a host's pstate
     */
    class SimulationTimestampPstateSet : public SimulationTimestampType {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        SimulationTimestampPstateSet(std::string hostname, int pstate);
        /***********************/
        /** \endcond           */
        /***********************/

        std::string getHostname();
        int getPstate();
    private:
        std::string hostname;
        int pstate;
    };

    /**
     * @brief A simulation timestamp class for energy consumption
     */
    class SimulationTimestampEnergyConsumption: public SimulationTimestampType {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        SimulationTimestampEnergyConsumption(std::string hostname, double joules);
        /***********************/
        /** \endcond           */
        /***********************/

        std::string getHostname();
        double getConsumption();

    private:
        std::string hostname;
        double joules;
    };
};

#endif //WRENCH_SIMULATIONTIMESTAMPTYPES_H
