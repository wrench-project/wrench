/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_WRENCH_H
#define WRENCH_WRENCH_H

#include "wrench/simulation/Simulation.h"
#include "wrench/workflow/Workflow.h"

// Services and Service Properties
#include "wrench/services/compute/bare_metal/BareMetalComputeService.h"
#include "wrench/services/compute/bare_metal/BareMetalComputeServiceProperty.h"
#include "wrench/services/storage/simple/SimpleStorageService.h"
#include "wrench/services/storage/simple/SimpleStorageServiceProperty.h"
#include "wrench/services/file_registry/FileRegistryService.h"
#include "wrench/services/file_registry/FileRegistryServiceProperty.h"
#include "wrench/services/compute/virtualized_cluster/VirtualizedClusterComputeService.h"
#include "wrench/services/compute/virtualized_cluster/VirtualizedClusterComputeServiceProperty.h"
#include "wrench/services/compute/cloud/CloudComputeService.h"
#include "wrench/services/compute/cloud/CloudComputeServiceProperty.h"
#include "wrench/services/compute/batch/BatchComputeService.h"
#include "wrench/services/compute/batch/BatchComputeServiceProperty.h"
#include "wrench/services/compute/htcondor/HTCondorComputeService.h"
#include "wrench/services/compute/htcondor/HTCondorComputeServiceProperty.h"
#include "wrench/services/network_proximity/NetworkProximityService.h"
#include "wrench/services/network_proximity/NetworkProximityServiceProperty.h"

// WMS Implementations
#include "wrench/wms/WMS.h"

// Scheduler
#include "wrench/wms/scheduler/StandardJobScheduler.h"
#include "wrench/wms/scheduler/PilotJobScheduler.h"

// Scheduling Optimizations
#include "wrench/wms/StaticOptimization.h"
#include "wrench/wms/DynamicOptimization.h"

// Simulation Output Analysis
#include "wrench/simulation/SimulationTimestamp.h"
#include "wrench/simulation/SimulationTimestampTypes.h"

// Tools
#include "wrench/tools/pegasus/PegasusWorkflowParser.h"


#endif //WRENCH_WRENCH_H
