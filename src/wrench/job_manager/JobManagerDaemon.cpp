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
#include "JobManagerDaemon.h"
#include "JobManager.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(pilotjob_manager_daemon, "Log category for PilotJob Manager Daemon");

namespace wrench {

	JobManagerDaemon::JobManagerDaemon(JobManager *job_manager): S4U_DaemonWithMailbox("job_manager", "job_manager") {
		this->job_manager = job_manager;
	}


	int JobManagerDaemon::main() {
		XBT_INFO("New Multicore Task Executor starting (%s)", this->mailbox_name.c_str());

		bool keep_going = true;
		while (keep_going) {
			std::unique_ptr<SimulationMessage> message = S4U_Mailbox::get(this->mailbox_name);
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
					this->job_manager->pending_jobs.erase(job);
					this->job_manager->completed_jobs.insert(job);

					// Forward the notification along the notification chain
					S4U_Mailbox::put(job->popCallbackMailbox(),
													 new StandardJobDoneMessage(job, m->compute_service));
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