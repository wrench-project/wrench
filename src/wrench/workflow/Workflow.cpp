/**
 *  @file    Workflow.cpp
 *  @author  Henri Casanova
 *  @date    2/21/2017
 *  @version 1.0
 *
 *  @brief WRENCH::Workflow class implementation
 *
 *  @section DESCRIPTION
 *
 *  The WRENCH::Workflow class provides all basic functionality
 *  to represent/instantiate/manipulate a WRENCH Workflow.
 *
 */

#include <lemon/list_graph.h>
#include <lemon/graph_to_eps.h>
#include <lemon/bfs.h>

#include "Workflow.h"
#include "exception/WRENCHException.h"


namespace WRENCH {

		/******************************/
		/**      PRIVATE METHODS     **/
		/******************************/

		/*
		 * pathExists()
		 */
		bool Workflow::pathExists(std::shared_ptr<WorkflowTask> src, std::shared_ptr<WorkflowTask> dst) {
			Bfs<ListDigraph> bfs(*DAG);
			bool reached = bfs.run(src->DAG_node, dst->DAG_node);
			//std::cout << "PATH from " << src->id << " to " << dst->id << ": " << reached << std::endl;
			return reached;
		}

		/*
		 * updateTaskReadyState()
		 */
		void Workflow::updateTaskReadyState(std::shared_ptr<WorkflowTask> task) {

			if (task->state != WorkflowTask::NOT_READY) {
				return;
			}

			WorkflowTask::State putative_state = WorkflowTask::READY;
			// Iterate through the parents
			for (ListDigraph::InArcIt a(*DAG, task->DAG_node); a != INVALID; ++a) {
				WorkflowTask::State  parent_state = (*DAG_node_map)[(*DAG).source(a)]->state;
				if (parent_state != WorkflowTask::COMPLETED) {
					putative_state = WorkflowTask::NOT_READY;
					break;
				}
			}
			task->state = putative_state;
			return;
		}


		/******************************/
		/**      PUBLIC METHODS     **/
		/******************************/

		/**
		 * @brief  Constructor
		 *
		 */

		Workflow::Workflow() {
			DAG = std::make_shared<ListDigraph>();
			DAG_node_map = std::make_shared<ListDigraph::NodeMap<std::shared_ptr<WorkflowTask>>>(*DAG);
		};

		/**
		 * @brief  Destructor
		 *
		 */
		Workflow::~Workflow() {
		};


		/**
		 * @brief Creates and adds a new task to the workflow
		 *
		 * @param id is a unique string id
		 * @param execution_time is a reference execution time in second
		 * @param num_procs is a number of processors
		 *
		 * @return a SHARED POINTER to a WorkflowTask object
		 */
		std::shared_ptr<WorkflowTask> Workflow::addTask(const std::string id,
																										double execution_time,
																										int num_procs = 1) {

			// Check that the task doesn't really exist
			if (tasks[id]) {
				throw WRENCHException("Task ID already exists");
			}

			// Create the WorkflowTask object
			std::shared_ptr<WorkflowTask> task(new WorkflowTask(id, execution_time, num_procs));
			// Create a DAG node for it
			task->workflow = this;
			task->DAG = DAG;
			task->DAG_node = DAG->addNode();
			// Add it to the DAG node's metadata
			(*DAG_node_map)[task->DAG_node] = task;
			// Add it to the set of workflow tasks
			tasks[task->id] = task;

			return task;
		}

		/**
		 * @brief Finds a WorkflowTask object based on its ID
		 *
		 * @param id is a string id
		 *
		 * @return a SHARED POINTER to a WorkflowTask object
		 */
		std::shared_ptr<WorkflowTask> Workflow::getWorkflowTaskByID(const std::string id) {
			if (!tasks[id]) {
				throw WRENCHException("Unknown WorkflowTask ID " + id);
			}
			return tasks[id];
		}


		/**
		 * @brief Creates a control dependency between two workflow tasks. Will not
		 *        do anything if there is already a path between the two tasks.
		 *
		 * @param src is the source task
		 * @param dst is the destination task
		 *
		 * @return nothing
		 */
		void Workflow::addControlDependency(std::shared_ptr<WorkflowTask> src, std::shared_ptr<WorkflowTask> dst) {
			if (!pathExists(src, dst)) {
				DAG->addArc(src->DAG_node, dst->DAG_node);
				updateTaskReadyState(dst);
			}
		}

		/**
		 * @brief Creates a data dependency between two workflow tasks. Will add a file
		 *        to the input/output sets of the tasks (if not already there), and will add
		 *        a control dependency between the two tasks if necessary.
		 *
		 * @param src is the source task
		 * @param dst is the destination task
		 * @param file is the datA file
		 *
		 * @return nothing
		 */
		void Workflow::addDataDependency(std::shared_ptr<WorkflowTask> src, std::shared_ptr<WorkflowTask> dst,
																		 std::shared_ptr<WorkflowFile> file) {
			src->addOutputFile(file);
			dst->addInputFile(file);
			addControlDependency(src, dst);

		}



		/**
		 * @brief Adds a new file to the workflow specification
		 *
		 * @param id is a unique string id
		 * @param size is a file size in bytes
		 *
		 * @return a SHARED POINTER to a WorkflowFile object
		 */
		std::shared_ptr<WorkflowFile> Workflow::addFile(const std::string id,
																										double size) {
			// Create the WorkflowFile object
			std::shared_ptr<WorkflowFile> file(new WorkflowFile(id, size));
			// Add if to the set of workflow files
			files[file->id] = file;

			return file;
		}

		/**
		 * @brief Finds a WorkflowFile object based on its ID
		 *
		 * @param id is a string id
		 *
		 * @return a SHARED POINTER to a WorkflowFile object
		 */
		std::shared_ptr<WorkflowFile> Workflow::getWorkflowFileByID(const std::string id) {
			if (!files[id]) {
				throw WRENCHException("Unknown WorkflowFile ID " + id);
			}
			return files[id];
		}

		/**
		 * @brief Outputs a workflow's dependency graph to EPS
		 *
		 * @param eps_filename is a filename to which the EPS content is saved
		 *
		 */
		void Workflow::exportToEPS(std::string eps_filename) {
			// TODO: Figure out why this doesn't compile
			// (even though the standalone Lemon "export to EPS" example compiles fine

			//graphToEps(*DAG, eps_filename).run();
			std::cerr << "Export to EPS broken / not implemented at the moment" << std::endl;
		}

		/**
		 * @brief method to get the number of tasks in the workflow
		 * @return number of tasks
		 */
		unsigned long Workflow::getNumberOfTasks() {
			return this->tasks.size();

		}

		std::shared_ptr<WorkflowTask> Workflow::getSomeReadyTask() {

			std::map<std::string, std::shared_ptr<WorkflowTask>>::iterator it;
			for (it = this->tasks.begin(); it != this->tasks.end(); it++ )
			{
				std::shared_ptr<WorkflowTask> task = it->second;
				if (task->getState() == WorkflowTask::READY) {
					return task;
				}
			}
			return nullptr;
		}

		void Workflow::makeTaskCompleted(std::shared_ptr<WorkflowTask> task) {
			task->state = WorkflowTask::COMPLETED;
			for (ListDigraph::OutArcIt a(*DAG, task->DAG_node); a != INVALID; ++a) {
				updateTaskReadyState((*DAG_node_map)[(*DAG).source(a)]);
			}
		}


}
