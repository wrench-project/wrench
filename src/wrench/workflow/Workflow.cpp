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
		bool Workflow::pathExists(WorkflowTask *src, WorkflowTask *dst) {
			Bfs<ListDigraph> bfs(*DAG);


			bool reached = bfs.run(src->DAG_node, dst->DAG_node);
			//std::cout << "PATH from " << src->id << " to " << dst->id << ": " << reached << std::endl;
			return reached;
		}




		/******************************/
		/**      PUBLIC METHODS     **/
		/******************************/

		/**
		 * @brief  Constructor
		 *
		 */

		Workflow::Workflow() {
			DAG = std::unique_ptr<ListDigraph>(new ListDigraph());
			DAG_node_map = std::unique_ptr<ListDigraph::NodeMap<WorkflowTask*>>(
							new ListDigraph::NodeMap<WorkflowTask*>(*DAG));
		};


		/**
		 * @brief Creates and adds a new task to the workflow
		 *
		 * @param id is a unique string id
		 * @param execution_time is a reference execution time in second
		 * @param num_procs is a number of processors
		 *
		 * @return a pointer to a WorkflowTask object
		 */
		WorkflowTask* Workflow::addTask(const std::string id,
																		double execution_time,
																		int num_procs = 1) {

			// Check that the task doesn't really exist
			if (tasks[id]) {
				throw WRENCHException("Task ID already exists");
			}

			// Create the WorkflowTask object
			WorkflowTask  *task = new WorkflowTask(id, execution_time, num_procs);
			// Create a DAG node for it
			task->workflow = this;
			task->DAG = this->DAG.get();
			task->DAG_node = DAG->addNode();
			// Add it to the DAG node's metadata
			(*DAG_node_map)[task->DAG_node] = task;
			// Add it to the set of workflow tasks
			tasks[task->id] = std::unique_ptr<WorkflowTask>(task); // owner

			return task;
		}

		/**
		 * @brief Finds a WorkflowTask object based on its ID
		 *
		 * @param id is a string id
		 *
		 * @return a pointer to a WorkflowTask object
		 */
		WorkflowTask* Workflow::getWorkflowTaskByID(const std::string id) {
			if (!tasks[id]) {
				throw WRENCHException("Unknown WorkflowTask ID " + id);
			}
			return tasks[id].get();
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
		void Workflow::addControlDependency(WorkflowTask *src, WorkflowTask *dst) {
			if (!pathExists(src, dst)) {
				DAG->addArc(src->DAG_node, dst->DAG_node);
				if (src->getState() != WorkflowTask::COMPLETED) {
					updateTaskState(dst, WorkflowTask::NOT_READY);
				}
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
		void Workflow::addDataDependency(WorkflowTask *src, WorkflowTask *dst,
																		 WorkflowFile *file) {
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
		 * @return a pointer to a WorkflowFile object
		 */
		WorkflowFile * Workflow::addFile(const std::string id, double size) {
			// Create the WorkflowFile object
			WorkflowFile *file = new WorkflowFile(id, size);
			// Add if to the set of workflow files
			files[file->id] = std::unique_ptr<WorkflowFile>(file);

			return file;
		}

		/**
		 * @brief Finds a WorkflowFile object based on its ID
		 *
		 * @param id is a string id
		 *
		 * @return a pointer to a WorkflowFile object
		 */
		WorkflowFile *Workflow::getWorkflowFileByID(const std::string id) {
			if (!files[id]) {
				throw WRENCHException("Unknown WorkflowFile ID " + id);
			}
			return files[id].get();
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

		/**
		 * @brief method to get a vector of the ready tasks (very inefficiently
		 *        implemented right now)
		 * @return vector of pointers to workflow tasks
		 */
		std::vector<WorkflowTask *> Workflow::getReadyTasks() {

			std::vector<WorkflowTask *> task_list;

			std::map<std::string, std::unique_ptr<WorkflowTask>>::iterator it;
			for (it = this->tasks.begin(); it != this->tasks.end(); it++ )
			{
				WorkflowTask *task = it->second.get();
				if (task->getState() == WorkflowTask::READY) {
					task_list.push_back(task);
				}
			}
			return task_list;
		}



		/**
		 * @brief method to check whether all tasks are complete
		 *
		 * @return true or false
		 */
		bool Workflow::isDone() {
			std::map<std::string, std::unique_ptr<WorkflowTask>>::iterator it;
			for (it = this->tasks.begin(); it != this->tasks.end(); it++ )
			{
				WorkflowTask *task = it->second.get();
				// std::cerr << "==> " << task->id << " " << task->state << std::endl;
				if (task->getState() != WorkflowTask::COMPLETED) {
					return false;
				}
			}
			return true;
		}

		/**
		 * @brief method to update the state of a task, and propagate the change
		 *        to other tasks if necessary.
		 * @param task is a pointer to the task
		 * @param state is the new task state
		 */
		void Workflow::updateTaskState(WorkflowTask *task, WorkflowTask::State state){

			switch(state) {
				// Make a task completed, which may cause its children to become ready
				case WorkflowTask::COMPLETED:
				{
					if (task->getState() != WorkflowTask::RUNNING) {
						throw WRENCHException("Workflow::updateTaskState(): Cannot set non-running task state to completed");
					}
					task->setState(WorkflowTask::COMPLETED);
					// Go through the children and make them ready if possible
					for (ListDigraph::OutArcIt a(*DAG, task->DAG_node); a != INVALID; ++a) {
						WorkflowTask *child = (*DAG_node_map)[(*DAG).target(a)];
						updateTaskState(child, WorkflowTask::READY);

					}
					break;
				}
				case WorkflowTask::READY:
				{
					if (task->getState() != WorkflowTask::NOT_READY) {
						throw WRENCHException("Workflow::updateTaskState(): Cannot set non-not_ready task state to ready");
					}
					// Go through the parent and check whether they are all completed
					for (ListDigraph::InArcIt a(*DAG, task->DAG_node); a != INVALID; ++a) {
						WorkflowTask *parent = (*DAG_node_map)[(*DAG).source(a)];
						if (parent->getState() != WorkflowTask::COMPLETED) {
							// At least one parent is not in the COMPLETED state
							return;
						}
					}
					task->setState(WorkflowTask::READY);

					break;
				}

				case WorkflowTask::SCHEDULED:
				{
					task->setState(WorkflowTask::SCHEDULED);
					break;
				}
				case WorkflowTask::RUNNING:
				{
					task->setState(WorkflowTask::RUNNING);
					break;
				}
				case WorkflowTask::FAILED:
				{
					task->setState(WorkflowTask::FAILED);
					break;
				}
				case WorkflowTask::NOT_READY:
				{
					task->setState(WorkflowTask::NOT_READY);
					break;
				}
				default:
				{
					throw WRENCHException("Workflow::updateTaskState(): invalid state");
				}
			}
		}

}
