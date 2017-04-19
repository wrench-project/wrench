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
#include <workflow/Workflow.h>
#include "simgrid_S4U_util/S4U_DaemonWithMailbox.h"


namespace wrench {

		class PilotJob;

		class ComputeService;

		/**
		 * @brief A  facility for managing pilot
 		 *     jobs submitted by a WMS Engine to compute services.
		 */
		class JobManager : public S4U_DaemonWithMailbox {

		public:
				/*! \cond DEVELOPER */

				/**
		 		 * @brief Constructor, which starts a job manager daemon
		     *
		     * @param workflow: a pointer to the Workflow whose jobs are to be managed
		     */
				JobManager(Workflow *);

				/**
				 * @brief Default destructor
				 */
				~JobManager();


				/**
		     * @brief Stop the job manager
		     */
				void stop();

				/**
		     * @brief Kill the job manager (brutally)
		     */
				void kill();

				/**
		     * @brief Create a standard job
		     *
		     * @param tasks: a vector of WorkflowTask pointers to include in the StandardJob
		     * @return a raw pointer to the StandardJob
		     */
				StandardJob *createStandardJob(std::vector<WorkflowTask *>);

				/**
		     * @brief Create a standard job
		     *
		     * @param task: a pointer the single WorkflowTask to include in the StandardJob
		     * @return a raw pointer to the StandardJob
		     */
				StandardJob *createStandardJob(WorkflowTask *);


				/**
		     * @brief Create a pilot job
		     *
		     * @param workflow: a pointer to a Workflow
		     * @param num_cores: the number of cores required by the PilotJob
		     * @param duration: is the PilotJob duration in seconds
		     * @return a raw pointer to the PilotJob
		     */
				PilotJob *createPilotJob(Workflow *workflow, int num_cores, double duration);

				/**
		     * @brief Submit a job to a compute service
		     *
		     * @param job: a pointer to a WorkflowJob
		     * @param compute_service: is a pointer to a ComputeService
		     */
				void submitJob(WorkflowJob *job, ComputeService *compute_service);

        /**
		     * @brief Cancel a PilotJob that hasn't expired yet
		     * @param job: a pointer to the PilotJob
		     */
				void cancelPilotJob(PilotJob *);

        /**
		     * @brief Forget a job (to free memory, typically once the job is completed)
		     *
		     * @param job: a pointer to a WorkflowJob
		     */
				void forgetJob(WorkflowJob *job);


				/**
		     * @brief Get the list of currently pending PilotJob instances
		     * @return  a set of PilotJob pointers
		     */
				std::set<PilotJob *> getPendingPilotJobs();

				/**
		     * @brief Get the list of currently running PilotJob instances
		     * @return a set of PilotJob pointers
		     */
				std::set<PilotJob *> getRunningPilotJobs();

		private:

				/*! \cond INTERNAL */

				/**
		     * @brief Main method of the daemon that implements the JobManager
		     * @return 0 in success
		     */
				int main();

				/*! \endcond */

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

		};

};

#endif //WRENCH_PILOTJOBMANAGER_H
