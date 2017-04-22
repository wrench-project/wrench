/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SCHEDULER_H
#define WRENCH_SCHEDULER_H

#include "workflow/WorkflowTask.h"
#include "compute_services/ComputeService.h"
#include <set>
#include <job_manager/JobManager.h>

namespace wrench {

		/***********************/
		/** \cond DEVELOPER    */
		/***********************/


		/**
		 * @brief A (mostly) abstract implementation of a scheduler
		 */
		class Scheduler {

		protected:
				Scheduler() {};
				std::string sched_type;

		public:
				virtual ~Scheduler() {};

				/**
				 * @brief Schedule and run a set of ready tasks in available compute resources
				 *
				 * @param job_manager: a pointer to a JobManager object
				 * @param ready_tasks: a vector of ready WorkflowTask objects (i.e., ready tasks in the workflow)
				 * @param compute_services: a set of pointers to ComputeService objects (i.e., compute services available to run jobs)
				 */
				virtual void scheduleTasks(JobManager *job_manager,
																	 std::vector<WorkflowTask *> ready_tasks,
																	 const std::set<ComputeService *> &compute_services) = 0;

			/**
				* @brief Schedule and run pilot jobs
				*
				* @param job_manager: a pointer to a JobManager object
				* @param workflow: a pointer to a Workflow object
				* @param flops: the number of flops that the PilotJob should be able to do before terminating
				* @param compute_services: a set of pointers to ComputeSertvice objects (i.e., compute services available to run jobs)
				*/
				virtual void schedulePilotJobs(JobManager *job_manager,
																			 Workflow *workflow,
																			 double pilot_job_duration,
																			 const std::set<ComputeService *> &compute_services) = 0;
		};

		/***********************/
		/** \endcond           */
		/***********************/


}

#endif //WRENCH_SCHEDULER_H
