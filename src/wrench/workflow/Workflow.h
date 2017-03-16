/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief WRENCH::Workflow provides all basic functionality
 *  to represent/instantiate/manipulate a WRENCH Workflow.
 */

#ifndef WRENCH_WORKFLOW_H
#define WRENCH_WORKFLOW_H

#include <lemon/list_graph.h>
#include <map>
#include <simulation/WorkflowExecutionEvent.h>

#include "WorkflowTask.h"
#include "WorkflowFile.h"

using namespace lemon;

namespace wrench {

	class Workflow {

	/**************/
	/**  PUBLIC  **/
	/**************/

	public:
		Workflow();

		WorkflowTask *addTask(std::string, double, int);
		WorkflowTask *getWorkflowTaskByID(const std::string);

		WorkflowFile *addFile(const std::string, double);
		WorkflowFile *getWorkflowFileByID(const std::string);

		void addControlDependency(WorkflowTask *, WorkflowTask *);
		//void addDataDependency(WorkflowTask *, WorkflowTask *, WorkflowFile *);

		void loadFromDAX(const std::string filename);

		/** Get information from the workflow **/
		// TODO: Make these efficient - Right now they are really naively implemented
		unsigned long getNumberOfTasks();
		bool isDone();
		std::vector<WorkflowTask *> getReadyTasks();

		/** misc **/
		void exportToEPS(std::string);

	/***************/
	/**  PRIVATE  **/
	/***************/

	friend class WorkflowTask;
	friend class SimpleWMSDaemon;

	private:

		std::unique_ptr<ListDigraph> DAG;  // Lemon DiGraph
		std::unique_ptr<ListDigraph::NodeMap<WorkflowTask *>> DAG_node_map;  // Lemon map

		std::map<std::string, std::unique_ptr<WorkflowTask>> tasks;
		std::map<std::string, std::unique_ptr<WorkflowFile>> files;

		bool pathExists(WorkflowTask *, WorkflowTask *);

		std::string callback_mailbox;

		/** Update task state **/
		void update_task_state(WorkflowTask *task, WorkflowTask::State state);

		/** Method to get the callback mailbox associated with the workflow **/
		std::string get_callback_mailbox();

		/** Method to wait the next event **/
		std::unique_ptr<WorkflowExecutionEvent> wait_for_next_execution_event();

	};

};


#endif //WRENCH_WORKFLOW_H
