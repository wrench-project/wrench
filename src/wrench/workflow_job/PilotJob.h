/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_PILOTJOB_H
#define WRENCH_PILOTJOB_H


#include "WorkflowJob.h"

namespace wrench {

		class ComputeService;

		/***********************/
		/** \cond DEVELOPER    */
		/***********************/

		/**
		 * @brief A pilot WorkflowJob
		 */
		class PilotJob : public WorkflowJob {

		public:
				enum State {
						NOT_SUBMITTED,
						PENDING,
						RUNNING,
						EXPIRED,
						FAILED
				};

				ComputeService *getComputeService();

				/***********************/
				/** \cond INTERNAL     */
				/***********************/

				void setComputeService(ComputeService*);

				/***********************/
				/** \endcond           */
				/***********************/



		private:

				friend class JobManager;

				PilotJob(Workflow *workflow, unsigned long, double);
				PilotJob::State getState();



				State state;
				ComputeService *compute_service; // Associated compute service
		};

		/***********************/
		/** \endcond           */
		/***********************/


};


#endif //WRENCH_PILOTJOB_H
