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
#include "Workflow.h"

namespace WRENCH {


		/**
		 * @brief Constructor
		 *
		 * @param string is the task id
		 * @param t is the task execution time
		 * @param n is the number of processors for running the task
		 */
		WorkflowTask::WorkflowTask(const std::string string, const double t, const int n) {
			id = string;
			flops = t;
			number_of_processors = n;
			state = WorkflowTask::READY;
		}

		/**
		 * @brief adds an input file to the task
		 *
		 * @param f is a pointer to the file
		 */
		void WorkflowTask::addInputFile(WorkflowFile *f) {
			addFileToMap(input_files, f);
			f->setInputOf(this);
			// Perhaps add a control dependency?
			if (f->getOutputOf()) {
				workflow->addControlDependency(f->getOutputOf(), this);
			}
		}

		/**
		 * @brief adds an output file to the task
		 *
		 * @param f is a pointer to the file
		 */
		void WorkflowTask::addOutputFile(WorkflowFile *f) {
			addFileToMap(output_files, f);
			f->setOutputOf(this);
			// Perhaps add control dependencies?
			for (auto const &x : f->getInputOf()) {
				workflow->addControlDependency(this, x.second);
			}

		}

		/**
		 * @brief helper method to add a file to a map if necessary
		 *
		 * @param map is the map
		 * @param f is a pointer to a file
		 */
		void WorkflowTask::addFileToMap(std::map<std::string, WorkflowFile*> map,
																		WorkflowFile * f) {
			map[f->id] = f;

		}


		/**
		 * @brief Returns the id of the task
		 *
		 * @return
		 */
		std::string WorkflowTask::getId() {
			return this->id;
		}

		/**
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
		 * @brief Returns the state of the task
		 *
		 * @return task state
		 */
		WorkflowTask::State WorkflowTask::getState() {
			return state;
		}

		/**
		 * @brief Sets the state of the task
		 */
		void WorkflowTask::setState(WorkflowTask::State state) {
			this->state = state;
		}

		/**
		 * @brief Sets the task to the scheduled state
		 * @param task
		 */

		void WorkflowTask::setScheduled() {
			this->workflow->updateTaskState(this, WorkflowTask::SCHEDULED);
		}


		/**
		 * @brief Sets the task to the running state
		 * @param task
		 */

		void WorkflowTask::setRunning() {
			this->workflow->updateTaskState(this, WorkflowTask::RUNNING);
		}

		/**
		 * @brief Sets the task to the completed state
		 * @param task
		 */

		void WorkflowTask::setCompleted() {
			this->workflow->updateTaskState(this, WorkflowTask::COMPLETED);
		}


};


