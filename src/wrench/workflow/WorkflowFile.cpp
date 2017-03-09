
/**
 *  @file    WorkflowFile.cpp
 *  @author  Henri Casanova
 *  @date    3/9/2017
 *  @version 1.0
 *
 *  @brief WRENCH::WorkflowFile class implementation
 *
 *  @section DESCRIPTION
 *
 *  The WRENCH::WorkflowFile class represents a data file used in a WRENCH::Workflow.
 *
 */


#include <map>

#include "WorkflowFile.h"
#include "WorkflowTask.h"


namespace WRENCH {

		/**
		 * @brief Constructor
		 *
		 * @param string is the file name/id
		 * @param s is the file size
		 */
		WorkflowFile::WorkflowFile(const std::string string, double s) {
			this->id = string;
			this->size = s;
		};

		/**
		 * @brief Method to define the task that outputs this file
		 *
		 * @param task is the task
		 */
		void WorkflowFile::setOutputOf(WorkflowTask *task) {
			this->output_of = task;
		}

		/**
		 * @brief Method to get the task the outputs this file
		 *
		 * @return a pointer to a WorkflowTask
		 */
		WorkflowTask *WorkflowFile::getOutputOf() {
			return this->output_of;
		}


		/**
		 * @brief Method to add a task that uses this file as input
		 *
		 * @param task is the task
		 */
		void WorkflowFile::setInputOf(WorkflowTask *task) {
			this->input_of[task->getId()] = task;
		}

		/**
		 * @brief Method to get the set of tasks that use this file as input
		 *
		 * @return a map of pointers to WorkflowTask
		 */
		std::map<std::string, WorkflowTask *> WorkflowFile::getInputOf() {
			return this->input_of;
		};


};

