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

#include <lemon/list_graph.h>
#include <lemon/graph_to_eps.h>
#include <lemon/bfs.h>
#include <pugixml.hpp>
#include <simulation/SimulationMessage.h>
#include <simgrid_S4U_util/S4U_Mailbox.h>

#include "workflow/Workflow.h"
#include "exception/WRENCHException.h"


namespace wrench {

		/**
		 * @brief Create and add a new task to the workflow
		 *
		 * @param id is a unique string id
		 * @param execution_time is a reference execution time in second
		 * @param num_procs is a number of processors
		 *
		 * @return a pointer to a WorkflowTask object
		 */
		WorkflowTask *Workflow::addTask(const std::string id,
																		double execution_time,
																		int num_procs = 1) {

			// Check that the task doesn't really exist
			if (tasks[id]) {
				throw WRENCHException("Task ID already exists");
			}

			// Create the WorkflowTask object
			WorkflowTask *task = new WorkflowTask(id, execution_time, num_procs);
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
		 * @brief Find a WorkflowTask object based on its ID
		 *
		 * @param id is a string id
		 *
		 * @return a pointer to a WorkflowTask object
		 */
		WorkflowTask *Workflow::getWorkflowTaskByID(const std::string id) {
			if (!tasks[id]) {
				throw WRENCHException("Unknown WorkflowTask ID " + id);
			}
			return tasks[id].get();
		}


		/**
		 * @brief Create a control dependency between two workflow tasks. Will not
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
		 * @brief Add a new file to the workflow specification
		 *
		 * @param id is a unique string id
		 * @param size is a file size in bytes
		 *
		 * @return a pointer to a WorkflowFile object
		 */
		WorkflowFile *Workflow::addFile(const std::string id, double size) {
			// Create the WorkflowFile object
			WorkflowFile *file = new WorkflowFile(id, size);
			file->workflow = this;
			// Add if to the set of workflow files
			files[file->id] = std::unique_ptr<WorkflowFile>(file);

			return file;
		}

		/**
		 * @brief Find a WorkflowFile object based on its ID
		 *
		 * @param id is a string id
		 *
		 * @return a pointer to a WorkflowFile object, nullptr if not found
		 */
		WorkflowFile *Workflow::getWorkflowFileByID(const std::string id) {
			if (!files[id]) {
				return nullptr;
			} else {
				return files[id].get();
			}
		}

		/**
		 * @brief Output a workflow's dependency graph to EPS
		 *
		 * @param eps_filename is a filename to which the EPS content is saved
		 *
		 */
		void Workflow::exportToEPS(std::string eps_filename) {
			graphToEps(*DAG, eps_filename).run();
			std::cerr << "Export to EPS broken / not implemented at the moment" << std::endl;
		}

		/**
		 * @brief Get the number of tasks in the workflow
		 * @return number of tasks
		 */
		unsigned long Workflow::getNumberOfTasks() {
			return this->tasks.size();

		}

		/**
		 * @brief Create a workflow based on a DAX file
		 *
		 * @param filename is the path to the file
		 */
		void Workflow::loadFromDAX(const std::string filename) {
			pugi::xml_document dax_tree;

			if (!dax_tree.load_file(filename.c_str())) {
				throw WRENCHException("Workflow::loadFromDAX(): Invalid DAX file");
			}

			// Get the root node
			pugi::xml_node dag = dax_tree.child("adag");

			// Iterate through the "job" nodes
			for (pugi::xml_node job = dag.child("job"); job; job = job.next_sibling("job")) {
				WorkflowTask *task;
				// Get the job attributes
				std::string id = job.attribute("id").value();
				std::string name = job.attribute("name").value();
				double flops = std::strtod(job.attribute("runtime").value(), NULL);
				int num_procs = 1;
				if (job.attribute("num_procs")) {
					num_procs = std::stoi(job.attribute("num_procs").value());
				}
				// Create the task
				task = this->addTask(id + "_" + name, flops, num_procs);


				// Go through the children "uses" nodes
				for (pugi::xml_node uses = job.child("uses"); uses; uses = uses.next_sibling("uses")) {
					// get the "uses" attributes
					// TODO: There are several attributes that we're ignoring for now...
					std::string id = uses.attribute("file").value();

					double size = std::strtod(uses.attribute("size").value(), NULL);
					std::string link = uses.attribute("link").value();
					// Check whether the file already exists
					std::cerr.flush();
					WorkflowFile *file = this->getWorkflowFileByID(id);

					if (!file) {
						file = this->addFile(id, size);
					}
					if (link == "input") {
						task->addInputFile(file);
					}
					if (link == "output") {
						task->addOutputFile(file);
					}
					// TODO: Are there other types of "link" values?
				}
			}
		}


		/***********************************************************/
		/**	DEVELOPER METHODS BELOW **/
		/***********************************************************/

		/*! \cond DEVELOPER */

		/**
		 * @brief Determine whether one source is an ancestor of a destination task
		 *
		 * @param src is a pointer to the source task
		 * @param dst is a pointer to the destination task
		 * @return true if there is a path from src to dst
		 */
		bool Workflow::pathExists(WorkflowTask *src, WorkflowTask *dst) {
			Bfs<ListDigraph> bfs(*DAG);


			bool reached = bfs.run(src->DAG_node, dst->DAG_node);
			return reached;
		}




		/**
		 * @brief  Constructor
		 */
		Workflow::Workflow() {
			DAG = std::unique_ptr<ListDigraph>(new ListDigraph());
			DAG_node_map = std::unique_ptr<ListDigraph::NodeMap<WorkflowTask *>>(
							new ListDigraph::NodeMap<WorkflowTask *>(*DAG));
			this->callback_mailbox = S4U_Mailbox::generateUniqueMailboxName("workflow_mailbox");
		};




		/**
		 * @brief Get a vector of the ready tasks (very inefficiently
		 *        implemented right now)
		 * @return vector of pointers to workflow tasks
		 */
		std::vector<WorkflowTask *> Workflow::getReadyTasks() {

			std::vector<WorkflowTask *> task_list;

			std::map<std::string, std::unique_ptr<WorkflowTask>>::iterator it;
			for (it = this->tasks.begin(); it != this->tasks.end(); it++) {
				WorkflowTask *task = it->second.get();
				if (task->getState() == WorkflowTask::READY) {
					task_list.push_back(task);
				}
			}
			return task_list;
		}

		/**
		 * @brief Check whether all tasks are complete
		 *
		 * @return true or false
		 */
		bool Workflow::isDone() {
			std::map<std::string, std::unique_ptr<WorkflowTask>>::iterator it;
			for (it = this->tasks.begin(); it != this->tasks.end(); it++) {
				WorkflowTask *task = it->second.get();
				// std::cerr << "==> " << task->id << " " << task->state << std::endl;
				if (task->getState() != WorkflowTask::COMPLETED) {
					return false;
				}
			}
			return true;
		}


		/*! \endcond */


		/***********************************************************/
		/**	INTERNAL METHODS BELOW **/
		/***********************************************************/

		/*! \cond INTERNAL */

		/**
		 * @brief Wait for a task completion
		 * @return the completed task
		 */
		std::unique_ptr<WorkflowExecutionEvent> Workflow::waitForNextExecutionEvent() {
			return WorkflowExecutionEvent::waitForNextExecutionEvent(this->callback_mailbox);
		}

		/**
		 * @brief Get the mailbox name associated to this workflow, i.e.,
		 *        the mailbox to which "TASK_DONE" messages are sent
		 *
		 * @return the mailbox name
		 */
		std::string Workflow::getCallbackMailbox() {
			return this->callback_mailbox;
		}

		/**
		 * @brief Update the state of a task, and propagate the change
		 *        to other tasks if necessary.
		 * @param task is a pointer to the task
		 * @param state is the new task state
		 */
		void Workflow::updateTaskState(WorkflowTask *task, WorkflowTask::State state) {

			switch (state) {
				// Make a task completed, which may cause its children to become ready
				case WorkflowTask::COMPLETED: {
					if (task->getState() != WorkflowTask::RUNNING) {
						throw WRENCHException(
										"Workflow::updateTaskState(): Cannot set non-running task state to completed");
					}
					task->setState(WorkflowTask::COMPLETED);
					// Go through the children and make them ready if possible
					for (ListDigraph::OutArcIt a(*DAG, task->DAG_node); a != INVALID; ++a) {
						WorkflowTask *child = (*DAG_node_map)[(*DAG).target(a)];
						updateTaskState(child, WorkflowTask::READY);

					}
					break;
				}
				case WorkflowTask::READY: {
					if (task->getState() == WorkflowTask::READY) {
						return;
					}
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
				case WorkflowTask::RUNNING: {
					task->setState(WorkflowTask::RUNNING);
					break;
				}
				case WorkflowTask::FAILED: {
					task->setState(WorkflowTask::FAILED);
					break;
				}
				case WorkflowTask::NOT_READY: {
					task->setState(WorkflowTask::NOT_READY);
					break;
				}
				default: {
					throw WRENCHException("Workflow::updateTaskState(): invalid state");
				}
			}
		}

		/*! \endcond */
};
