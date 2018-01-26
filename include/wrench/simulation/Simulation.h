/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef WRENCH_SIMULATION_H
#define WRENCH_SIMULATION_H

#include <string>
#include <vector>

#include "wrench/services/file_registry/FileRegistryService.h"
#include "wrench/services/network_proximity/NetworkProximityService.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "wrench/simulation/SimulationOutput.h"
#include "wrench/wms/WMS.h"
#include "wrench/workflow/job/StandardJob.h"


namespace wrench {

    class StorageService;

    /**
     * @brief The simulation state
     */
    class Simulation {

    public:
        Simulation();

        ~Simulation();

        void init(int *, char **);

        void instantiatePlatform(std::string);

        std::vector<std::string> getHostnameList();

        bool hostExists(std::string hostname);

        void launch();

        ComputeService *add(std::unique_ptr<ComputeService> executor);

        StorageService *add(std::unique_ptr<StorageService> executor);

        void setFileRegistryService(std::unique_ptr<FileRegistryService> file_registry_service);

        void setNetworkProximityService(std::unique_ptr<NetworkProximityService> network_proximity_service);

        void stageFile(WorkflowFile *file, StorageService *storage_service);

        void stageFiles(std::set<WorkflowFile *> files, StorageService *storage_service);

        WMS *setWMS(std::unique_ptr<WMS>);

        /** @brief The simulation post-mortem output */
        SimulationOutput output;

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        template<class T>
        void newTimestamp(SimulationTimestamp<T> *event);

        void shutdownAllComputeServices();

        void shutdownAllStorageServices();

        std::set<ComputeService *> getRunningComputeServices();

        std::set<StorageService *> getRunningStorageServices();

        FileRegistryService *getFileRegistryService();

        NetworkProximityService *getNetworkProximityService();

        double getCurrentSimulatedDate();

        /***********************/
        /** \endcond            */
        /***********************/

        /***********************/
        /** \cond INTERNAL     */
        /***********************/


        /***********************/
        /** \endcond           */
        /***********************/

    private:

        std::unique_ptr<S4U_Simulation> s4u_simulation;

        std::unique_ptr<WMS> wms = nullptr;

        std::unique_ptr<FileRegistryService> file_registry_service = nullptr;

        std::unique_ptr<NetworkProximityService> network_proximity_service = nullptr;

        std::set<std::unique_ptr<ComputeService>> compute_services;

        std::set<std::unique_ptr<StorageService>> storage_services;

        void check_simulation_setup();
        void start_all_processes();

    };

};

#endif //WRENCH_SIMULATION_H
