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

				/** Stop **/
				void stop();

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
				std::set<PilotJob*> getRunningPilotJobs();

			private:
				friend class JobManagerDaemon;

				Workflow *workflow;
				std::unique_ptr<JobManagerDaemon> daemon;
				std::map<std::string, std::unique_ptr<WorkflowJob>> jobs;
				std::set<WorkflowJob*> pending_jobs;
				std::set<WorkflowJob*> running_jobs;
				std::set<WorkflowJob*> completed_jobs;

		};

};

#endif //WRENCH_PILOTJOBMANAGER_H
