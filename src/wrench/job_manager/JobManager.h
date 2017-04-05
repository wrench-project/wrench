/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief PilotJobManager implements a simple facility for managing pilot
 *        jobs submitted by a WMS to pilot-job-enabled compute services.
 */


#ifndef WRENCH_PILOTJOBMANAGER_H
#define WRENCH_PILOTJOBMANAGER_H

#include <vector>
#include <set>
#include <workflow/Workflow.h>

namespace wrench {

		class PilotJob;
		class JobManagerDaemon;
		class ComputeService;

		class JobManager {
			public:
				JobManager(Workflow *);
				~JobManager();

				/** Stop / Kill **/
				void stop();
				void kill();

				/** Job creation **/
				StandardJob *createStandardJob(std::vector<WorkflowTask *>);
				StandardJob *createStandardJob(WorkflowTask *);
				PilotJob *createPilotJob(Workflow *workflow, int num_cores, double durations);

				/** job submission **/
				void submitJob(WorkflowJob *, ComputeService *);

				/** pilot job cancellation **/
				void cancelPilotJob(PilotJob *);

				/** forget job **/
				void forgetJob(WorkflowJob *);

				/** Information **/
				std::set<PilotJob*> getPendingPilotJobs();
				std::set<PilotJob*> getRunningPilotJobs();

			private:
				friend class JobManagerDaemon;

				Workflow *workflow;
				std::unique_ptr<JobManagerDaemon> daemon;

				std::map<std::string, std::unique_ptr<WorkflowJob>> jobs;

				std::set<StandardJob*> pending_standard_jobs;
				std::set<StandardJob*> running_standard_jobs;
				std::set<StandardJob*> completed_standard_jobs;
				std::set<PilotJob*> pending_pilot_jobs;
				std::set<PilotJob*> running_pilot_jobs;
				std::set<PilotJob*> completed_pilot_jobs;


		};

};

#endif //WRENCH_PILOTJOBMANAGER_H
