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

		/**
		 * @brief Constructor
		 * @param workflow: a pointer to a Workflow object
		 * @param num_cores: the number of cores required by the pilot job
		 * @param duration: duration of the pilot job, in seconds
		 */
		PilotJob::PilotJob(Workflow *workflow, unsigned long num_cores, double duration) {
			this->type = WorkflowJob::PILOT;

			this->state = PilotJob::State::NOT_SUBMITTED;
			this->compute_service = nullptr;
			this->workflow = workflow;
			this->num_cores = num_cores;
			this->duration = duration;
			this->name = "pilot_job_" + std::to_string(WorkflowJob::getNewUniqueNumber());

		}

		/**
		 * @brief Get the state of the pilot job
		 * @return the state
		 */
		PilotJob::State PilotJob::getState() {
			return this->state;
		}

		/**
		 * @brief Get the compute service on which the pilot job is running
		 * @return a pointer to a ComputeService object
		 */
		ComputeService *PilotJob::getComputeService() {
			return this->compute_service;
		}


		/**
		 * @brief Set the compute service on which the pilot job is running
		 * @param cs: a pointer to a ComputeService object
		 */
		void PilotJob::setComputeService(ComputeService* cs) {
			this->compute_service = cs;
		}

};