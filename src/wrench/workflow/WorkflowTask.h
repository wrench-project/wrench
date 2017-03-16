/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief WRENCH::WorkflowFile represents a data file used in a WRENCH::Workflow.
 */

#ifndef WRENCH_WORKFLOWTASK_H
#define WRENCH_WORKFLOWTASK_H

#include <map>
#include <lemon/list_graph.h>
#include <stack>
#include "WorkflowFile.h"


namespace wrench {

	// Forward reference
	class Workflow;


	/* Task meta-data class */
	class WorkflowTask {

		// Workflow class must be a friend so as to access the private constructor, etc.
		friend class Workflow;
		friend class MulticoreTaskExecutorDaemon;
		friend class SequentialTaskExecutorDaemon;

	public:
		/* Task-state enum */
		enum State {
			NOT_READY,
			READY,
			SCHEDULED,
			RUNNING,
			COMPLETED,
			FAILED
		};

	public:
		std::string id;
		double flops;
		int number_of_processors;        // currently vague: cores? nodes?
		double submit_date = -1.0;
		double start_date = -1.0;
		double end_date = -1.0;

	private:
		State state;
		Workflow *workflow;    // Containing workflow
		lemon::ListDigraph *DAG;    // Containing workflow
		lemon::ListDigraph::Node DAG_node; // pointer to the underlying DAG node
		std::map<std::string, WorkflowFile *> output_files;  // List of output files
		std::map<std::string, WorkflowFile *> input_files;   // List of input files

	protected:
		std::stack<std::string> callback_mailbox_stack;
		std::string pop_callback_mailbox();
		void push_callback_mailbox(std::string);


	private:
		// Private constructor
		WorkflowTask(const std::string id, const double t, const int n);

		void addFileToMap(std::map<std::string, WorkflowFile *> map, WorkflowFile *f);
		void setState(WorkflowTask::State);


	public:
		std::string getId();
		void addInputFile(WorkflowFile *);
		void addOutputFile(WorkflowFile *);
		int getNumberOfChildren();
		int getNumberOfParents();
		WorkflowTask::State getState();
		void setReady();
		void setScheduled();
		void setRunning();
		void setCompleted();
		std::string getCallbackMailbox();


	};

};


#endif //WRENCH_WORKFLOWTASK_H
