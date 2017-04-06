/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief TBD
 */

#include <simulation/SimulationMessage.h>
#include <simgrid_S4U_util/S4U_Mailbox.h>
#include <exception/WRENCHException.h>
#include <workflow_job/StandardJob.h>
#include <workflow_job/PilotJob.h>
#include "JobManagerDaemon.h"
#include "JobManager.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(pilotjob_manager_daemon, "Log category for PilotJob Manager Daemon");

namespace wrench {

	JobManagerDaemon::JobManagerDaemon(JobManager *job_manager): S4U_DaemonWithMailbox("job_manager", "job_manager") {
		this->job_manager = job_manager;
	}


	int JobManagerDaemon::main() {
		XBT_INFO("New Job Manager starting (%s)", this->mailbox_name.c_str());

		bool keep_going = true;
		while (keep_going) {
			std::unique_ptr<SimulationMessage> message = S4U_Mailbox::get(this->mailbox_name);

			// Clear finished asynchronous dput()
			S4U_Mailbox::clear_dputs();

			XBT_INFO("Job Manager got a %s message", message->toString().c_str());
			switch (message->type) {

				case SimulationMessage::STOP_DAEMON: {
					// There should be any need to clean any state up
					keep_going = false;
					break;
				}

				case SimulationMessage::STANDARD_JOB_DONE: {
					std::unique_ptr<StandardJobDoneMessage> m(static_cast<StandardJobDoneMessage *>(message.release()));

					// update job state
					StandardJob *job = m->job;
					job->state = StandardJob::State::COMPLETED;

					// move the job from the "pending" list to the "completed" list
					this->job_manager->pending_standard_jobs.erase(job);
					this->job_manager->completed_standard_jobs.insert(job);

					// Forward the notification along the notification chain
					S4U_Mailbox::dput(job->popCallbackMailbox(),
													 new StandardJobDoneMessage(job, m->compute_service));
					break;
				}

				case SimulationMessage::STANDARD_JOB_FAILED: {
					std::unique_ptr<StandardJobFailedMessage> m(static_cast<StandardJobFailedMessage *>(message.release()));

					// update job state
					StandardJob *job = m->job;
					job->state = StandardJob::State::FAILED;

					// remove the job from the "pending" list
					this->job_manager->pending_standard_jobs.erase(job);

					// Forward the notification along the notification chain
					S4U_Mailbox::dput(job->popCallbackMailbox(),
													 new StandardJobFailedMessage(job, m->compute_service));
					break;
				}

				case SimulationMessage::PILOT_JOB_STARTED: {
					std::unique_ptr<PilotJobStartedMessage> m(static_cast<PilotJobStartedMessage *>(message.release()));

					// update job state
					PilotJob *job = m->job;
					job->state = PilotJob::State::RUNNING;

					// move the job from the "pending" list to the "running" list
					this->job_manager->pending_pilot_jobs.erase(job);
					this->job_manager->running_pilot_jobs.insert(job);

					// Forward the notification to the source
					XBT_INFO("Forwarding to %s", job->getOriginCallbackMailbox().c_str());
					S4U_Mailbox::dput(job->getOriginCallbackMailbox(),
														new PilotJobStartedMessage(job, m->compute_service));

					break;
				}

				case SimulationMessage::PILOT_JOB_EXPIRED: {
					std::unique_ptr<PilotJobExpiredMessage> m(static_cast<PilotJobExpiredMessage *>(message.release()));

					// update job state
					PilotJob *job = m->job;
					job->state = PilotJob::State::EXPIRED;

					// Remove the job from the "running" list
					this->job_manager->running_pilot_jobs.erase(job);
					XBT_INFO("THERE ARE NOW %ld running pilot jobs", this->job_manager->running_pilot_jobs.size());

					// Forward the notification to the source
					XBT_INFO("Forwarding to %s", job->getOriginCallbackMailbox().c_str());
					S4U_Mailbox::dput(job->getOriginCallbackMailbox(),
													 new PilotJobExpiredMessage(job, m->compute_service));

					break;
				}

				default: {
					throw WRENCHException("Invalid message type " + std::to_string(message->type));
				}
			}




			}

		XBT_INFO("New Multicore Task Executor terminating");
		return 0;
	}


};