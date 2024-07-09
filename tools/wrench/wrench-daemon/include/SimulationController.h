/**
 * Copyright (c) 2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SIMULATION_CONTROLLER_H
#define WRENCH_SIMULATION_CONTROLLER_H

#include <wrench-dev.h>
#include <map>
#include <vector>
#include <queue>
#include <mutex>

#include <nlohmann/json.hpp>

#include "BlockingQueue.h"
#include "KeyValueStore.h"

using json = nlohmann::json;

namespace wrench {

    /**
     * @brief A class that implements the Simulation Controller as a WMS. This is the key
     * WRENCH service that controls the simulation, handling all interaction with a
     * Simulation Daemon (which itself was spawned by wrench-daemon, and which handles
     * all interaction with the simulation client).
     */
    class SimulationController : public ExecutionController {

    public:
        SimulationController(const std::string &hostname, int sleep_us);

        void stopSimulation();

        json getSimulationTime(const json& data);

        json getAllHostnames(const json& data);

        json advanceTime(json data);

        json getSimulationEvents(const json&);

        json createStandardJob(json data);
        json submitStandardJob(json data);
        json getStandardJobTasks(json data);
        json addInputFile(json data);
        json addOutputFile(json data);

        json createCompoundJob(json data);
        json submitCompoundJob(json data);
        json addComputeAction(json data);
        json addFileCopyAction(json data);
        json addFileDeleteAction(json data);
        json addFileWriteAction(json data);
        json addFileReadAction(json data);
        json addSleepAction(json data);
        json addParentJob(json data);

        json createTask(json data);
        json getTaskFlops(json data);
        json getTaskMinNumCores(json data);
        json getTaskMaxNumCores(json data);
        json getTaskMemory(json data);
        json getTaskStartDate(json data);
        json getTaskEndDate(json data);

        json waitForNextSimulationEvent(const json& data);

        json addBareMetalComputeService(json data);

        json addCloudComputeService(json data);

        json addBatchComputeService(json data);

        json addSimpleStorageService(json data);

        json createFileCopyAtStorageService(json data);
        json lookupFileAtStorageService(json data);

        json addFileRegistryService(json data);
        json fileRegistryServiceAddEntry(json data);
        json fileRegistryServiceLookUpEntry(json data);
        json fileRegistryServiceRemoveEntry(json data);

        json addFile(json data);
        json getFileSize(json data);

        json getTaskInputFiles(json data);

        json getTaskOutputFiles(json data);

        json getInputFiles(json data);
        json getReadyTasks(json data);
        json stageInputFiles(json data);
        json workflowIsDone(json data);


        json supportsCompoundJobs(json data);
        json supportsPilotJobs(json data);
        json supportsStandardJobs(json data);
        json getCoreFlopRates(json data);
        json getCoreCounts(json data);

        json createVM(json data);

        json startVM(json data);

        json shutdownVM(json data);

        json destroyVM(json data);

        json isVMRunning(json data);

        json isVMDown(json data);

        json suspendVM(json data);

        json resumeVM(json data);

        json isVMSuspended(json data);

        json getExecutionHosts(json data);

        json getVMPhysicalHostname(json data);

        json getVMComputeService(json data);

        json createWorkflow(const json& data);

        json createWorkflowFromJSON(json data);

    private:
        template<class T>
        json startService(T *s);

        // Thread-safe key value stores
        KeyValueStore<std::shared_ptr<wrench::Workflow>> workflow_registry;
        KeyValueStore<std::shared_ptr<wrench::StandardJob>> standard_job_registry;
        KeyValueStore<std::shared_ptr<wrench::CompoundJob>> compound_job_registry;
        KeyValueStore<std::shared_ptr<ComputeService>> compute_service_registry;
        KeyValueStore<std::shared_ptr<StorageService>> storage_service_registry;
        KeyValueStore<std::shared_ptr<FileRegistryService>> file_registry_service_registry;

        // Thread-safe queues for the server thread and the simulation thread to communicate
        BlockingQueue<std::pair<double, std::shared_ptr<wrench::ExecutionEvent>>> event_queue;

//        BlockingQueue<std::tuple<std::shared_ptr<StandardJob>, std::shared_ptr<ComputeService>, std::map<std::string, std::string>>> submissions_to_do;

        BlockingQueue<std::function<void()>> things_to_do;

        // The two managers
        std::shared_ptr<JobManager> job_manager;
        std::shared_ptr<DataMovementManager> data_movement_manager;

        bool keep_going = true;
        double time_horizon_to_reach = 0;
        unsigned int sleep_us;

        int main() override;

        static json eventToJSON(double date, const std::shared_ptr<wrench::ExecutionEvent> &event);
    };
}// namespace wrench

#endif// WRENCH_SIMULATION_CONTROLLER_H
