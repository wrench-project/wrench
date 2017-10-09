/**
 * Copyright (c) 2017. The WRENCH Team.
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
#include "wrench/services/compute/multihost_multicore/MultihostMulticoreComputeService.h"
#include "wrench/services/compute/multihost_multicore/MultihostMulticoreComputeServiceProperty.h"
#include "wrench/services/storage/SimpleStorageService.h"
#include "wrench/services/storage/SimpleStorageServiceProperty.h"
#include "wrench/services/file_registry/FileRegistryService.h"
#include "wrench/services/file_registry/FileRegistryServiceProperty.h"
#include "wrench/services/compute/cloud/CloudService.h"
#include "wrench/services/compute/cloud/CloudServiceProperty.h"

// WMS Implementations
#include "wrench/wms/WMS.h"

// Scheduler
#include "wrench/wms/scheduler/Scheduler.h"
#include "wrench/wms/scheduler/PilotJobScheduler.h"

// Scheduling Optimizations
#include "wrench/wms/StaticOptimization.h"
#include "wrench/wms/DynamicOptimization.h"

// Simulation Output Analysis
#include "wrench/simulation/SimulationTimestamp.h"
#include "wrench/simulation/SimulationTimestampTypes.h"

// WorkflowUtil Namespace
#include "wrench/util/WorkflowUtil.h"



#endif //WRENCH_WRENCH_H
