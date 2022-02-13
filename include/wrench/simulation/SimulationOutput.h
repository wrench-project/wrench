/**
 * Copyright (c) 2017-2020. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SIMULATIONOUTPUT_H
#define WRENCH_SIMULATIONOUTPUT_H

#include <typeinfo>
#include <typeindex>
#include <iostream>

#include "wrench/simulation/SimulationTimestamp.h"
#include "wrench/simulation/SimulationTrace.h"


namespace wrench {
    class Simulation;

    /**
     * @brief A class that contains post-mortem simulation-generated data
     */
    class SimulationOutput {

    public:
        /**
         * @brief Retrieve a copy of a simulation output trace
         *        once the simulation has completed
         *
         * @tparam a particular SimulationTimestampXXXX class (defined in SimulationTimestampTypes.h)
         * @return a vector of pointers to SimulationTimestampXXXX instances
         */
        template<class T>
        std::vector<SimulationTimestamp<T> *> getTrace() {
            std::type_index type_index = std::type_index(typeid(T));

            // Is the trace empty?
            if (this->traces.find(type_index) == this->traces.end()) {
                return {};
            }

            std::vector<SimulationTimestamp<T> *> non_generic_vector;
            auto trace = (SimulationTrace<T> *) (this->traces[type_index]);
            for (auto ts : trace->getTrace()) {
                non_generic_vector.push_back((SimulationTimestamp<T> *) ts);
            }
            return non_generic_vector;
        }

        void dumpWorkflowExecutionJSON(std::shared_ptr<Workflow> workflow, std::string file_path,
                                       bool generate_host_utilization_layout = false, bool writing_file = true);

        void dumpWorkflowGraphJSON(std::shared_ptr<Workflow> workflow, std::string file_path, bool writing_file = true);

        void dumpHostEnergyConsumptionJSON(std::string file_path, bool writing_file = true);

        void dumpPlatformGraphJSON(std::string file_path, bool writing_file = true);

        void dumpDiskOperationsJSON(std::string file_path, bool writing_file = true);

        void dumpLinkUsageJSON(std::string file_path, bool writing_file = true);

        void dumpUnifiedJSON(std::shared_ptr<Workflow> workflow, std::string file_path,
                             bool include_platform = false,
                             bool include_workflow_exec = true,
                             bool include_workflow_graph = false,
                             bool include_energy = false,
                             bool generate_host_utilization_layout = false,
                             bool include_disk = false,
                             bool include_bandwidth = false);

        void enableWorkflowTaskTimestamps(bool enabled);

        void enableFileReadWriteCopyTimestamps(bool enabled);

        void enableEnergyTimestamps(bool enabled);

        void enableDiskTimestamps(bool enabled);

        void enableBandwidthTimestamps(bool enabled);

        /***********************/
        /** \cond INTERNAL     */
        /***********************/

        void addTimestampTaskStart(double date, std::shared_ptr<WorkflowTask>task);

        void addTimestampTaskFailure(double date, std::shared_ptr<WorkflowTask>task);

        void addTimestampTaskCompletion(double date, std::shared_ptr<WorkflowTask>task);

        void addTimestampTaskTermination(double date, std::shared_ptr<WorkflowTask>task);

        void addTimestampFileReadStart(double date, std::shared_ptr<DataFile>file, std::shared_ptr<FileLocation>  src, std::shared_ptr<StorageService>  service,
                                       std::shared_ptr<WorkflowTask>task = nullptr);

        void addTimestampFileReadFailure(double date, std::shared_ptr<DataFile>file, std::shared_ptr<FileLocation>  src, std::shared_ptr<StorageService>  service,
                                         std::shared_ptr<WorkflowTask>task = nullptr);

        void addTimestampFileReadCompletion(double date, std::shared_ptr<DataFile>file, std::shared_ptr<FileLocation>  src, std::shared_ptr<StorageService>  service,
                                            std::shared_ptr<WorkflowTask>task = nullptr);

        void addTimestampFileWriteStart(double date, std::shared_ptr<DataFile>file, std::shared_ptr<FileLocation>  src, std::shared_ptr<StorageService>  service,
                                        std::shared_ptr<WorkflowTask>task = nullptr);

        void addTimestampFileWriteFailure(double date, std::shared_ptr<DataFile>file, std::shared_ptr<FileLocation>  src, std::shared_ptr<StorageService>  service,
                                          std::shared_ptr<WorkflowTask>task = nullptr);

        void addTimestampFileWriteCompletion(double date, std::shared_ptr<DataFile>file, std::shared_ptr<FileLocation>  src, std::shared_ptr<StorageService>  service,
                                             std::shared_ptr<WorkflowTask>task = nullptr);

        void addTimestampFileCopyStart(double date, std::shared_ptr<DataFile>file, std::shared_ptr<FileLocation> src,
                                       std::shared_ptr<FileLocation> dst);

        void addTimestampFileCopyFailure(double date, std::shared_ptr<DataFile>file, std::shared_ptr<FileLocation> src,
                                         std::shared_ptr<FileLocation> dst);

        void addTimestampFileCopyCompletion(double date, std::shared_ptr<DataFile>file, std::shared_ptr<FileLocation> src,
                                            std::shared_ptr<FileLocation> dst);

        void
        addTimestampDiskReadStart(double date, std::string hostname, std::string mount, double bytes, int unique_sequence_number);

        void
        addTimestampDiskReadFailure(double date, std::string hostname, std::string mount, double bytes, int unique_sequence_number);

        void addTimestampDiskReadCompletion(double date, std::string hostname, std::string mount, double bytes,
                                            int unique_sequence_number);

        void
        addTimestampDiskWriteStart(double date, std::string hostname, std::string mount, double bytes, int unique_sequence_number);

        void
        addTimestampDiskWriteFailure(double date, std::string hostname, std::string mount, double bytes, int unique_sequence_number);

        void addTimestampDiskWriteCompletion(double date, std::string hostname, std::string mount, double bytes,
                                             int unique_sequence_number);

        void addTimestampPstateSet(double date, std::string hostname, int pstate);

        void addTimestampEnergyConsumption(double date, std::string hostname, double joules);

        void addTimestampLinkUsage(double date, std::string linkname, double bytes_per_second);

        /**
        * @brief Append a simulation timestamp to a simulation output trace
        *
        * @tparam a particular SimulationTimestampXXXX class (defined in SimulationTimestampTypes.h)
        * @param timestamp: a pointer to a SimulationTimestampXXXX object
        */
        template<class T>
        void addTimestamp(T *timestamp) {
            std::type_index type_index = std::type_index(typeid(T));
            if (this->traces.find(type_index) == this->traces.end()) {
                this->traces[type_index] = new SimulationTrace<T>();
            }
            ((SimulationTrace<T> *) (this->traces[type_index]))->addTimestamp(new SimulationTimestamp<T>(timestamp));
        }

        /***********************/
        /** \endcond          */
        /***********************/

        /***********************/
        /** \cond              */
        /***********************/

        ~SimulationOutput();

        SimulationOutput();

        /***********************/
        /** \endcond          */
        /***********************/

    private:
        std::map<std::type_index, GenericSimulationTrace *> traces;
        nlohmann::json platform_json_part;
        nlohmann::json workflow_exec_json_part;
        nlohmann::json workflow_graph_json_part;
        nlohmann::json energy_json_part;
        nlohmann::json disk_json_part;
        nlohmann::json bandwidth_json_part;

        std::map<std::type_index, bool> enabledStatus;

        /**
         * @brief  Determines whether a time stamp time is enabled
         * @tparam a particular SimulationTimestampXXXX class (defined in SimulationTimestampTypes.h)
         * @return true or false
         */
        template<class T>
        bool isEnabled() {
            std::type_index type_index = std::type_index(typeid(T));
            return this->enabledStatus[type_index];
        }

        /**
         * @brief  Determines whether a time stamp time is enabled
         * @tparam a particular SimulationTimestampXXXX class (defined in SimulationTimestampTypes.h)
         * @param enabled true is the time stamp type should be enabled, false otherwise
         */
        template<class T>
        void setEnabled(bool enabled) {
            std::type_index type_index = std::type_index(typeid(T));
            this->enabledStatus[type_index] = enabled;
        }
    };
};

#endif //WRENCH_SIMULATIONOUTPUT_H
