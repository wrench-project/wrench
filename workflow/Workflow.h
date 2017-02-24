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
				std::shared_ptr<ListDigraph> DAG;  // Lemon DiGraph
				std::shared_ptr<ListDigraph::NodeMap<std::shared_ptr<WorkflowTask>>> DAG_node_map;  // Lemon map

				std::map<std::string, std::shared_ptr<WorkflowTask>> tasks;  // STL map
				std::map<std::string, std::shared_ptr<WorkflowFile>> files;  // STL map

		private:
				bool pathExists(std::shared_ptr<WorkflowTask>, std::shared_ptr<WorkflowTask>);
				void updateTaskReadyState(std::shared_ptr<WorkflowTask>);


		public:
				Workflow();
				~Workflow();

				std::shared_ptr<WorkflowTask> addTask(std::string, double, int);
				std::shared_ptr<WorkflowTask> getWorkflowTaskByID(const std::string);

				void addControlDependency(std::shared_ptr<WorkflowTask>,
																	std::shared_ptr<WorkflowTask>);
				void addDataDependency(std::shared_ptr<WorkflowTask>,
																				 std::shared_ptr<WorkflowTask>,
																				 std::shared_ptr<WorkflowFile>);

				std::shared_ptr<WorkflowFile> addFile(const std::string, double);
				std::shared_ptr<WorkflowFile> getWorkflowFileByID(const std::string);

				void exportToEPS(std::string);

				unsigned long getNumberOfTasks();


				void makeTaskCompleted(std::shared_ptr<WorkflowTask> task);

				// For initial testing
				std::shared_ptr<WorkflowTask> getSomeReadyTask();




		};

};


#endif //WRENCH_WORKFLOW_H
