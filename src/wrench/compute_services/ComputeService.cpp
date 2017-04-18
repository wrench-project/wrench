/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <exception/WRENCHException.h>
#include <logging/Logging.h>
#include "ComputeService.h"
#include "simulation/Simulation.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(compute_service, "Log category for Compute Service");

namespace wrench {

		void ComputeService::stop() {
			this->state = ComputeService::DOWN;
			// Notify the simulation that the service is terminated, if that
			// service was registered with the simulation
			if (this->simulation) {
				this->simulation->mark_compute_service_as_terminated(this);
			}
		}

		std::string ComputeService::getName() {
			return this->service_name;
		}

		bool ComputeService::isUp() {
			return (this->state == ComputeService::UP);
		}

		void ComputeService::runJob(WorkflowJob *job) {

			if (this->state == ComputeService::DOWN) {
				throw new WRENCHException("Compute Service is Down");
			}

			switch (job->getType()) {
				case WorkflowJob::STANDARD: {
					this->runStandardJob((StandardJob *) job);
					break;
				}
				case WorkflowJob::PILOT: {
					this->runPilotJob((PilotJob *) job);
					break;
				}
			}
		}

		bool ComputeService::canRunJob(WorkflowJob::Type job_type,
																	 unsigned long min_num_cores,
																	 double flops) {
			// If the service isn't up, forget it
			if (this->state != ComputeService::UP) {
				return false;
			}

			// Check if the job type works
			switch (job_type) {
				case WorkflowJob::STANDARD: {
					if (!this->supportsStandardJobs()) {
						return false;
					}
					break;
				}
				case WorkflowJob::PILOT: {
					if (!this->supportsPilotJobs()) {
						return false;
					}
					break;
				}
			}

			// Check that the number of cores is ok (does a communication with the daemons)
			unsigned long num_idle_cores = this->getNumIdleCores();
			WRENCH_INFO("The compute service says it has %ld idle cores", num_idle_cores);
			if (num_idle_cores < min_num_cores) {
				return false;
			}

			// Check that the TTL is ok (does a communication with the daemons)
			double ttl = this->getTTL();
			double duration = flops / this->getCoreFlopRate();
			if ((ttl > 0) && (ttl < duration)) {
				return false;
			}

			// Everything checks out
			return true;
		}

		ComputeService::ComputeService(std::string service_name, Simulation *simulation) {
			this->service_name = service_name;
			this->simulation = simulation;
			this->state = ComputeService::UP;
		}

		ComputeService::ComputeService(std::string service_name) {
			this->service_name = service_name;
			this->simulation = nullptr;
			this->state = ComputeService::UP;
		}

		void ComputeService::setSupportStandardJobs(bool v) {
			this->supports_standard_jobs = v;
		}

		void ComputeService::setSupportPilotJobs(bool v) {
			this->supports_pilot_jobs = v;
		}

		bool ComputeService::supportsStandardJobs() {
			return this->supports_standard_jobs;
		}

		bool ComputeService::supportsPilotJobs() {
			return this->supports_pilot_jobs;
		}

		void ComputeService::runStandardJob(StandardJob *job) {
			throw WRENCHException("The compute service does not implement runStandardJob(StandardJob *)");
		}


		void ComputeService::runPilotJob(PilotJob *job) {
			throw WRENCHException("The compute service does not implement runPilotJob(PilotJob *)");
		}

		void ComputeService::setStateToDown() {
			this->state = ComputeService::DOWN;
		}

};
