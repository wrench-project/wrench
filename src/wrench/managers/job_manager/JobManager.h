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

#include "simgrid_S4U_util/S4U_DaemonWithMailbox.h"

namespace wrench {

    class Workflow;
		class WorkflowTask;
    class WorkflowJob;
		class PilotJob;
		class StandardJob;
		class ComputeService;

		/***********************/
		/** \cond DEVELOPER    */
		/***********************/


		/**
		 * @brief A  facility for managing pilot
 		 *     jobs submitted by a WMS Engine to compute services.
		 */
		class JobManager : public S4U_DaemonWithMailbox {

		public:

				JobManager(Workflow *);

				~JobManager();

				void stop();

				void kill();

				StandardJob *createStandardJob(std::vector<WorkflowTask *>);

				StandardJob *createStandardJob(WorkflowTask *);

				PilotJob *createPilotJob(Workflow *workflow, int num_cores, double duration);

				void submitJob(WorkflowJob *job, ComputeService *compute_service);

				void cancelPilotJob(PilotJob *);

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
				std::map<std::string, std::unique_ptr<WorkflowJob>> jobs;

				// Job lists
				std::set<StandardJob *> pending_standard_jobs;
				std::set<StandardJob *> running_standard_jobs;
				std::set<StandardJob *> completed_standard_jobs;
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
