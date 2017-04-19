/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "PilotJob.h"

namespace wrench {

		/***********************************************************/
		/**	DEVELOPER METHODS BELOW **/
		/***********************************************************/

		/*! \cond DEVELOPER */

		PilotJob::PilotJob(Workflow *workflow, unsigned long num_cores, double duration) {
			this->type = WorkflowJob::PILOT;

			this->state = PilotJob::State::NOT_SUBMITTED;
			this->compute_service = nullptr;
			this->workflow = workflow;
			this->num_cores = num_cores;
			this->duration = duration;
			this->name = "pilot_job_" + std::to_string(WorkflowJob::getNewUniqueNumber());

		}

		PilotJob::State PilotJob::getState() {
			return this->state;
		}

		ComputeService *PilotJob::getComputeService() {
			return this->compute_service;
		}

		/*! \endcond */

		/***********************************************************/
		/**	INTERNAL METHODS BELOW **/
		/***********************************************************/

		/*! \cond INTERNAL */

		/**
		 * @brief
		 * @param cs
		 */
		void PilotJob::setComputeService(ComputeService* cs) {
			this->compute_service = cs;
		}
		/*! \endcond */


};