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

// Services and Service Properties
#include "wrench/services/compute/bare_metal/BareMetalComputeService.h"
#include "wrench/services/compute/bare_metal/BareMetalComputeServiceProperty.h"
#include "wrench/services/storage/simple/SimpleStorageService.h"
#include "wrench/services/storage/compound/CompoundStorageService.h"
#include "wrench/services/storage/simple/SimpleStorageServiceProperty.h"
#include "wrench/services/storage/xrootd/Deployment.h"
#include "wrench/services/storage/xrootd/XRootDProperty.h"
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
#include "wrench/services/storage/proxy/StorageServiceProxy.h"
#include "wrench/services/compute/serverless/ServerlessComputeService.h"
#include "wrench/services/compute/serverless/ServerlessComputeServiceProperty.h"
#include "wrench/services/compute/serverless/schedulers/greedy_scheduler/GreedyServerlessScheduler.h"
#include "wrench/services/compute/serverless/schedulers/scheduling_policies/node_activation_policies/ServerlessNodeScalingPolicy.h"
#include "wrench/services/compute/serverless/schedulers/scheduling_policies/node_activation_policies/ActivateAllNodePolicy.h"
#include "wrench/services/compute/serverless/schedulers/scheduling_policies/node_activation_policies/ReactiveScaleUpNodeActivationPolicy.h"
#include "wrench/services/compute/serverless/schedulers/scheduling_policies/node_activation_policies/ReactiveScaleUpScaleDownNodeActivationPolicy.h"
#include "wrench/services/compute/serverless/schedulers/scheduling_policies/invocation_ordering_policies/ServerlessInvocationOrderingPolicy.h"
#include "wrench/services/compute/serverless/schedulers/scheduling_policies/invocation_ordering_policies/RandomInvocationOrderingPolicy.h"
#include "wrench/services/compute/serverless/schedulers/scheduling_policies/invocation_ordering_policies/FCFSInvocationOrderingPolicy.h"
#include "wrench/services/compute/serverless/schedulers/scheduling_policies/node_selection_policies/ServerlessNodeSelectionPolicy.h"
#include "wrench/services/compute/serverless/schedulers/scheduling_policies/node_selection_policies/AllPossibleNodeSelectionPolicy.h"
#include "wrench/services/compute/serverless/schedulers/scheduling_policies/eviction_policies/ServerlessEvictionPolicy.h"
#include "wrench/services/compute/serverless/schedulers/scheduling_policies/eviction_policies/LRUServerlessEvictionPolicy.h"
#include "wrench/services/compute/serverless/schedulers/scheduling_policies/eviction_policies/FewestServerlessEvictionPolicy.h"
#include "wrench/services/compute/serverless/schedulers/scheduling_policies/plan_selection_policies/EvictionAversePlanSelectionPolicy.h"
#include "wrench/services/compute/serverless/schedulers/scheduling_policies/plan_selection_policies/RandomPlanSelectionPolicy.h"
#include "wrench/services/compute/serverless/schedulers/scheduling_policies/container_prewarm_policy/ServerlessContainerPrewarmPolicy.h"
#include "wrench/services/compute/serverless/schedulers/scheduling_policies/container_prewarm_policy/NeverPrewarmContainerPolicy.h"
#include "wrench/services/compute/serverless/schedulers/scheduling_policies/invocation_failing_policies/ServerlessInvocationFailingPolicy.h"
#include "wrench/services/compute/serverless/schedulers/scheduling_policies/invocation_failing_policies/NeverFailInvocationsPolicy.h"
#include "wrench/services/compute/serverless/schedulers/scheduling_policies/invocation_failing_policies/QueueSizeLimitFailingPolicy.h"
#include "wrench/services/compute/serverless/schedulers/scheduling_policies/idle_container_selection_policies/ServerlessIdleContainerSelectionPolicy.h"
#include "wrench/services/compute/serverless/schedulers/scheduling_policies/idle_container_selection_policies/ExpiringLatestReusableContainerSelectionPolicy.h"

// Simulation Output Analysis
#include "wrench/simulation/SimulationTimestamp.h"
#include "wrench/simulation/SimulationTimestampTypes.h"

// Workflow
#include "wrench/workflow/Workflow.h"
#include "wrench/workflow/WorkflowTask.h"

// Serverless
#include "wrench/managers/function_manager/FunctionManager.h"
#include "wrench/function/Image.h"
#include "wrench/function/ImageLayer.h"
#include "wrench/function/Function.h"

// Tools
#include "wrench/tools/wfcommons/WfCommonsWorkflowParser.h"

#endif//WRENCH_WRENCH_H
