/**
 *  @file    WorkflowTask.cpp
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


#include <lemon/list_graph.h>
#include "WorkflowTask.h"

namespace WRENCH {

		/******************************/
		/**      PRIVATE METHODS     **/
		/******************************/

		/**
		 * Constructor
		 **/
		WorkflowTask::WorkflowTask(const std::string string, const double t, const int n) {
			id = string;
			execution_time = t;
			number_of_processors = n;
			state = WorkflowTask::READY;
		}

		/**
		 * addInputFile():
		 *   - will not do anything if file is already present
		 **/
		void WorkflowTask::addInputFile(std::shared_ptr<WorkflowFile> f) {
			addFileToMap(input_files, f);
		}

		/**
		 * addOutputFile():
		 *   - will not do anything if file is already present
		 **/
		void WorkflowTask::addOutputFile(std::shared_ptr<WorkflowFile> f) {
			addFileToMap(output_files, f);
		}

		/**
		 * Generic helper function to implement the above
		 */
		void WorkflowTask::addFileToMap(std::map<std::string, std::shared_ptr<WorkflowFile>> map,
																		std::shared_ptr<WorkflowFile> f) {
			if (!map[f->id]) {
				map[f->id] = f;
			}
		}


		/******************************/
		/**      PUBLIC METHODS      **/
		/******************************/


		/**
		 *
		 * @brief Computes the number of children of a task
		 *
		 * @return number of children
		 */
		int WorkflowTask::getNumberOfChildren() {
			int count = 0;
			for (ListDigraph::OutArcIt a(*DAG, DAG_node); a != INVALID; ++a) {
				++count;
			}
			return count;
		}

		/**
		 *
		 * @brief Computes the number of parents of a task
		 *
		 * @return number of parents
		 */
		int WorkflowTask::getNumberOfParents() {
			int count = 0;
			for (ListDigraph::InArcIt a(*DAG, DAG_node); a != INVALID; ++a) {
				++count;
			}
			return count;
		}

		/**
		 *
		 * @brief Returns the state of the task
		 *
		 * @return task state
		 */
		WorkflowTask::State WorkflowTask::getState() {
			return state;
		}

};


