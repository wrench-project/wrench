/**
 *  @file    WorkflowTask.h
 *  @author  Henri Casanova
 *  @date    2/21/2017
 *  @version 1.0
 *
 *  @brief WRENCH::WorkflowTask class implementation
 *
 *  @section DESCRIPTION
 *
 *  The WRENCH::WorkflowTask class represents a computational
 *  task in a WRENCH:Workflow.
 *
 */
#ifndef WRENCH_WORKFLOWTASK_H
#define WRENCH_WORKFLOWTASK_H

#include <map>
#include <lemon/list_graph.h>

#include "WorkflowFile.h"


using namespace lemon;

namespace WRENCH {

		// Forward reference
		class Workflow;


		/* Task meta-data class */
		class WorkflowTask {

				// Workflow class must be a friend so as to access the private constructor, etc.
				friend class Workflow;

				/* Task-state enum */
				enum State {
						READY,
						NOT_READY,
						SCHEDULED,
						RUNNING,
						COMPLETED,
						FAILED
				};

		public:
				std::string id;
				double flops;
				int number_of_processors;		// currently vague: cores? nodes?
				State state;
				double submit_date = -1.0;
				double start_date = -1.0;
				double end_date = -1.0;

		private:
				Workflow *workflow; 	// Containing workflow
				std::shared_ptr<ListDigraph> DAG; 	// Containing workflow
				ListDigraph::Node DAG_node; // pointer to the underlying DAG node
				std::map<std::string, std::shared_ptr<WorkflowFile>> output_files;  // List of output files
				std::map<std::string, std::shared_ptr<WorkflowFile>> input_files;   // List of input files


		private:
				// Private constructor
				WorkflowTask(const std::string id, const double t, const int n);

				void addInputFile(std::shared_ptr<WorkflowFile> f);
				void addOutputFile(std::shared_ptr<WorkflowFile> f);
				void addFileToMap(std::map<std::string, std::shared_ptr<WorkflowFile>> map, std::shared_ptr<WorkflowFile> f);

		public:
				int getNumberOfChildren();
				int getNumberOfParents();
				WorkflowTask::State getState();

		};

};


#endif //WRENCH_WORKFLOWTASK_H
