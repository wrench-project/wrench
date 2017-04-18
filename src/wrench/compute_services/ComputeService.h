/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef SIMULATION_COMPUTESERVICE_H
#define SIMULATION_COMPUTESERVICE_H

#include <map>
#include <workflow_job/WorkflowJob.h>

namespace wrench {

		class Simulation;

		class WorkflowJob;

		class StandardJob;

		class PilotJob;

		/**
		 * @brief Abstract implementation of a compute service.
		 */
		class ComputeService {

		public:

				enum State {
						UP,
						DOWN,
				};

				/** \cond DEVELOPER **/

				/**
				 * @brief Stop the compute service - must be called by the stop()
				 *        method of derived classes
				 */
				virtual void stop();


				/**
		    * @brief Submit a job to the compute service
		    * @param job: a pointer to the job
		    */
				void runJob(WorkflowJob *job);

				/**
				 * @brief Get the name of the compute service
				 * @return the compute service name
				 */
				std::string getName();

				/**
		    * @brief Find out whether the compute service is UP
		    * @return true if the compute service is UP, false otherwise
		    */
				bool isUp();

				/**
		     * @brief Check whether the service is able to run a job
		     *
		     * @param job_type: the job type
		     * @param min_num_cores: the minimum number of cores required
		     * @param duration: the duration in seconds
		     * @return true if the compute service can run the job, false otherwise
		     */
				bool canRunJob(WorkflowJob::Type job_type, unsigned long min_num_cores, double duration);


				/**
				 * @brief Get the flop/sec rate of one core of the compute service's host
				 * @return  the flop rate
				 */
				virtual double getCoreFlopRate() = 0;

				/**
				 * @brief Get the number of physical cores on the compute service's host
				 * @return the core count
				 */
				virtual unsigned long getNumCores() = 0;

				/**
				 * @brief Get the number of currently idle cores on the compute service's host
				 * @return the idle core count
				 */
				virtual unsigned long getNumIdleCores() = 0;

				/**
				 * @brief Get the time-to-live, in seconds, of the compute service
				 * @return the ttl
				 */
				virtual double getTTL() = 0;

				/** \endcond  */



				/** \cond INTERNAL **/

				/**
		     * @brief Submit a standard job to the compute service (virtual)
		     * @param job: a pointer to the job
		     * @return
		     */
				virtual void runStandardJob(StandardJob *job);

				/**
				 * @brief Submit a pilot job to the compute service (virtual)
				 * @param job: a pointer ot the job
				 * @return
				 */
				virtual void runPilotJob(PilotJob *job);

				/**
				 * @brief Constructor, which links back the ComputeService
				 *        to a Simulation (i.e., "registering" the ComputeService).
				 *        This means that the Simulation can provide access to
				 *        the ComputeService when queried.
				 *
				 * @param  service_name: the name of the compute service
				 * @param  simulation: a pointer to a WRENCH simulation
				 */
				ComputeService(std::string service_name, Simulation *simulation);

				/**
				 * @brief Constructor
				 *
				 * @param service_name: the name of the compute service
				 */
				ComputeService(std::string service_name);

				/**
				 * @brief Set the "supports standard jobs" property
				 * @param v: true or false
				 */
				void setSupportStandardJobs(bool v);

				/**
				 * @brief Set the "supports pilot jobs" property
				 * @param v: true or false
				 */
				void setSupportPilotJobs(bool v);

				/**
				 * @brief Get the "supports standard jobs" property
				 * @return true or false
				 */
				bool supportsStandardJobs();

				/**
				 * @brief Get the "supports pilot jobs" property
				 * @return true or false
				 */
				bool supportsPilotJobs();

				/**
				 * @brief Set the state of the compute service to DOWN
				 */
				void setStateToDown();


				/** \endcond  **/


		protected:
				friend class Simulation;

				ComputeService::State state;
				std::string service_name;
				Simulation *simulation;  // pointer to the simulation object

		private:
				bool supports_standard_jobs;
				bool supports_pilot_jobs;
		};
};


#endif //SIMULATION_COMPUTESERVICE_H
