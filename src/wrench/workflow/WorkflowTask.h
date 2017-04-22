/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_WORKFLOWTASK_H
#define WRENCH_WORKFLOWTASK_H

#include <map>
#include <stack>

#include <lemon/list_graph.h>

#include "workflow_job/WorkflowJob.h"
#include "WorkflowFile.h"


namespace wrench {

		/**
		 * @brief class to represent a task in a Workflow
		 */
		class WorkflowTask {


		public:

				std::string getId() const;
				double getFlops() const;
				int getNumProcs() const;
				int getNumberOfChildren();
				int getNumberOfParents();
				void addInputFile(WorkflowFile *);
				void addOutputFile(WorkflowFile *);

				/***********************/
				/** \cond DEVELOPER    */
				/***********************/

				/* Task-state enum */
				enum State {
						NOT_READY,
						READY,
						PENDING,
						RUNNING,
						COMPLETED,
						FAILED
				};

				WorkflowTask::State getState();

				/***********************/
				/** \endcond           */
				/***********************/

				/***********************/
				/** \cond INTERNAL     */
				/***********************/

				void setWorkflowJob(WorkflowJob *);
				WorkflowJob *getWorkflowJob();

				/***********************/
				/** \endcond           */
				/***********************/

		private:

				friend class Workflow;

				// These friend clauses are bad
				friend class StandardJob;
				friend class JobManager;
				friend class MulticoreJobExecutor;
				friend class SequentialTaskExecutor;
				friend class RandomScheduler;

				std::string id;										// Task ID
				double flops;											// Number of flops
				int number_of_processors;        	// currently vague: cores? nodes?
				double scheduled_date = -1.0;			// Date at which task was scheduled (getter?)
				double start_date = -1.0;					// Date at which task began execution (getter?)
				double end_date = -1.0;						// Date at which task finished execution (getter?)

				State state;
				Workflow *workflow;    																// Containing workflow
				lemon::ListDigraph *DAG;    													// Containing workflow
				lemon::ListDigraph::Node DAG_node;									 	// pointer to the underlying DAG node
				std::map<std::string, WorkflowFile *> output_files;  	// List of output files
				std::map<std::string, WorkflowFile *> input_files;   	// List of input files

				// Private constructor (called by Workflow)
				WorkflowTask(const std::string id, const double t, const int n);

				// Only WRENCH internals can change a task state
				void setState(WorkflowTask::State);
				void setReady();
				void setRunning();
				void setCompleted();


				// Containing job
				WorkflowJob *job;

				// Private helper function
				void addFileToMap(std::map<std::string, WorkflowFile *> map, WorkflowFile *f);

		};

};


#endif //WRENCH_WORKFLOWTASK_H
