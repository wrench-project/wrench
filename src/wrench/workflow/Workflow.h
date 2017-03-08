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
				void updateTaskReadyState(WorkflowTask *);


		public:
				Workflow();
				~Workflow();

				WorkflowTask *addTask(std::string, double, int);
				WorkflowTask *getWorkflowTaskByID(const std::string);

				void addControlDependency(WorkflowTask *,
																	WorkflowTask *);
				void addDataDependency(WorkflowTask *, WorkflowTask *, WorkflowFile *);

				WorkflowFile *addFile(const std::string, double);
				WorkflowFile *getWorkflowFileByID(const std::string);

				void exportToEPS(std::string);

				unsigned long getNumberOfTasks();


				void makeTaskCompleted(WorkflowTask *task);

				// For initial testing
				WorkflowTask* getSomeReadyTask();




		};

};


#endif //WRENCH_WORKFLOW_H
