/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_PILOTJOBMANAGER_H
#define WRENCH_PILOTJOBMANAGER_H

#include <vector>
#include <set>

#include "simgrid_S4U_util/S4U_Daemon.h"

namespace wrench {

		class Workflow;
		class WorkflowTask;
		class WorkflowFile;
		class WorkflowJob;
		class PilotJob;
		class StandardJob;
		class ComputeService;
		class StorageService;

		/***********************/
		/** \cond DEVELOPER    */
		/***********************/


		/**
     * @brief A helper daemon (co-located with the WMS) to handle job executions
     */
		class JobManager : public S4U_Daemon {

		public:

				JobManager(Workflow *workflow);

				~JobManager();

				void stop();

				void kill();

				StandardJob *createStandardJob(std::vector<WorkflowTask *> tasks,
				                               std::map<WorkflowFile *, StorageService *> file_locations,
				                               std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> pre_file_copies,
				                               std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> post_file_copies,
				                               std::set<std::tuple<WorkflowFile *, StorageService *>> cleanup_file_deletions);

				StandardJob *createStandardJob(std::vector<WorkflowTask *> tasks,
				                               std::map<WorkflowFile *,
								                               StorageService *> file_locations);

				StandardJob *createStandardJob(WorkflowTask *task,
				                               std::map<WorkflowFile *,
								                               StorageService *> file_locations);

				PilotJob *createPilotJob(Workflow *workflow, unsigned int num_cores, double duration);

				void submitJob(WorkflowJob *job, ComputeService *compute_service);

				void submitJob(WorkflowJob *job, ComputeService *compute_service, std::map<std::string,unsigned long> batchjobargs);

				void terminateJob(WorkflowJob *);

				void forgetJob(WorkflowJob *job);

				std::set<PilotJob *> getPendingPilotJobs();

				std::set<PilotJob *> getRunningPilotJobs();

		private:

				/***********************/
				/** \cond INTERNAL     */
				/***********************/

				int main();

				// Relevant workflow
				Workflow *workflow;

				// Job map
				std::map<WorkflowJob*, std::unique_ptr<WorkflowJob>> jobs;

				// Job lists
				std::set<StandardJob *> pending_standard_jobs;
				std::set<StandardJob *> running_standard_jobs;
				std::set<StandardJob *> completed_standard_jobs;
				std::set<StandardJob *> failed_standard_jobs;

				std::set<PilotJob *> pending_pilot_jobs;
				std::set<PilotJob *> running_pilot_jobs;
				std::set<PilotJob *> completed_pilot_jobs;


				/***********************/
				/** \endcond           */
				/***********************/
		};

		/***********************/
		/** \endcond            */
		/***********************/

};

#endif //WRENCH_PILOTJOBMANAGER_H
