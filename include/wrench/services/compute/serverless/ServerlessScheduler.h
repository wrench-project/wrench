/**
 * Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

 #ifndef WRENCH_SERVERLESSSCHEDULER_H
 #define WRENCH_SERVERLESSSCHEDULER_H
 
 #include <wrench/managers/function_manager/Function.h>
 #include <wrench/managers/function_manager/FunctionManager.h>
 #include <wrench/managers/function_manager/RegisteredFunction.h>
 #include <wrench/services/compute/serverless/Invocation.h>
 #include <memory>
 #include <vector>
 #include <string>
 #include <utility>
 
 namespace wrench {
 
     // Forward declaration for StateOfTheSystem; TODO: Implement this.
     class StateOfTheSystem;
 
     /**
      * @brief Represents a decision for image management.
      *        This decision indicates which images should be removed and which should be downloaded/copied.
      */
     struct ImageManagementDecision {
        std::map<std::string, std::vector<std::shared_ptr<DataFile>>> imagesToCopy;
        std::map<std::string, std::vector<std::shared_ptr<DataFile>>> imagesToRemove;
    };
 
     /**
      * @brief Abstract base class for scheduling in a serverless compute service.
      *        This scheduler operates on a list of schedulable invocations (i.e., invocations whose images
      *        are already on the head node) and makes two kinds of decisions:
      *
      *        - Image management decisions: Which images to remove and which to copy to compute nodes.
      *        - Function scheduling decisions: Which invocation should run on which compute node.
      */
     class ServerlessScheduler {
     public:
         ServerlessScheduler() = default;
         virtual ~ServerlessScheduler() = default;
 
         /**
          * @brief Analyze the list of schedulable invocations and decide which images need to be downloaded/copied to
          *        compute nodes or removed from them.
          *
          * @param schedulableInvocations A list of invocations whose images reside on the head node.
          * @param state The current system state snapshot.
          * @return ImageManagementDecision structure with images to delete and images to download/copy.
          */
         virtual ImageManagementDecision manageImages(
             const std::vector<std::shared_ptr<Invocation>>& schedulableInvocations,
             std::shared_ptr<StateOfTheSystem> state
         ) = 0;
 
         /**
          * @brief Schedule functions to be executed on specific compute nodes.
          *
          * @param schedulableInvocations A list of invocations ready for scheduling.
          * @param state The current system state snapshot.
          * @return A vector of scheduling decisions, where each decision is a pair:
          *         - The first element is an invocation.
          *         - The second element is the target compute node (as a string) for that invocation.
          */
         virtual std::vector<std::pair<std::shared_ptr<Invocation>, std::string>> scheduleFunctions(
             const std::vector<std::shared_ptr<Invocation>>& schedulableInvocations,
             std::shared_ptr<StateOfTheSystem> state
         ) = 0;
     };
 
 } // namespace wrench
 
 #endif // WRENCH_SERVERLESSSCHEDULER_H
 