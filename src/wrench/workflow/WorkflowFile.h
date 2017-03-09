/**
 *  @file    WorkflowFile.h
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


#ifndef WRENCH_WORKFLOWFILE_H
#define WRENCH_WORKFLOWFILE_H

#include <string>
#include <map>

namespace WRENCH {

		class WorkflowTask;
		class Workflow;

// Workflow class must be a friend so as to access the private constructor, etc.

		class WorkflowFile {

				friend class Workflow;
				friend class WorkflowTask;

		public:
				std::string id;
				double size; // in bytes

		protected:
				void setOutputOf(WorkflowTask *task);
				WorkflowTask *getOutputOf();
				void setInputOf(WorkflowTask *task);
				std::map<std::string, WorkflowTask *> getInputOf();


		private:
				Workflow *workflow; // Containing workflow

				WorkflowFile(const std::string, double);

				WorkflowTask *output_of;
				std::map<std::string, WorkflowTask *> input_of;

		};

};

#endif //WRENCH_WORKFLOWFILE_H
