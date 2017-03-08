/**
 *  @file    Workflow.h
 *  @author  Henri Casanova
 *  @date    2/21/2017
 *  @version 1.0
 *
 *  @brief WRENCH::Platform class implementation
 *
 *  @section DESCRIPTION
 *
 *  The WRENCH::Platform class provides all basic functionality
 *  to represent/instantiate/manipulate a SimGrid simulation platform.
 *
 */

#ifndef WRENCH_WORKFLOW_H
#define WRENCH_WORKFLOW_H

#include <lemon/list_graph.h>
#include <map>

#include "WorkflowTask.h"
#include "WorkflowFile.h"

using namespace lemon;

namespace WRENCH {


		class Workflow {

		private:
				std::unique_ptr<ListDigraph> DAG;  // Lemon DiGraph
				std::unique_ptr<ListDigraph::NodeMap<WorkflowTask*>> DAG_node_map;  // Lemon map

				std::map<std::string, std::unique_ptr<WorkflowTask>> tasks;
				std::map<std::string, std::unique_ptr<WorkflowFile>> files;

		private:
				bool pathExists(WorkflowTask *, WorkflowTask *);


		public:

				/** Constructor and the like **/
				Workflow();

				WorkflowTask *addTask(std::string, double, int);
				WorkflowTask *getWorkflowTaskByID(const std::string);

				WorkflowFile *addFile(const std::string, double);
				WorkflowFile *getWorkflowFileByID(const std::string);

				void addControlDependency(WorkflowTask *, WorkflowTask *);
				void addDataDependency(WorkflowTask *, WorkflowTask *, WorkflowFile *);

				/** Update task state **/
			  void updateTaskState(WorkflowTask *task, WorkflowTask::State state);


				/** Get information from the workflow **/
			  // TODO: Make these efficient - Right now they are really naively implemented
				unsigned long getNumberOfTasks();
				bool isDone();
				std::vector<WorkflowTask *> getReadyTasks();

				/** misc **/
				void exportToEPS(std::string);



		};

};


#endif //WRENCH_WORKFLOW_H
